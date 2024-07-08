/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "l3/short_data_service_packet.hpp"

static auto integer_to_double(uint32_t data, std::size_t bits, double multiplier) -> double {
    if (data & (1 << (bits - 1))) {
        data = ~data;
        data += 1;
        data &= (0xFFFFFFFF >> (32 - bits));

        return -1 * multiplier * data / static_cast<double>(1 << (bits - 1));
    }
    return multiplier * data / static_cast<double>(1 << (bits - 1));
}

static auto decode_longitude(unsigned _BitInt(25) v) -> double {
    return integer_to_double(static_cast<uint32_t>(v), 25, 180.0);
}

static auto decode_latitude(unsigned _BitInt(24) v) -> double {
    return integer_to_double(static_cast<uint32_t>(v), 24, 90.0);
}

static auto decode_position_error(unsigned _BitInt(3) v) -> std::string {
    static const std::array<std::string, 8> kPositionError = {"< 2 m",   "< 20 m",    "< 200 m",  "< 2 km",
                                                              "< 20 km", "<= 200 km", "> 200 km", "unknown"};
    return kPositionError.at(v);
}

static auto decode_horizontal_velocity(unsigned _BitInt(7) v) -> double {
    if (v == 127) {
        return -1.0;
    }
    const double C = 16.0;
    const double x = 0.038;
    const double A = 13.0;
    const double B = 0.0;

    return C * std::pow((1 + x), (v - A)) + B;
}

static auto decode_direction_of_travel(unsigned _BitInt(4) v) -> std::string {
    static const std::array<std::string, 16> kDirectionOfTravel = {
        "0 N",   "22.5 NNE",  "45 NE",  "67.5 ENE",  "90 E",  "112.5 ESE", "135 SE", "157.5 SSE",
        "180 S", "202.5 SSW", "225 SW", "247.5 WSW", "270 W", "292.5 WNW", "315 NW", "337.5 NNW"};
    return kDirectionOfTravel.at(v);
}

ShortLocationReport::ShortLocationReport(BitVector& data)
    : time_elapsed_(data.take<2>())
    , longitude_(decode_longitude(data.take<25>()))
    , latitude_(decode_latitude(data.take<24>()))
    , position_error_(decode_position_error(data.take<3>()))
    , horizontal_velocity_(decode_horizontal_velocity(data.take<7>()))
    , direction_of_travel_(decode_direction_of_travel(data.take<4>()))
    , type_of_addition_data_(data.take<1>())
    , additional_data_(data.take<8>()) {}

LocationInformationProtocol::LocationInformationProtocol(BitVector& data)
    : pdu_type_(data.take<2>()) {
    if (pdu_type_ == 0b00) {
        short_location_report_ = ShortLocationReport(data);
    }
}

ShortDataServicePacket::ShortDataServicePacket(const CircuitModeControlEntityPacket& packet)
    : CircuitModeControlEntityPacket(packet) {
    assert(sds_data_.has_value());
    auto data = BitVector(sds_data_->data_);

    protocol_identifier_ = data.take<8>();

    if (protocol_identifier_ == kLocationInformationProtocolIdentifier) {
        location_information_protocol_ = LocationInformationProtocol(data);
    }
}
