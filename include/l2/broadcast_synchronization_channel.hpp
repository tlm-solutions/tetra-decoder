/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "burst_type.hpp"
#include "l2/timebase_counter.hpp"
#include <cstdint>
#include <vector>

struct BroadcastSynchronizationChannel {
  public:
    uint8_t system_code = 0;
    uint32_t color_code = 0;
    TimebaseCounter time{};
    uint8_t sharing_mode = 0;
    uint8_t time_slot_reserved_frames = 0;
    uint8_t up_lane_dtx = 0;
    uint8_t frame_18_extension = 0;

    uint32_t scrambling_code = 0;

    uint32_t mobile_country_code = 0;
    uint32_t mobile_network_code = 0;
    uint8_t dNwrk_broadcast_broadcast_supported = 0;
    uint8_t dNwrk_broadcast_enquiry_supported = 0;
    uint8_t cell_load_ca = 0;
    uint8_t late_entry_supported = 0;

    BroadcastSynchronizationChannel() = default;
    BroadcastSynchronizationChannel(const BurstType burst_type, const std::vector<uint8_t>& data);

    friend auto operator<<(std::ostream& stream, const BroadcastSynchronizationChannel& bsc) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const BroadcastSynchronizationChannel& bsc) -> std::ostream&;