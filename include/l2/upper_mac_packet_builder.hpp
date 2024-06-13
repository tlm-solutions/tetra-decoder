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

struct UpperMacPackets {
    std::vector<UpperMacCPlaneSignallingPacket> c_plane_signalling_packets_;
    std::vector<UpperMacUPlaneSignallingPacket> u_plane_signalling_packet_;
    std::optional<UpperMacUPlaneTrafficPacket> u_plane_traffic_packet_;
    std::optional<UpperMacBroadcastPacket> broadcast_packet_;

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

    [[nodiscard]] auto has_user_or_control_plane_data() const -> bool {
        for (const auto& packet : c_plane_signalling_packets_) {
            if (packet.is_null_pdu()) {
                // filter out null pdus
                continue;
            }
            return true;
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

    [[nodiscard]] static auto parseLogicalChannel(BurstType burst_type,
                                                  const LogicalChannelDataAndCrc& logical_channel_data_and_crc)
        -> UpperMacPackets;

    [[nodiscard]] static auto parseBroadcast(LogicalChannel channel, BitVector&& data) -> UpperMacBroadcastPacket;
    [[nodiscard]] static auto parseCPlaneSignallingPacket(BurstType burst_type, LogicalChannel channel, BitVector& data)
        -> UpperMacCPlaneSignallingPacket;
    [[nodiscard]] static auto parseCPlaneSignalling(BurstType burst_type, LogicalChannel channel, BitVector&& data)
        -> std::vector<UpperMacCPlaneSignallingPacket>;
    [[nodiscard]] static auto parseUPlaneSignalling(LogicalChannel channel, BitVector&& data)
        -> UpperMacUPlaneSignallingPacket;
    [[nodiscard]] static auto parseUPlaneTraffic(LogicalChannel channel, BitVector&& data)
        -> UpperMacUPlaneTrafficPacket;

  public:
    UpperMacPacketBuilder() = default;

    // TODO: make this function take a const Slots reference
    static auto parseSlots(Slots& slots) -> UpperMacPackets;
};