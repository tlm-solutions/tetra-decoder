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

    friend auto operator<<(std::ostream& stream, const UpperMacPackets& packets) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const UpperMacPackets& packets) -> std::ostream&;

class UpperMacPacketBuilder {
  private:
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