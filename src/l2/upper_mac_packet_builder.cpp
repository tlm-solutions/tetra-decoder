/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "l2/upper_mac_packet_builder.hpp"
#include "burst_type.hpp"
#include "l2/logical_channel.hpp"
#include "l2/slot.hpp"
#include "l2/upper_mac_packet.hpp"
#include "utils/address.hpp"
#include "utils/bit_vector.hpp"
#include <cstddef>
#include <optional>
#include <ostream>
#include <stdexcept>

auto operator<<(std::ostream& stream, const UpperMacPackets& packets) -> std::ostream& {
    stream << "[UpperMacPacket]" << std::endl;
    for (const auto& packet : packets.c_plane_signalling_packets_) {
        if (packet.is_null_pdu()) {
            // filter out null pdus
            continue;
        }
        stream << packet << std::endl;
    }
    for (const auto& packet : packets.u_plane_signalling_packet_) {
        stream << packet << std::endl;
    }
    if (packets.u_plane_traffic_packet_) {
        stream << *packets.u_plane_traffic_packet_ << std::endl;
    }
    if (packets.broadcast_packet_) {
        stream << *packets.broadcast_packet_ << std::endl;
    }

    return stream;
}

auto UpperMacPacketBuilder::parse_slots(Slots& slots) -> UpperMacPackets {
    UpperMacPackets packets;

    {
        const auto& first_slot = slots.get_first_slot().get_logical_channel_data_and_crc();
        packets.merge(parse_slot(slots.get_burst_type(), first_slot));
    }

    if (slots.has_second_slot()) {
        const auto& second_slot = slots.get_second_slot().get_logical_channel_data_and_crc();
        packets.merge(parse_slot(slots.get_burst_type(), second_slot));
    }

    return packets;
}

auto UpperMacPacketBuilder::parse_slot(const BurstType burst_type,
                                       const LogicalChannelDataAndCrc& logical_channel_data_and_crc)
    -> UpperMacPackets {
    const auto& channel = logical_channel_data_and_crc.channel;
    auto data = BitVector(logical_channel_data_and_crc.data);
    if (channel == LogicalChannel::kTrafficChannel) {
        return UpperMacPackets{.u_plane_traffic_packet_ = parse_u_plane_traffic(channel, std::move(data))};
    }

    // filter out signalling packets with a wrong crc
    if (!logical_channel_data_and_crc.crc_ok) {
        return UpperMacPackets{};
    }

    auto pdu_type = data.look<2>(0);

    // See "Table 21.38: MAC PDU types for SCH/F, SCH/HD, STCH, SCH-P8/F, SCH-P8/HD, SCH-Q/D, SCH-Q/B and SCH-Q/U" on
    // how the this check works to filter out the u-plane signalling
    if (channel == LogicalChannel::kStealingChannel) {
        if (pdu_type == 0b11) {
            // process MAC-U-SIGNAL
            // this takes the complete stealing channel
            return UpperMacPackets{.u_plane_signalling_packet_ = {parse_u_plane_signalling(channel, std::move(data))}};
        }
        return UpperMacPackets{.c_plane_signalling_packets_ =
                                   parse_c_plane_signalling(burst_type, channel, std::move(data))};
    }

    if (pdu_type == 0b10) {
        // Broadcast
        // TMB-SAP
        if (is_downlink_burst(burst_type)) {
            return UpperMacPackets{.broadcast_packet_ = parse_broadcast(channel, std::move(data))};
        }
        throw std::runtime_error("Broadcast may only be sent on downlink.");
    }

    return UpperMacPackets{.c_plane_signalling_packets_ =
                               parse_c_plane_signalling(burst_type, channel, std::move(data))};
}

auto UpperMacPacketBuilder::parse_broadcast(LogicalChannel channel, BitVector&& data) -> UpperMacBroadcastPacket {
    UpperMacBroadcastPacket packet{.logical_channel_ = channel, .type_ = MacPacketType::kMacBroadcast};

    auto pdu_type = data.take<2>();
    auto broadcast_type = data.take<2>();

    if (broadcast_type == 0b00) {
        // SYSINFO PDU
        packet.sysinfo_ = SystemInfo(data);
    } else if (broadcast_type == 0b01) {
        // ACCESS-DEFINE PDU
        packet.access_define_ = AccessDefine(data);
    } else if (broadcast_type == 0b10) {
        throw std::runtime_error("SYSINFO-DA is not implemented.");
    } else {
        throw std::runtime_error("Reserved broadcast type");
    }

    if (data.bits_left() != 0) {
        std::cout << packet;
        std::cout << data.bits_left() << " bits left over." << std::endl;
        std::cout << data << std::endl;
        throw std::runtime_error("Bits left after parsing broadcast PDU.");
    }

    return packet;
}

auto UpperMacPacketBuilder::extract_tm_sdu(BitVector& data, std::size_t preprocessing_bit_count,
                                           unsigned _BitInt(1) fill_bit_indication, std::optional<std::size_t> length)
    -> BitVector {
    // 1. calculate the header size of the MAC. this step must be performed before removing fill bits, as this would
    // change the number of bits in the BitVector
    const auto mac_header_length = preprocessing_bit_count - data.bits_left();

    // 2. remove the fill bits
    if (fill_bit_indication == 0b1) {
        data.remove_fill_bits();
    }

    // 3. calculate the length either through the length indicated in the packet or the implicit length
    std::size_t payload_length = 0;
    if (length) {
        // there was a length indication in the packet. use it to calculate the payload size
        payload_length = *length - mac_header_length;
        // The fill bit indication shall indicate if there are any fill bits, which shall be added whenever the size of
        // the TM-SDU is less than the available capacity of the MAC block or less than the size of the TM-SDU indicated
        // by the length indication field. The TM-SDU length is equal to the MAC PDU length minus the MAC PDU header
        // length.
        if (payload_length > data.bits_left()) {
            // cap the number of bits left to the maximum available. this should only happen if the
            // tm_sdu size + mac header size is not alligned to octect boundary
            if (payload_length - data.bits_left() >= 8) {
                throw std::runtime_error(
                    "Fill bits were indicated and the length indication shows a size that does not fit "
                    "in the MAC, but the length indication is more than 7 bits apart.");
            }
            payload_length = data.bits_left();
        }
    } else {
        // the length is defined implicitly
        payload_length = data.bits_left();
    }

    return data.take_vector(payload_length);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto UpperMacPacketBuilder::parse_c_plane_signalling_packet(BurstType burst_type, LogicalChannel channel,
                                                            BitVector& data) -> UpperMacCPlaneSignallingPacket {
    auto preprocessing_bit_count = data.bits_left();

    if (channel == LogicalChannel::kSignallingChannelHalfUplink) {
        if (is_downlink_burst(burst_type)) {
            throw std::runtime_error("SignallingChannelHalfUplink may only be set on uplink.");
        }

        auto pdu_type = data.take<1>();
        auto fill_bit_indication = data.take<1>();

        if (pdu_type == 0b0) {
            // process MAC-ACCESS
            UpperMacCPlaneSignallingPacket packet{.logical_channel_ = channel, .type_ = MacPacketType::kMacAccess};

            packet.encrypted_ = (data.take<1>() == 1U);
            packet.address_ = Address::from_mac_access(data);

            auto optional_field_flag = data.take<1>();
            std::optional<unsigned _BitInt(5)> length_indication;
            std::optional<std::size_t> length;
            if (optional_field_flag == 0b1) {
                auto length_indication_or_capacity_request = data.take<1>();
                if (length_indication_or_capacity_request == 0b0) {
                    length_indication = data.take<5>();
                    length = LengthIndication::from_mac_access(*length_indication);
                } else {
                    packet.fragmentation_ = (data.take<1>() == 1U);
                    packet.reservation_requirement_ = data.take<4>();
                }
            }

            // Null PDU
            if (length_indication == 0b00000) {
                return packet;
            }

            packet.tm_sdu_ =
                UpperMacPacketBuilder::extract_tm_sdu(data, preprocessing_bit_count, fill_bit_indication, length);

            return packet;
        }

        {
            // process MAC-END-HU
            UpperMacCPlaneSignallingPacket packet{.logical_channel_ = channel, .type_ = MacPacketType::kMacEndHu};

            auto length_indictaion_or_capacity_request = data.take<1>();
            std::optional<unsigned _BitInt(4)> length_indication;
            std::optional<std::size_t> length;
            if (length_indictaion_or_capacity_request == 0b0) {
                length_indication = data.take<4>();
                length = LengthIndication::from_mac_end_hu(*length_indication);
            } else {
                packet.reservation_requirement_ = data.take<4>();
            }

            packet.tm_sdu_ =
                UpperMacPacketBuilder::extract_tm_sdu(data, preprocessing_bit_count, fill_bit_indication, length);

            return packet;
        }
    }

    if (is_uplink_burst(burst_type)) {
        // process the SCH/F and STCH of the uplink
        auto pdu_type = data.take<2>();

        if (pdu_type == 0b00) {
            // MAC-DATA (uplink)
            // TMA-SAP
            UpperMacCPlaneSignallingPacket packet{.logical_channel_ = channel, .type_ = MacPacketType::kMacData};

            auto fill_bit_indication = data.take<1>();

            packet.encrypted_ = (data.take<1>() == 1U);
            packet.address_ = Address::from_mac_data(data);

            auto length_indication_or_capacity_request = data.take<1>();
            std::optional<unsigned _BitInt(6)> length_indication;
            std::optional<std::size_t> length;
            if (length_indication_or_capacity_request == 0b0) {
                length_indication = data.take<6>();
                if (length_indication == 0b111111) {
                    packet.fragmentation_on_stealling_channel_ = true;
                }
                if (length_indication == 0b111111 || length_indication == 0b111110) {
                    // length is defined implicitly
                    length_indication.reset();
                } else {
                    length = LengthIndication::from_mac_data(*length_indication);
                }
            } else {
                packet.fragmentation_ = (data.take<1>() == 1U);
                packet.reservation_requirement_ = data.take<4>();
                auto reserved = data.take<1>();
            }

            // Null PDU
            if (length_indication == 0b000000) {
                return packet;
            }

            packet.tm_sdu_ =
                UpperMacPacketBuilder::extract_tm_sdu(data, preprocessing_bit_count, fill_bit_indication, length);

            return packet;
        }

        if (pdu_type == 0b01) {
            // MAC-END or MAC-FRAG
            // TMA-SAP
            auto subtype = data.take<1>();
            if (subtype == 0b0) {
                if (channel == LogicalChannel::kStealingChannel) {
                    throw std::runtime_error("MAC-FRAG may not be sent on stealing channel.");
                }

                UpperMacCPlaneSignallingPacket packet{.logical_channel_ = channel,
                                                      .type_ = MacPacketType::kMacFragmentUplink};

                auto fill_bit_indication = data.take<1>();
                packet.tm_sdu_ =
                    UpperMacPacketBuilder::extract_tm_sdu(data, preprocessing_bit_count, fill_bit_indication);

                return packet;
            }

            {
                UpperMacCPlaneSignallingPacket packet{.logical_channel_ = channel,
                                                      .type_ = MacPacketType::kMacEndUplink};

                auto fill_bit_indication = data.take<1>();

                auto length_indictaion_or_reservation_requirement = data.take<6>();
                std::optional<std::size_t> length;

                if (length_indictaion_or_reservation_requirement >= 0b110000) {
                    // reservation requirement
                    packet.reservation_requirement_ = length_indictaion_or_reservation_requirement & 0x0f;
                } else {
                    // length indication
                    length = LengthIndication::from_mac_end_uplink(length_indictaion_or_reservation_requirement);
                }

                packet.tm_sdu_ =
                    UpperMacPacketBuilder::extract_tm_sdu(data, preprocessing_bit_count, fill_bit_indication, length);

                return packet;
            }
        }

        if (pdu_type == 0b10) {
            throw std::runtime_error("Broadcast PDU should not be handled in parseCPlaneSignallingPacket function!");
        }

        {
            // Supplementary MAC PDU (not on STCH, SCH/HD or SCH-P8/HD)
            auto subtype = data.take<1>();

            if (subtype == 0b1) {
                throw std::runtime_error("Supplementary MAC PDU subtype 0b1 is reserved.");
            }

            if (channel != LogicalChannel::kSignallingChannelFull) {
                throw std::runtime_error("MAC-U-BLCK may only be sent on SCH/F.");
            }

            UpperMacCPlaneSignallingPacket packet{.logical_channel_ = channel, .type_ = MacPacketType::kMacUBlck};

            auto fill_bit_indication = data.take<1>();
            packet.encrypted_ = (data.take<1>() == 1U);
            auto event_label = data.take<10>();
            packet.address_.set_event_label(event_label);
            packet.reservation_requirement_ = data.take<4>();

            packet.tm_sdu_ = UpperMacPacketBuilder::extract_tm_sdu(data, preprocessing_bit_count, fill_bit_indication);

            return packet;
        }
    } else {
        // process SCH/F, SCH/HD and STCH on the downlink
        // process the SCH/F and STCH of the uplink
        auto pdu_type = data.take<2>();

        if (pdu_type == 0b00) {
            // MAC-RESOURCE (downlink)
            // TMA-SAP
            UpperMacCPlaneSignallingPacket packet{.logical_channel_ = channel, .type_ = MacPacketType::kMacResource};

            auto fill_bit_indication = data.take<1>();

            packet.position_of_grant_ = data.take<1>();

            auto encryption_mode = data.take<2>();
            if (encryption_mode > 0b00) {
                packet.encrypted_ = true;
                packet.encryption_mode_ = encryption_mode;
            }

            packet.random_access_flag_ = data.take<1>();

            auto length_indication = data.take<6>();
            std::optional<std::size_t> length;

            if (length_indication == 0b111111) {
                packet.fragmentation_ = true;
            }
            if (length_indication != 0b111111 && length_indication != 0b111110) {
                length = LengthIndication::from_mac_data(length_indication);
            }

            packet.address_ = Address::from_mac_resource(data);

            if (packet.address_ == Address{}) {
                // The null PDU, if it appears in a MAC block, shall always be the last PDU in that block. Any spare
                // capacity after the null PDU shall be filled with fill bits.
                data.remove_fill_bits();
                return packet;
            }

            // NOTE 3: The immediate napping permission flag shall be present when the PDU is sent using π/8-D8PSK
            // or QAM modulation. It shall not be present when the PDU is sent using π/4-DQPSK modulation.
            auto power_control_flag = data.take<1>();
            if (power_control_flag == 0b1) {
                packet.power_control_element_ = data.take<4>();
            }
            auto slot_granting_flag = data.take<1>();
            // The multiple slot granting flag shall be present when the slot granting
            // flag is set to 1 and the PDU is sent using QAM modulation. It shall not be
            // present when the slot granting flag is set to 0 or the PDU is sent using
            // π/4-DQPSK or π/8-D8PSK modulation auto multipleSlotGranting = vec.take<1>();
            // The basic slot granting element shall be present when the slot granting
            // flag is set to 1 and either the PDU is sent using π/4-DQPSK or π/8-D8PSK
            // modulation, or the PDU is sent using QAM modulation and the multiple slot
            // granting flag is set to 0.
            if (slot_granting_flag == 0b1) {
                packet.basic_slot_granting_element_ = data.take<8>();
            }
            auto channel_allocation_flag = data.take<1>();
            if (channel_allocation_flag == 0b1) {
                packet.channel_allocation_element_ = ChannelAllocationElement(data);
            }

            packet.tm_sdu_ =
                UpperMacPacketBuilder::extract_tm_sdu(data, preprocessing_bit_count, fill_bit_indication, length);

            return packet;
        }

        if (pdu_type == 0b01) {
            // MAC-END or MAC-FRAG
            // TMA-SAP
            auto subtype = data.take<1>();
            if (subtype == 0b0) {
                if (channel == LogicalChannel::kStealingChannel) {
                    throw std::runtime_error("MAC-FRAG may not be sent on stealing channel.");
                }

                UpperMacCPlaneSignallingPacket packet{.logical_channel_ = channel,
                                                      .type_ = MacPacketType::kMacFragmentDownlink};

                auto fill_bit_indication = data.take<1>();

                packet.tm_sdu_ =
                    UpperMacPacketBuilder::extract_tm_sdu(data, preprocessing_bit_count, fill_bit_indication);

                return packet;
            }

            {
                UpperMacCPlaneSignallingPacket packet{.logical_channel_ = channel,
                                                      .type_ = MacPacketType::kMacEndDownlink};

                auto fill_bit_indication = data.take<1>();

                packet.position_of_grant_ = data.take<1>();
                auto length_indication = data.take<6>();
                auto length = LengthIndication::from_mac_end_downlink(length_indication);
                // The immediate napping permission flag shall be present when the PDU is sent
                // using π/8-D8PSK or QAM modulation. It shall not be present when the PDU is
                // sent using π/4-DQPSK modulation. auto immediateNapping = vec.take<1>();
                auto slot_granting_flag = data.take<1>();
                // The multiple slot granting flag shall be present when the slot granting
                // flag is set to 1 and the PDU is sent using QAM modulation. It shall not be
                // present when the slot granting flag is set to 0 or the PDU is sent using
                // π/4-DQPSK or π/8-D8PSK modulation auto multipleSlotGranting = vec.take<1>();
                // The basic slot granting element shall be present when the slot granting
                // flag is set to 1 and either the PDU is sent using π/4-DQPSK or π/8-D8PSK
                // modulation, or the PDU is sent using QAM modulation and the multiple slot
                // granting flag is set to 0.
                if (slot_granting_flag == 0b1) {
                    packet.basic_slot_granting_element_ = data.take<8>();
                }
                auto channel_allocation_flag = data.take<1>();
                if (channel_allocation_flag == 0b1) {
                    packet.channel_allocation_element_ = ChannelAllocationElement(data);
                }

                packet.tm_sdu_ =
                    UpperMacPacketBuilder::extract_tm_sdu(data, preprocessing_bit_count, fill_bit_indication, length);

                return packet;
            }
        }

        if (pdu_type == 0b10) {
            throw std::runtime_error("Broadcast PDU should not be handled in parseCPlaneSignallingPacket function!");
        }

        {
            // Supplementary MAC PDU (not on STCH, SCH/HD or SCH-P8/HD)
            auto subtype = data.take<1>();

            if (subtype == 0b1) {
                throw std::runtime_error("Supplementary MAC PDU subtype 0b1 is reserved.");
            }

            if (channel != LogicalChannel::kSignallingChannelFull) {
                throw std::runtime_error("MAC-D-BLCK may only be sent on SCH/F.");
            }

            UpperMacCPlaneSignallingPacket packet{.logical_channel_ = channel, .type_ = MacPacketType::kMacDBlck};

            auto fill_bit_indication = data.take<1>();
            auto encryption_mode = data.take<2>();
            if (encryption_mode > 0b00) {
                packet.encrypted_ = true;
                packet.encryption_mode_ = encryption_mode;
            }
            auto event_label = data.take<10>();
            packet.address_.set_event_label(event_label);

            packet.immediate_napping_permission_flag_ = (data.take<1>() == 1U);
            auto slot_granting_flag = data.take<1>();
            if (slot_granting_flag == 0b1) {
                packet.basic_slot_granting_element_ = data.take<8>();
            }

            packet.tm_sdu_ = UpperMacPacketBuilder::extract_tm_sdu(data, preprocessing_bit_count, fill_bit_indication);

            return packet;
        }
    }
}

auto UpperMacPacketBuilder::parse_c_plane_signalling(const BurstType burst_type, const LogicalChannel channel,
                                                     BitVector&& data) -> std::vector<UpperMacCPlaneSignallingPacket> {

    std::vector<UpperMacCPlaneSignallingPacket> packets;

    // 23.4.3.3 PDU dissociation
    // If the remaining size in the MAC block is less than the length of the Null PDU, the MAC shall discard the
    // remaining bits.
    // NOTE 2: The size of the appropriate Null PDU (as used above) is 16 bits for the downlink, 36 bits for an uplink
    // subslot, or 37 bits for an uplink full slot or uplink STCH.
    std::size_t min_bit_count = 0;
    if (is_downlink_burst(burst_type)) {
        min_bit_count = 16;
    } else {
        if (channel == LogicalChannel::kSignallingChannelHalfUplink) {
            min_bit_count = 36;
        } else if (channel == LogicalChannel::kSignallingChannelFull || channel == LogicalChannel::kStealingChannel) {
            min_bit_count = 37;
        }
    }

    while (data.bits_left() >= min_bit_count) {
        if (data.is_mac_padding()) {
            std::cout << "Found padding, skipping: " << data << std::endl;
            break;
        }
        auto packet = parse_c_plane_signalling_packet(burst_type, channel, data);
        packets.emplace_back(std::move(packet));

        // The Null PDU indicates that there is no more useful data in this MAC block; after receipt of the Null PDU,
        // the MAC shall not look for further information in the block.
        if (packets.back().is_null_pdu()) {
            break;
        }
    }
    return packets;
}

auto UpperMacPacketBuilder::parse_u_plane_signalling(const LogicalChannel channel, BitVector&& data)
    -> UpperMacUPlaneSignallingPacket {
    // the only valid packet here is MAC-U-SIGNAL

    auto pdu_type = data.take<2>();
    auto second_slot_stolen = data.take<1>();

    if (pdu_type != 0b11) {
        throw std::runtime_error("UPlane Signalling Packet may only be MAC-U-SIGNAL");
    }

    return UpperMacUPlaneSignallingPacket{
        .logical_channel_ = channel, .type_ = MacPacketType::kMacUSignal, .tm_sdu_ = std::move(data)};
}

auto UpperMacPacketBuilder::parse_u_plane_traffic(const LogicalChannel channel, BitVector&& data)
    -> UpperMacUPlaneTrafficPacket {
    return UpperMacUPlaneTrafficPacket{.logical_channel_ = channel, .data_ = std::move(data)};
}