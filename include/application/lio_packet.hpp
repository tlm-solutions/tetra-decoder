/*
 * Copyright (C) 2026 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l3/short_data_service_packet.hpp"
#include "utils/bit_vector.hpp"
#include <optional>

struct LioHeader {
    // 10s time delta from schedule
    unsigned _BitInt(10) time_delta_{};
    unsigned _BitInt(16) pattern_segment_index_{};
    unsigned _BitInt(1) distance_scale_{};
    // distance in m oder 10m depending on the value of distance_scale_ 0 -> m, 1 -> 10m
    unsigned _BitInt(12) distance_{};

    LioHeader() = default;

    explicit LioHeader(BitVector& data);

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(LioHeader, time_delta_, pattern_segment_index_, distance_scale_, distance_)
};

auto operator<<(std::ostream& stream, const LioHeader& header) -> std::ostream&;

struct LioGpsPacket {
    unsigned _BitInt(1) extended_area_{};
    unsigned _BitInt(12) x_{};
    unsigned _BitInt(12) y_{};
    unsigned _BitInt(3) direction_{};
    unsigned _BitInt(1) dgps_{};
    unsigned _BitInt(2) jam_{};
    unsigned _BitInt(1) unknown0_{};
    BitVector remaining_;

    LioGpsPacket() = default;

    explicit LioGpsPacket(BitVector& data);

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(LioGpsPacket, extended_area_, x_, y_, direction_, dgps_, jam_)
};

auto operator<<(std::ostream& stream, const LioGpsPacket& packet) -> std::ostream&;

struct LioPacket : public ShortDataServicePacket {
    unsigned _BitInt(8) lio_protocol_identifier_{};
    LioHeader header_;
    std::optional<LioGpsPacket> gps_;

    static constexpr const std::size_t kLioUplinkGpsUpdate = 0xe3;

    LioPacket() = default;

    explicit LioPacket(const ShortDataServicePacket& sds);

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(LioPacket, burst_type_, logical_channel_, type_, encrypted_, address_,
                                   fragmentation_, fragmentation_on_stealling_channel_, reservation_requirement_,
                                   tm_sdu_, encryption_mode_, immediate_napping_permission_flag_,
                                   basic_slot_granting_element_, position_of_grant_, channel_allocation_element_,
                                   random_access_flag_, power_control_element_, basic_link_information_, tl_sdu_,
                                   mle_protocol_, sdu_, packet_type_, sds_data_, protocol_identifier_,
                                   simple_text_messaging_, location_information_protocol_, text_messaging_, sds_tl_,
                                   lio_protocol_identifier_, header_, gps_)
};

auto operator<<(std::ostream& stream, const LioPacket& packet) -> std::ostream&;