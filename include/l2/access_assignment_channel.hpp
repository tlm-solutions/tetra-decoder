/*
 * Copyright (C) 2024 Transit Live Mapping Solutionss
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "burst_type.hpp"
#include "l2/timebase_counter.hpp"
#include "utils/bit_vector.hpp"
#include <optional>
#include <ostream>

enum DownlinkUsage { CommonControl, Unallocated, AssignedControl, CommonAndAssignedControl, Traffic };

struct AccessAssignmentChannel {
    DownlinkUsage downlink_usage;
    std::optional<int> downlink_traffic_usage_marker;

    AccessAssignmentChannel() = delete;

    AccessAssignmentChannel(BurstType burst_type, const TimebaseCounter& time, BitVector&& vec);

    friend auto operator<<(std::ostream& stream, const AccessAssignmentChannel& aac) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const AccessAssignmentChannel& aac) -> std::ostream&;