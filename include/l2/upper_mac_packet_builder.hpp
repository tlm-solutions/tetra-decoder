/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l2/slot.hpp"
#include "l2/upper_mac_packet.hpp"
#include "utils/bit_vector.hpp"
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <vector>

/// datastructure to hold the packets that are processed in the upper MAC
struct UpperMacPackets {
    /// the C-plane signalling packets
    std::vector<UpperMacCPlaneSignallingPacket> c_plane_signalling_packets_;
    /// the U-plane signalling packets
    std::vector<UpperMacUPlaneSignallingPacket> u_plane_signalling_packet_;
    /// an optional U-plane traffic packet
    std::optional<UpperMacUPlaneTrafficPacket> u_plane_traffic_packet_;
    /// an optional broadcast packet
    std::optional<UpperMacBroadcastPacket> broadcast_packet_;

    /// Merge two extracted UpperMacPackets into one. Thsi function is used to merge the extracted packets from two
    /// slots.
    /// \param other the UpperMacPackets which should be merged into the current one
    auto merge(UpperMacPackets&& other) -> void {
        std::move(other.c_plane_signalling_packets_.begin(), other.c_plane_signalling_packets_.end(),
                  std::back_inserter(c_plane_signalling_packets_));
        std::move(other.u_plane_signalling_packet_.begin(), other.u_plane_signalling_packet_.end(),
                  std::back_inserter(u_plane_signalling_packet_));

        if (u_plane_traffic_packet_ && other.u_plane_traffic_packet_) {
            throw std::runtime_error("Trying to merge two packets both with a UpperMacUPlaneTrafficPacket");
        }

        if (other.u_plane_traffic_packet_) {
            u_plane_traffic_packet_ = std::move(*other.u_plane_traffic_packet_);
        }

        if (broadcast_packet_ && other.broadcast_packet_) {
            throw std::runtime_error("Trying to merge two packets both with a UpperMacBroadcastPacket");
        }

        if (other.broadcast_packet_) {
            broadcast_packet_ = std::move(*other.broadcast_packet_);
        }
    }

    /// Distribute the information of the uplink c-plane signalling null pdu to all other c-plane signalling packets.
    /// This operation will associate uplink packets with no address (MacFragmentUplink and MacEndUplink) with the
    /// address of the uplink null pdu.
    auto apply_uplink_null_pdu_information() -> void {
        std::optional<UpperMacCPlaneSignallingPacket> null_pdu;

        for (auto const& packet : c_plane_signalling_packets_) {
            if (packet.is_null_pdu()) {
                null_pdu = packet;
            }
        }

        if (null_pdu) {
            for (auto& packet : c_plane_signalling_packets_) {
                if ((packet.type_ == MacPacketType::kMacFragmentUplink) ||
                    (packet.type_ == MacPacketType::kMacEndUplink)) {
                    packet.address_ = null_pdu->address_;
                }
            }
        }
    }

    /// Check if the packet contains data that is of importance for C-Plane or U-Plane
    /// \return true if the UpperMacPackets contain user or control plane data (either signalling or traffic)
    [[nodiscard]] auto has_user_or_control_plane_data() const -> bool {
        for (const auto& packet : c_plane_signalling_packets_) {
            // filter out null pdus
            if (!packet.is_null_pdu()) {
                return true;
            }
        }
        if (u_plane_signalling_packet_.size() > 0) {
            return true;
        }
        if (u_plane_traffic_packet_) {
            return true;
        }
        return false;
    };

    friend auto operator<<(std::ostream& stream, const UpperMacPackets& packets) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const UpperMacPackets& packets) -> std::ostream&;

class UpperMacPacketBuilder {
  private:
    /// Extract a TM-SDU for a MAC packet. It needs to know the size of the header, if there are fill bits. The length
    /// may be either defined implicitly or by a length indication.
    /// \param data the BitVector which holds the TM-SDU
    /// \param preprocessing_bit_count the number of bits int he BitVector before parsing the MAC PDU
    /// \param fill_bit_indication true if there are fill bits indicated
    /// \param length the length in bits of the payload if it is not defined implicitly
    /// \return the TM-SDU as a BitVector
    [[nodiscard]] static auto extract_tm_sdu(BitVector& data, std::size_t preprocessing_bit_count,
                                             unsigned _BitInt(1) fill_bit_indication,
                                             std::optional<std::size_t> length = std::nullopt) -> BitVector;

    /// Parse the broadcast packet contained in a BitVector
    /// \param channel the logical channel on which the broadcast packet is sent
    /// \param data the BitVector which holds the MAC broadcast packet
    /// \return the parsed broadcast packet
    [[nodiscard]] static auto parse_broadcast(LogicalChannel channel, BitVector&& data) -> UpperMacBroadcastPacket;

    /// Parse a control plane signalling packet (singular) contained in a BitVector
    /// \param burst_type which burst was used to send this packet
    /// \param channel the logical channel on which the packets are sent
    /// \param data the BitVector which holds the packet
    /// \return the parsed c-plane signalling packet
    [[nodiscard]] static auto parse_c_plane_signalling_packet(BurstType burst_type, LogicalChannel channel,
                                                              BitVector& data) -> UpperMacCPlaneSignallingPacket;

    /// Parse the control plane signalling packets contained in a BitVector
    /// \param burst_type which burst was used to send these packets
    /// \param channel the logical channel on which the packets are sent
    /// \param data the BitVector which holds the packets
    /// \return the parsed c-plane signalling packets
    [[nodiscard]] static auto parse_c_plane_signalling(BurstType burst_type, LogicalChannel channel, BitVector&& data)
        -> std::vector<UpperMacCPlaneSignallingPacket>;

    /// Parse the user plane signalling packet contained in a BitVector
    /// \param channel the logical channel on which the packet is sent
    /// \param data the BitVector which holds the packet
    /// \return the parsed u-plane signalling packet
    [[nodiscard]] static auto parse_u_plane_signalling(LogicalChannel channel, BitVector&& data)
        -> UpperMacUPlaneSignallingPacket;

    /// Parse the user plane traffic packet contained in a BitVector
    /// \param channel the logical channel on which the packet is sent
    /// \param data the BitVector which holds the packet
    /// \return the parsed u-plane traffic packet
    [[nodiscard]] static auto parse_u_plane_traffic(LogicalChannel channel, BitVector&& data)
        -> UpperMacUPlaneTrafficPacket;

  public:
    UpperMacPacketBuilder() = default;

    /// Parse a slot (singular) and extract all the contained packets
    /// \param burst_type which burst was used to send this slot
    /// \param slot the data passed from the lower mac for this slot
    /// \return the extracted packets
    [[nodiscard]] static auto parse_slot(const ConcreateSlot& slot) -> UpperMacPackets;
};