/*
 * Copyright (C) 2026 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "application/lio_packet.hpp"
#include "utils/bit_vector.hpp"
#include <stdexcept>

LioHeader::LioHeader(BitVector& data) {
    unsigned _BitInt(40) bits = static_cast<unsigned _BitInt(40)>(data.take<8>()) |
                                (static_cast<unsigned _BitInt(40)>(data.take<8>()) << 8) |
                                (static_cast<unsigned _BitInt(40)>(data.take<8>()) << 16) |
                                (static_cast<unsigned _BitInt(40)>(data.take<8>()) << 24) |
                                (static_cast<unsigned _BitInt(40)>(data.take<8>()) << 32);

    time_delta_ = bits & 0x3ff;
    pattern_segment_index_ = (bits >> 10) & 0xffff;
    distance_scale_ = (bits >> 26) & 0x1;
    distance_ = (bits >> 27) & 0xfff;
}

LioGpsPacket::LioGpsPacket(BitVector& data) {
    unsigned _BitInt(32) bits = static_cast<unsigned _BitInt(32)>(data.take<8>()) |
                                (static_cast<unsigned _BitInt(32)>(data.take<8>()) << 8) |
                                (static_cast<unsigned _BitInt(32)>(data.take<8>()) << 16) |
                                (static_cast<unsigned _BitInt(32)>(data.take<8>()) << 24);

    extended_area_ = bits & 0x1;
    x_ = (bits >> 1) & 0xfff;
    y_ = (bits >> 13) & 0xfff;
    direction_ = (bits >> 25) & 0x7;
    dgps_ = (bits >> 28) & 0x1;
    jam_ = (bits >> 29) & 0x3;
    unknown0_ = (bits >> 31) & 0x1;
    remaining_ = BitVector(data);
}

LioPacket::LioPacket(const ShortDataServicePacket& sds)
    : ShortDataServicePacket(sds) {

    auto optional_message = sds.text_message();
    if (!optional_message) {
        throw std::runtime_error("Either Text message or Simple Text Message must be present for the LIO protocol.");
    }

    lio_protocol_identifier_ = optional_message->take<8>();
    header_ = LioHeader(*optional_message);

    if (lio_protocol_identifier_ == kLioUplinkGpsUpdate) {
        if (optional_message->bits_left() < 32) {
            throw std::runtime_error("LIO Gps protocol does not contain enough bits.");
        }
        gps_ = LioGpsPacket(*optional_message);
    }

    remaining_lio_bits_ = BitVector(*optional_message);
}