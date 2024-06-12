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
#include "utils/address_type.hpp"
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

auto UpperMacPacketBuilder::parseSlots(Slots& slots) -> UpperMacPackets {
    UpperMacPackets packets;

    {
        const auto& first_slot = slots.get_first_slot().get_logical_channel_data_and_crc();
        packets.merge(parseLogicalChannel(slots.get_burst_type(), first_slot));
    }

    if (slots.has_second_slot()) {
        const auto& second_slot = slots.get_second_slot().get_logical_channel_data_and_crc();
        packets.merge(parseLogicalChannel(slots.get_burst_type(), second_slot));
    }

    return packets;
}

auto UpperMacPacketBuilder::parseLogicalChannel(const BurstType burst_type,
                                                const LogicalChannelDataAndCrc& logical_channel_data_and_crc)
    -> UpperMacPackets {
    const auto& channel = logical_channel_data_and_crc.channel;
    auto data = BitVector(logical_channel_data_and_crc.data);
    if (channel == LogicalChannel::kTrafficChannel) {
        return UpperMacPackets{.u_plane_traffic_packet_ = parseUPlaneTraffic(channel, std::move(data))};
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
            return UpperMacPackets{.u_plane_signalling_packet_ = {parseUPlaneSignalling(channel, std::move(data))}};
        }
        return UpperMacPackets{.c_plane_signalling_packets_ =
                                   parseCPlaneSignalling(burst_type, channel, std::move(data))};
        // throw std::runtime_error("Only MAC-U-SIGNAL may be sent on the stealing channel.");
    }

    if (pdu_type == 0b10) {
        // Broadcast
        // TMB-SAP
        if (is_downlink_burst(burst_type)) {
            return UpperMacPackets{.broadcast_packet_ = parseBroadcast(channel, std::move(data))};
        }
        throw std::runtime_error("Broadcast may only be sent on downlink.");
    }

    return UpperMacPackets{.c_plane_signalling_packets_ = parseCPlaneSignalling(burst_type, channel, std::move(data))};
}

auto UpperMacPacketBuilder::parseBroadcast(LogicalChannel channel, BitVector&& data) -> UpperMacBroadcastPacket {
    UpperMacBroadcastPacket packet{.logical_channel_ = channel, .type_ = MacPacketType::kMacBroadcast};

    auto pdu_type = data.take<2>();
    auto broadcast_type = data.take<2>();

    if (broadcast_type == 0b00) {
        // SYSINFO PDU
        SystemInfo sysinfo;

        auto main_carrier = data.take<12>();
        auto frequency_band = data.take<4>();
        auto offset = data.take<2>();
        auto duplex_spacing_field = data.take<3>();
        auto reverse_operation = data.take<1>();
        sysinfo.number_secondary_control_channels_main_carrier_ = data.take<2>();
        sysinfo.ms_txpwr_max_cell_ = data.take<3>();
        sysinfo.rxlev_access_min_ = data.take<4>();
        sysinfo.access_parameter_ = data.take<4>();
        sysinfo.radio_downlink_timeout_ = data.take<4>();
        auto hyper_frame_cipher_key_flag = data.take<1>();
        if (hyper_frame_cipher_key_flag == 0) {
            sysinfo.hyper_frame_number_ = data.take<16>();
        } else {
            sysinfo.common_cipher_key_identifier_or_static_cipher_key_version_number_ = data.take<16>();
        }
        auto optional_field_flag = data.take<2>();
        if (optional_field_flag == 0b00) {
            sysinfo.even_multi_frame_definition_for_ts_mode_ = data.take<20>();
        } else if (optional_field_flag == 0b01) {
            sysinfo.odd_multi_frame_definition_for_ts_mode_ = data.take<20>();
        } else if (optional_field_flag == 0b10) {
            sysinfo.defaults_for_access_code_a_ = AccessCodeDefinition(data);
        } else if (optional_field_flag == 0b11) {
            ExtendedServiceBroadcast extended_service_broadcast;
            extended_service_broadcast.security_information_ = data.take<8>();
            extended_service_broadcast.sdstl_addressing_method_ = data.take<2>();
            extended_service_broadcast.gck_supported_ = data.take<1>();
            auto section = data.take<2>();
            if (section == 0b00) {
                ExtendedServiceBroadcastSection1 section;
                section.data_priority_supported_ = data.take<1>();
                section.extended_advanced_links_and_max_ublck_supported_ = data.take<1>();
                section.qos_negotiation_supported_ = data.take<1>();
                section.d8psk_service_ = data.take<1>();
                section.section2_sent_ = data.take<1>();
                section.section3_sent_ = data.take<1>();
                section.section4_sent_ = data.take<1>();
                extended_service_broadcast.section1_ = section;
            } else if (section == 0b01) {
                ExtendedServiceBroadcastSection2 section;
                section.service_25Qam_ = data.take<1>();
                section.service_50Qam_ = data.take<1>();
                section.service_100Qam_ = data.take<1>();
                section.service_150Qam_ = data.take<1>();
                section.reserved_ = data.take<3>();
                extended_service_broadcast.section2_ = section;
            } else if (section == 0b10) {
                ExtendedServiceBroadcastSection3 section;
                section.reserved_ = data.take<3>();
                extended_service_broadcast.section3_ = section;
            } else {
                ExtendedServiceBroadcastSection4 section;
                section.reserved_ = data.take<3>();
                extended_service_broadcast.section4_ = section;
            }
            sysinfo.extended_service_broadcast_ = extended_service_broadcast;
        }

        // downlink main carrier frequency = base frequency + (main carrier × 25 kHz)
        // + offset kHz.
        const int32_t duplex[4] = {0, 6250, -6250, 12500};
        sysinfo.downlink_frequency_ = frequency_band * 100000000 + main_carrier * 25000 + duplex[offset];

        static const int32_t tetra_duplex_spacing[8][16] = {
            /* values are in kHz */
            [0] = {-1, 1600, 10000, 10000, 10000, 10000, 10000, -1, -1, -1, -1, -1, -1, -1, -1, -1},
            [1] = {-1, 4500, -1, 36000, 7000, -1, -1, -1, 45000, 45000, -1, -1, -1, -1, -1, -1},
            [2] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            [3] = {-1, -1, -1, 8000, 8000, -1, -1, -1, 18000, 18000, -1, -1, -1, -1, -1, -1},
            [4] = {-1, -1, -1, 18000, 5000, -1, 30000, 30000, -1, 39000, -1, -1, -1, -1, -1, -1},
            [5] = {-1, -1, -1, -1, 9500, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
            [6] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
            [7] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        };
        const uint32_t duplex_spacing = tetra_duplex_spacing[duplex_spacing_field][frequency_band];
        /* reserved for future standardization */
        if (duplex_spacing < 0) {
            sysinfo.uplink_frequency_ = 0;
        } else if (reverse_operation) {
            sysinfo.uplink_frequency_ = static_cast<int32_t>(sysinfo.downlink_frequency_) + duplex_spacing * 1000;
        } else {
            sysinfo.uplink_frequency_ = static_cast<int32_t>(sysinfo.downlink_frequency_) - duplex_spacing * 1000;
        }

        // This element shall be present when the PDU is sent using π/8-D8PSK
        // modulation. This element shall not be present when the PDU is sent using
        // π/4-DQPSK modulation.
        // auto reserved = vec.take<28>();

        // Location area (14)
        sysinfo.location_area_ = data.take<14>();
        // Subscriber class (16)
        sysinfo.subscriber_class_ = data.take<16>();
        // BS service details (12)
        sysinfo.registration_ = data.take<1>();
        sysinfo.deregistration_ = data.take<1>();
        sysinfo.priority_cell_ = data.take<1>();
        sysinfo.minimum_mode_service_ = data.take<1>();
        sysinfo.migration_ = data.take<1>();
        sysinfo.system_wide_service_ = data.take<1>();
        sysinfo.tetra_voice_service_ = data.take<1>();
        sysinfo.circuit_mode_data_service_ = data.take<1>();
        auto reserved = data.take<1>();
        sysinfo.sndcp_service_ = data.take<1>();
        sysinfo.air_interface_encryption_service_ = data.take<1>();
        sysinfo.advanced_link_supported_ = data.take<1>();

        packet.sysinfo_ = sysinfo;
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
        throw std::runtime_error("Reseved broadcast PDU but there are bits left after parsing.");
    }

    return packet;
}

auto UpperMacPacketBuilder::parseCPlaneSignallingPacket(BurstType burst_type, LogicalChannel channel, BitVector& data)
    -> UpperMacCPlaneSignallingPacket {
    if (channel == LogicalChannel::kSignalingChannelHalfUplink) {
        if (is_downlink_burst(burst_type)) {
            throw std::runtime_error("SignalingChannelHalfUplink may only be set on uplink.");
        }

        auto preprocessing_bit_count = data.bits_left();

        auto pdu_type = data.take<1>();
        auto fill_bit_indication = data.take<1>();

        if (pdu_type == 0b0) {
            // process MAC-ACCESS
            UpperMacCPlaneSignallingPacket packet{.logical_channel_ = channel, .type_ = MacPacketType::kMacAccess};

            packet.encrypted_ = (data.take<1>() == 1U);
            auto address_type = data.take<2>();

            auto& address = packet.address_;
            if (address_type == 0b00) {
                address.set_ssi(data.take<24>());
            } else if (address_type == 0b01) {
                address.set_event_label(data.take<10>());
            } else if (address_type == 0b11) {
                address.set_ussi(data.take<24>());
            } else if (address_type == 0b11) {
                address.set_smi(data.take<24>());
            }

            auto optional_field_flag = data.take<1>();
            std::optional<unsigned _BitInt(5)> length_indication;
            if (optional_field_flag == 0b1) {
                auto length_indication_or_capacity_request = data.take<1>();
                if (length_indication_or_capacity_request == 0b0) {
                    length_indication = data.take<5>();
                } else {
                    packet.fragmentation_ = (data.take<1>() == 1U);
                    packet.reservation_requirement_ = data.take<4>();
                }
            }

            auto bits_left = data.bits_left();
            const auto mac_header_length = preprocessing_bit_count - bits_left;

            if (fill_bit_indication == 0b1) {
                data.remove_fill_bits();
            }

            if (length_indication.has_value()) {
                if (length_indication == 0b00000) {
                    bits_left = 0;
                } else {
                    bits_left = LengthIndication::from_mac_access(*length_indication) - mac_header_length;
                    if (fill_bit_indication == 0b1) {
                        // The fill bit indication shall indicate if there are any fill bits, which shall be added
                        // whenever the
                        // size of the TM-SDU is less than the available capacity of the MAC block or less than the size
                        // of the TM-SDU indicated by the length indication field. The TM-SDU length is equal to the MAC
                        // PDU length minus the MAC PDU header length.
                        if (bits_left > data.bits_left()) {
                            // cap the number of bits left to the maximum available. this should only happen if the
                            // tm_sdu size + mac header size is not alligned to octect boundary
                            if (bits_left - data.bits_left() >= 8) {
                                throw std::runtime_error(
                                    "Fill bits were indicated and the length indication shows a size that does not fit "
                                    "in the MAC, but the length indication is more than 7 bits apart.");
                            }
                            bits_left = data.bits_left();
                        }
                    }
                }
            } else {
                bits_left = data.bits_left();
            }

            if (bits_left != 0) {
                packet.tm_sdu_ = data.take_vector(bits_left);
            }

            return packet;
        }

        {
            // process MAC-END-HU
            UpperMacCPlaneSignallingPacket packet{.logical_channel_ = channel, .type_ = MacPacketType::kMacEndHu};

            auto length_indictaion_or_capacity_request = data.take<1>();
            std::optional<unsigned _BitInt(4)> length_indication;
            if (length_indictaion_or_capacity_request == 0b0) {
                length_indication = data.take<4>();
            } else {
                packet.reservation_requirement_ = data.take<4>();
            }

            auto bits_left = data.bits_left();
            const auto mac_header_length = preprocessing_bit_count - bits_left;

            if (length_indication.has_value()) {
                bits_left = LengthIndication::from_mac_end_hu(*length_indication) - mac_header_length;
                if (fill_bit_indication == 0b1) {
                    // The fill bit indication shall indicate if there are any fill bits, which shall be added
                    // whenever the
                    // size of the TM-SDU is less than the available capacity of the MAC block or less than the size
                    // of the TM-SDU indicated by the length indication field. The TM-SDU length is equal to the MAC
                    // PDU length minus the MAC PDU header length.
                    if (bits_left > data.bits_left()) {
                        // cap the number of bits left to the maximum available. this should only happen if the
                        // tm_sdu size + mac header size is not alligned to octect boundary
                        if (bits_left - data.bits_left() >= 8) {
                            throw std::runtime_error(
                                "Fill bits were indicated and the length indication shows a size that does not fit "
                                "in the MAC, but the length indication is more than 7 bits apart.");
                        }
                        bits_left = data.bits_left();
                    }
                }
            } else {
                bits_left = data.bits_left();
            }

            packet.tm_sdu_ = data.take_vector(bits_left);

            return packet;
        }
    }

    if (is_uplink_burst(burst_type)) {
        // process the SCH/F and STCH of the uplink
        auto preprocessing_bit_count = data.bits_left();

        auto pdu_type = data.take<2>();

        if (pdu_type == 0b00) {
            // MAC-DATA (uplink)
            // TMA-SAP
            UpperMacCPlaneSignallingPacket packet{.logical_channel_ = channel, .type_ = MacPacketType::kMacData};

            auto fill_bit_indication = data.take<1>();

            packet.encrypted_ = (data.take<1>() == 1U);
            auto address_type = data.take<2>();

            auto& address = packet.address_;
            if (address_type == 0b00) {
                address.set_ssi(data.take<24>());
            } else if (address_type == 0b01) {
                address.set_event_label(data.take<10>());
            } else if (address_type == 0b11) {
                address.set_ussi(data.take<24>());
            } else if (address_type == 0b11) {
                address.set_smi(data.take<24>());
            }

            auto length_indication_or_capacity_request = data.take<1>();
            std::optional<unsigned _BitInt(6)> length_indication;
            if (length_indication_or_capacity_request == 0b0) {
                length_indication = data.take<6>();
                if (length_indication == 0b111111) {
                    packet.fragmentation_on_stealling_channel_ = true;
                }
            } else {
                packet.fragmentation_ = (data.take<1>() == 1U);
                packet.reservation_requirement_ = data.take<4>();
                auto reserved = data.take<1>();
            }

            auto bits_left = data.bits_left();
            const auto mac_header_length = preprocessing_bit_count - bits_left;

            if (fill_bit_indication == 0b1) {
                data.remove_fill_bits();
            }

            if (length_indication.has_value()) {
                if (length_indication == 0b000000) {
                    bits_left = 0;
                } else if (length_indication == 0b111111 || length_indication == 0b111110) {
                    bits_left = data.bits_left();
                } else {
                    bits_left = LengthIndication::from_mac_data(*length_indication) - mac_header_length;
                    if (fill_bit_indication == 0b1) {
                        // The fill bit indication shall indicate if there are any fill bits, which shall be added
                        // whenever the
                        // size of the TM-SDU is less than the available capacity of the MAC block or less than the size
                        // of the TM-SDU indicated by the length indication field. The TM-SDU length is equal to the MAC
                        // PDU length minus the MAC PDU header length.
                        if (bits_left > data.bits_left()) {
                            // cap the number of bits left to the maximum available. this should only happen if the
                            // tm_sdu size + mac header size is not alligned to octect boundary
                            if (bits_left - data.bits_left() >= 8) {
                                throw std::runtime_error(
                                    "Fill bits were indicated and the length indication shows a size that does not fit "
                                    "in the MAC, but the length indication is more than 7 bits apart.");
                            }
                            bits_left = data.bits_left();
                        }
                    }
                }
            } else {
                bits_left = data.bits_left();
            }

            if (bits_left != 0) {
                packet.tm_sdu_ = data.take_vector(bits_left);
            }

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
                if (fill_bit_indication == 0b1) {
                    data.remove_fill_bits();
                }

                packet.tm_sdu_ = data.take_vector(data.bits_left());

                return packet;
            }

            {
                UpperMacCPlaneSignallingPacket packet{.logical_channel_ = channel,
                                                      .type_ = MacPacketType::kMacEndUplink};

                auto fill_bit_indication = data.take<1>();

                auto length_indictaion_or_reservation_requirement = data.take<6>();

                auto bits_left = data.bits_left();
                const auto mac_header_length = preprocessing_bit_count - bits_left;

                if (fill_bit_indication == 0b1) {
                    data.remove_fill_bits();
                }

                if (length_indictaion_or_reservation_requirement >= 0b110000) {
                    // reservation requirement
                    packet.reservation_requirement_ = length_indictaion_or_reservation_requirement & 0x0f;
                    bits_left = data.bits_left();
                } else {
                    // length indication
                    bits_left = LengthIndication::from_mac_end_uplink(length_indictaion_or_reservation_requirement) -
                                mac_header_length;
                    if (fill_bit_indication == 0b1) {
                        // The fill bit indication shall indicate if there are any fill bits, which shall be added
                        // whenever the
                        // size of the TM-SDU is less than the available capacity of the MAC block or less than the size
                        // of the TM-SDU indicated by the length indication field. The TM-SDU length is equal to the MAC
                        // PDU length minus the MAC PDU header length.
                        if (bits_left > data.bits_left()) {
                            // cap the number of bits left to the maximum available. this should only happen if the
                            // tm_sdu size + mac header size is not alligned to octect boundary
                            if (bits_left - data.bits_left() >= 8) {
                                throw std::runtime_error(
                                    "Fill bits were indicated and the length indication shows a size that does not fit "
                                    "in the MAC, but the length indication is more than 7 bits apart.");
                            }
                            bits_left = data.bits_left();
                        }
                    }
                }

                packet.tm_sdu_ = data.take_vector(bits_left);

                return packet;
            }
        }

        if (pdu_type == 0b10) {
            throw std::runtime_error("Broadcast PDU should not be handled in parseCPlaneSignallingPacket function!");
        }

        if (pdu_type == 0b11) {
            // Supplementary MAC PDU (not on STCH, SCH/HD or SCH-P8/HD)
            auto subtype = data.take<1>();

            if (subtype == 0b1) {
                throw std::runtime_error("Supplementary MAC PDU subtype 0b1 is reserved.");
            }

            if (channel != LogicalChannel::kSignalingChannelFull) {
                throw std::runtime_error("MAC-U-BLCK may only be sent on SCH/F.");
            }

            UpperMacCPlaneSignallingPacket packet{.logical_channel_ = channel, .type_ = MacPacketType::kMacUBlck};

            auto fill_bit_indication = data.take<1>();
            if (fill_bit_indication == 0b1) {
                data.remove_fill_bits();
            }

            packet.encrypted_ = (data.take<1>() == 1U);
            auto event_label = data.take<10>();
            packet.address_.set_event_label(event_label);
            packet.reservation_requirement_ = data.take<4>();

            return packet;
        }
    } else {
        // process SCH/F, SCH/HD and STCH on the downlink
        // process the SCH/F and STCH of the uplink
        const auto preprocessing_bit_count = data.bits_left();

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

            if (length_indication == 0b111111) {
                packet.fragmentation_ = true;
            }

            auto address_type = data.take<3>();
            auto& address = packet.address_;
            if (address_type == 0b001) {
                address.set_ssi(data.take<24>());
            } else if (address_type == 0b010) {
                address.set_event_label(data.take<10>());
            } else if (address_type == 0b011) {
                address.set_ussi(data.take<24>());
            } else if (address_type == 0b100) {
                address.set_smi(data.take<24>());
            } else if (address_type == 0b101) {
                address.set_ssi(data.take<24>());
                address.set_event_label(data.take<10>());
            } else if (address_type == 0b110) {
                address.set_ssi(data.take<24>());
                address.set_usage_marker(data.take<6>());
            } else if (address_type == 0b111) {
                address.set_smi(data.take<24>());
                address.set_event_label(data.take<10>());
            }

            if (address_type != 0b000) {
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

                auto bits_left = data.bits_left();
                const auto mac_header_length = preprocessing_bit_count - bits_left;

                if (fill_bit_indication == 0b1) {
                    data.remove_fill_bits();
                }

                if (length_indication < 0b111110) {
                    bits_left = LengthIndication::from_mac_resource(length_indication) - mac_header_length;
                    if (fill_bit_indication == 0b1) {
                        // The fill bit indication shall indicate if there are any fill bits, which shall be added
                        // whenever the
                        // size of the TM-SDU is less than the available capacity of the MAC block or less than the size
                        // of the TM-SDU indicated by the length indication field. The TM-SDU length is equal to the MAC
                        // PDU length minus the MAC PDU header length.
                        if (bits_left > data.bits_left()) {
                            // cap the number of bits left to the maximum available. this should only happen if the
                            // tm_sdu size + mac header size is not alligned to octect boundary
                            if (bits_left - data.bits_left() >= 8) {
                                throw std::runtime_error(
                                    "Fill bits were indicated and the length indication shows a size that does not fit "
                                    "in the MAC, but the length indication is more than 7 bits apart.");
                            }
                            bits_left = data.bits_left();
                        }
                    }
                } else {
                    bits_left = data.bits_left();
                }

                packet.tm_sdu_ = data.take_vector(bits_left);
            } else {
                // The null PDU, if it appears in a MAC block, shall always be the last PDU in that block. Any spare
                // capacity after the null PDU shall be filled with fill bits.
                data.remove_fill_bits();
            }

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
                if (fill_bit_indication == 0b1) {
                    data.remove_fill_bits();
                }

                packet.tm_sdu_ = data.take_vector(data.bits_left());

                return packet;
            }

            {
                UpperMacCPlaneSignallingPacket packet{.logical_channel_ = channel,
                                                      .type_ = MacPacketType::kMacEndDownlink};

                auto fill_bit_indication = data.take<1>();

                packet.position_of_grant_ = data.take<1>();
                auto length_indication = data.take<6>();
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

                auto mac_header_length = preprocessing_bit_count - data.bits_left();
                auto bits_left = LengthIndication::from_mac_end_downlink(length_indication) - mac_header_length;

                // The fill bit indication shall indicate if there are any fill bits, which shall be added whenever the
                // size of the TM-SDU is less than the available capacity of the MAC block or less than the size of the
                // TM-SDU indicated by the length indication field. The TM-SDU length is equal to the MAC PDU length
                // minus the MAC PDU header length.
                if (fill_bit_indication == 0b1) {
                    data.remove_fill_bits();
                    if (bits_left > data.bits_left()) {
                        // cap the number of bits left to the maximum available. this should only happen if the tm_sdu
                        // size + mac header size is not alligned to octect boundary
                        if (bits_left - data.bits_left() >= 8) {
                            throw std::runtime_error(
                                "Fill bits were indicated and the length indication shows a size that does not fit "
                                "in the MAC, but the length indication is more than 7 bits apart.");
                        }
                        bits_left = data.bits_left();
                    }
                }

                packet.tm_sdu_ = data.take_vector(bits_left);

                return packet;
            }
        }

        if (pdu_type == 0b10) {
            throw std::runtime_error("Broadcast PDU should not be handled in parseCPlaneSignallingPacket function!");
        }

        if (pdu_type == 0b11) {
            // Supplementary MAC PDU (not on STCH, SCH/HD or SCH-P8/HD)
            auto subtype = data.take<1>();

            if (subtype == 0b1) {
                throw std::runtime_error("Supplementary MAC PDU subtype 0b1 is reserved.");
            }

            if (channel != LogicalChannel::kSignalingChannelFull) {
                throw std::runtime_error("MAC-D-BLCK may only be sent on SCH/F.");
            }

            UpperMacCPlaneSignallingPacket packet{.logical_channel_ = channel, .type_ = MacPacketType::kMacDBlck};

            auto fill_bit_indication = data.take<1>();
            if (fill_bit_indication == 0b1) {
                data.remove_fill_bits();
            }

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

            packet.tm_sdu_ = data.take_vector(data.bits_left());

            return packet;
        }
    }
}

auto UpperMacPacketBuilder::parseCPlaneSignalling(const BurstType burst_type, const LogicalChannel channel,
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
        if (channel == LogicalChannel::kSignalingChannelHalfUplink) {
            min_bit_count = 36;
        } else if (channel == LogicalChannel::kSignalingChannelFull || channel == LogicalChannel::kStealingChannel) {
            min_bit_count = 37;
        }
    }

    while (data.bits_left() >= min_bit_count) {
        if (data.is_mac_padding()) {
            std::cout << "Found padding, skipping: " << data << std::endl;
            break;
        }
        auto packet = parseCPlaneSignallingPacket(burst_type, channel, data);
        packets.emplace_back(std::move(packet));

        // The Null PDU indicates that there is no more useful data in this MAC block; after receipt of the Null PDU,
        // the MAC shall not look for further information in the block.
        if (packets.back().is_null_pdu()) {
            break;
        }
    }
    return packets;
}

auto UpperMacPacketBuilder::parseUPlaneSignalling(const LogicalChannel channel, BitVector&& data)
    -> UpperMacUPlaneSignallingPacket {
    // the only valid packet here is MAC-U-SIGNAL

    auto pdu_type = data.take<2>();
    auto second_slot_stolen = data.take<1>();

    if (pdu_type != 0b11) {
        throw std::runtime_error("UPlane Signalling Packet may only be MAC-U-SIGNAL");
    }

    return UpperMacUPlaneSignallingPacket{
        .logical_channel_ = channel, .type_ = MacPacketType::kMacUSignal, .tm_sdu = std::move(data)};
}

auto UpperMacPacketBuilder::parseUPlaneTraffic(const LogicalChannel channel, BitVector&& data)
    -> UpperMacUPlaneTrafficPacket {
    return UpperMacUPlaneTrafficPacket{.logical_channel_ = channel, .data = std::move(data)};
}