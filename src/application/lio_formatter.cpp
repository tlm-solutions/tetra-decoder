/*
 * Copyright (C) 2026 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "application/lio_packet.hpp"
#include <bitset>

auto operator<<(std::ostream& stream, const LioHeader& header) -> std::ostream& {
    std::cout << "  time_delta: " << std::bitset<10>(header.time_delta_) << std::endl;
    std::cout << "  pattern_segment_index: " << std::bitset<16>(header.pattern_segment_index_) << std::endl;
    std::cout << "  distance_scale: " << std::bitset<1>(header.distance_scale_) << std::endl;
    std::cout << "  distance: " << std::bitset<12>(header.distance_) << std::endl;
    return stream;
}

auto operator<<(std::ostream& stream, const LioGpsPacket& packet) -> std::ostream& {
    std::cout << "  extended_area: " << std::bitset<1>(packet.extended_area_) << std::endl;
    std::cout << "  x: " << static_cast<unsigned>(packet.x_) << std::endl;
    std::cout << "  y: " << static_cast<unsigned>(packet.y_) << std::endl;
    std::cout << "  direction: " << std::bitset<3>(packet.direction_) << std::endl;
    std::cout << "  dgps: " << std::bitset<1>(packet.dgps_) << std::endl;
    std::cout << "  jam: " << std::bitset<2>(packet.jam_) << std::endl;
    std::cout << "  unknown0: " << std::bitset<1>(packet.unknown0_) << std::endl;
    std::cout << "  remaining bits: " << packet.remaining_ << std::endl;
    return stream;
}

auto operator<<(std::ostream& stream, const LioPacket& packet) -> std::ostream& {
    std::cout << "LIO" << std::endl;
    std::cout << "  protocol_identifier: " << std::bitset<8>(packet.lio_protocol_identifier_) << std::endl;
    std::cout << packet.header_;
    if (packet.gps_) {
        std::cout << *packet.gps_;
    }
    return stream;
}