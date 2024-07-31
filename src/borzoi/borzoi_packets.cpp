/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "borzoi/borzoi_packets.hpp"
#include "utils/ostream_std_unique_ptr_logical_link_control_packet.hpp"

inline static auto get_time() -> std::string {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::stringstream ss;
    ss << std::put_time(&tm, "%FT%T%z");
    return ss.str();
}

BorzoiSendTetraPacket::BorzoiSendTetraPacket(const std::unique_ptr<LogicalLinkControlPacket>& packet,
                                             std::string borzoi_uuid)
    : station(std::move(borzoi_uuid))
    , packet(packet) {
    time = get_time();
}

BorzoiSendTetraSlots::BorzoiSendTetraSlots(const Slots& slots, std::string borzoi_uuid)
    : station(std::move(borzoi_uuid))
    , slots(slots) {
    time = get_time();
}

auto operator<<(std::ostream& stream, const BorzoiSendTetraPacket& packet) -> std::ostream& {
    stream << packet.time << std::endl;
    stream << packet.station << std::endl;
    stream << packet.packet << std::endl;
    return stream;
};

auto operator<<(std::ostream& stream, const BorzoiReceiveTetraPacket& packet) -> std::ostream& {
    stream << packet.time << std::endl;
    stream << packet.station << std::endl;
    stream << packet.packet << std::endl;
    return stream;
};

auto operator<<(std::ostream& stream, const BorzoiSendTetraSlots& packet) -> std::ostream& {
    stream << packet.time << std::endl;
    stream << packet.station << std::endl;
    stream << packet.slots << std::endl;
    return stream;
};

auto operator<<(std::ostream& stream, const BorzoiReceiveTetraSlots& packet) -> std::ostream& {
    stream << packet.time << std::endl;
    stream << packet.station << std::endl;
    stream << packet.slots << std::endl;
    return stream;
};