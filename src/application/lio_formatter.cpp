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
    stream << "  time_delta: " << std::bitset<10>(header.time_delta_) << std::endl;
    stream << "  pattern_segment_index: " << std::bitset<16>(header.pattern_segment_index_) << std::endl;
    stream << "  distance_scale: " << std::bitset<1>(header.distance_scale_) << std::endl;
    stream << "  distance: " << std::bitset<12>(header.distance_) << std::endl;
    return stream;
}

auto operator<<(std::ostream& stream, const LioGpsPacket& packet) -> std::ostream& {
    stream << "  extended_area: " << std::bitset<1>(packet.extended_area_) << std::endl;
    stream << "  x: " << static_cast<unsigned>(packet.x_) << std::endl;
    stream << "  y: " << static_cast<unsigned>(packet.y_) << std::endl;
    stream << "  direction: " << std::bitset<3>(packet.direction_) << std::endl;
    stream << "  dgps: " << std::bitset<1>(packet.dgps_) << std::endl;
    stream << "  jam: " << std::bitset<2>(packet.jam_) << std::endl;
    stream << "  unknown0: " << std::bitset<1>(packet.unknown0_) << std::endl;
    stream << "  remaining bits: " << packet.remaining_ << std::endl;
    return stream;
}

auto operator<<(std::ostream& stream, const LioPacket& packet) -> std::ostream& {
    stream << "LIO" << std::endl;
    stream << "  header_indication: 0x" << std::hex << static_cast<unsigned>(packet.header_indication_) << std::dec
           << std::endl;
    stream << "  protocol_identifier: 0x" << std::hex << static_cast<unsigned>(packet.lio_protocol_identifier_)
           << std::dec << std::endl;
    stream << packet.header_;
    if (packet.gps_) {
        stream << *packet.gps_;
    }
    stream << packet.remaining_lio_bits_ << std::endl;
    return stream;
}