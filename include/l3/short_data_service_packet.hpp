/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l3/circuit_mode_control_entity_packet.hpp"
#include "utils/bit_vector.hpp"
#include <cstddef>
#include <optional>

static auto integer_to_double(uint32_t data, std::size_t bits, double multiplier) -> double {
    if (data & (1 << (bits - 1))) {
        data = ~data;
        data += 1;
        data &= (0xFFFFFFFF >> (32 - bits));

        return -1 * multiplier * data / static_cast<double>(1 << (bits - 1));
    } else {
        return multiplier * data / static_cast<double>(1 << (bits - 1));
    }
}

static auto decode_longitude(uint64_t v) -> double {
    assert(v < std::pow(2, 25));

    return integer_to_double(static_cast<uint32_t>(v), 25, 180.0);
}

static auto decode_latitude(uint64_t v) -> double {
    assert(v < std::pow(2, 24));

    return integer_to_double(static_cast<uint32_t>(v), 24, 90.0);
}

static auto decode_position_error(uint64_t v) -> std::string {
    assert(v < 8);

    const std::string position_error[] = {"< 2 m",   "< 20 m",    "< 200 m",  "< 2 km",
                                          "< 20 km", "<= 200 km", "> 200 km", "unknown"};

    return position_error[v];
}

static auto decode_horizontal_velocity(uint64_t v) -> double {
    assert(v < std::pow(2, 7));

    if (v == 127) {
        return -1.0;
    } else {
        const double C = 16.0;
        const double x = 0.038;
        const double A = 13.0;
        const double B = 0.0;

        return C * std::pow((1 + x), (v - A)) + B;
    }
}

static auto decode_direction_of_travel(uint64_t v) -> std::string {
    assert(v < std::pow(2, 4));

    const std::string direction_of_travel[] = {"0 N",    "22.5 NNE",  "45 NE",  "67.5 ENE",  "90 E",   "112.5 ESE",
                                               "135 SE", "157.5 SSE", "180 S",  "202.5 SSW", "225 SW", "247.5 WSW",
                                               "270 W",  "292.5 WNW", "315 NW", "337.5 NNW"};

    return direction_of_travel[v];
}

struct ShortLocationReport {
    unsigned _BitInt(2) time_elapsed_;
    double longitude_;
    double latitude_;
    std::string position_error_;
    double horizontal_velocity_;
    std::string direction_of_travel_;
    unsigned _BitInt(1) type_of_addition_data_;
    unsigned _BitInt(8) additional_data_;

    ShortLocationReport() = delete;
    explicit ShortLocationReport(BitVector& data)
        : time_elapsed_(data.take<2>())
        , longitude_(decode_longitude(data.take<25>()))
        , latitude_(decode_latitude(data.take<24>()))
        , position_error_(decode_position_error(data.take<3>()))
        , horizontal_velocity_(decode_horizontal_velocity(data.take<7>()))
        , direction_of_travel_(decode_direction_of_travel(data.take<4>()))
        , type_of_addition_data_(data.take<1>())
        , additional_data_(data.take<8>()){};
};

inline auto operator<<(std::ostream& stream, const ShortLocationReport& slr) -> std::ostream& {
    stream << "  Short Location Report" << std::endl;
    stream << "  Time elapsed: " << std::bitset<2>(slr.time_elapsed_) << " Lon: " << slr.longitude_
           << " Lat: " << slr.latitude_ << " Position error: " << slr.position_error_
           << " horizontal_velocity: " << slr.horizontal_velocity_
           << " direction_of_travel: " << slr.direction_of_travel_
           << " type_of_addition_data: " << std::bitset<1>(slr.additional_data_)
           << " additional_data: " << std::bitset<8>(slr.additional_data_) << std::endl;
    return stream;
}

struct LocationInformationProtocol {
    unsigned _BitInt(2) pdu_type_;
    std::optional<ShortLocationReport> short_location_report_;

    LocationInformationProtocol() = delete;

    explicit LocationInformationProtocol(BitVector& data)
        : pdu_type_(data.take<2>()) {
        if (pdu_type_ == 0b00) {
            short_location_report_ = ShortLocationReport(data);
        }
    };
};

inline auto operator<<(std::ostream& stream, const LocationInformationProtocol& lip) -> std::ostream& {
    if (lip.short_location_report_) {
        stream << *lip.short_location_report_ << std::endl;
    } else if (lip.pdu_type_ == 0b01) {
        stream << "Location protocol PDU with extension" << std::endl;
    } else {
        stream << "Reserved LIP" << std::endl;
    }
    return stream;
};

struct ShortDataServicePacket : public CircuitModeControlEntityPacket {
    unsigned _BitInt(8) protocol_identifier_;
    std::optional<LocationInformationProtocol> location_information_protocol_;

    static constexpr const std::size_t kLocationInformationProtocolIdentifier = 0b00001010;

    ShortDataServicePacket() = delete;

    explicit ShortDataServicePacket(const CircuitModeControlEntityPacket& packet)
        : CircuitModeControlEntityPacket(packet) {
        assert(sds_data_.has_value());
        auto data = BitVector(sds_data_->data_);

        protocol_identifier_ = data.take<8>();

        if (protocol_identifier_ == kLocationInformationProtocolIdentifier) {
            location_information_protocol_ = LocationInformationProtocol(data);
        }
    };
};

inline auto operator<<(std::ostream& stream, const ShortDataServicePacket& sds) -> std::ostream& {
    stream << "SDS " << std::bitset<8>(sds.protocol_identifier_) << std::endl;
    stream << "  L2 Address: " << sds.address_ << " SDS Address: " << sds.sds_data_->address_ << std::endl;
    if (sds.location_information_protocol_) {
        stream << *sds.location_information_protocol_;
    }
    stream << "  Data: " << sds.sds_data_->data_ << std::endl;
    stream << "  decoded: ";
    const auto len = sds.sds_data_->data_.bits_left();
    for (auto i = 8; i + 8 <= len; i += 8) {
        auto bits = sds.sds_data_->data_.look<8>(i);
        stream << " " << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(bits);
    }
    stream << std::endl;

    return stream;
}