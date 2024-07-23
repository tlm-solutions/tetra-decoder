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

    ShortLocationReport() = delete;
    explicit ShortLocationReport(BitVector& data);
};

auto operator<<(std::ostream& stream, const ShortLocationReport& slr) -> std::ostream&;

struct LocationInformationProtocol {
    unsigned _BitInt(2) pdu_type_;
    std::optional<ShortLocationReport> short_location_report_;

    LocationInformationProtocol() = delete;

    explicit LocationInformationProtocol(BitVector& data);
};

auto operator<<(std::ostream& stream, const LocationInformationProtocol& lip) -> std::ostream&;

struct ShortDataServicePacket : public CircuitModeControlEntityPacket {
    unsigned _BitInt(8) protocol_identifier_;
    std::optional<LocationInformationProtocol> location_information_protocol_;

    static constexpr const std::size_t kLocationInformationProtocolIdentifier = 0b00001010;

    ShortDataServicePacket() = delete;

    explicit ShortDataServicePacket(const CircuitModeControlEntityPacket& packet);
};

auto operator<<(std::ostream& stream, const ShortDataServicePacket& sds) -> std::ostream&;