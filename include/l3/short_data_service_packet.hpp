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

struct ShortLocationReport {
    unsigned _BitInt(2) time_elapsed_;
    double longitude_;
    double latitude_;
    std::string position_error_;
    double horizontal_velocity_;
    std::string direction_of_travel_;
    unsigned _BitInt(1) type_of_addition_data_;
    unsigned _BitInt(8) additional_data_;

    ShortLocationReport() = default;
    explicit ShortLocationReport(BitVector& data);

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ShortLocationReport, time_elapsed_, longitude_, latitude_, position_error_,
                                   horizontal_velocity_, direction_of_travel_, type_of_addition_data_, additional_data_)
};

auto operator<<(std::ostream& stream, const ShortLocationReport& slr) -> std::ostream&;

struct LocationInformationProtocol {
    unsigned _BitInt(2) pdu_type_;
    std::optional<ShortLocationReport> short_location_report_;

    LocationInformationProtocol() = default;

    explicit LocationInformationProtocol(BitVector& data);

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(LocationInformationProtocol, pdu_type_, short_location_report_)
};

auto operator<<(std::ostream& stream, const LocationInformationProtocol& lip) -> std::ostream&;

struct ShortDataServicePacket : public CircuitModeControlEntityPacket {
    unsigned _BitInt(8) protocol_identifier_;
    std::optional<LocationInformationProtocol> location_information_protocol_;

    static constexpr const std::size_t kLocationInformationProtocolIdentifier = 0b00001010;

    ShortDataServicePacket() = default;

    explicit ShortDataServicePacket(const CircuitModeControlEntityPacket& packet);

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ShortDataServicePacket, protocol_identifier_, location_information_protocol_)
};

auto operator<<(std::ostream& stream, const ShortDataServicePacket& sds) -> std::ostream&;