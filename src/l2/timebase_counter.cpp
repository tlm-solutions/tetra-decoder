/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "l2/timebase_counter.hpp"

auto TimebaseCounter::increment() noexcept -> void {
    time_slot_++;

    // TODO: remove magic numbers
    // time slot
    if (time_slot_ > 4) {
        frame_number_++;
        time_slot_ = 1;
    }

    // frame number
    if (frame_number_ > 18) {
        multi_frame_number_++;
        frame_number_ = 1;
    }

    // multi-frame number
    if (multi_frame_number_ > 60) {
        multi_frame_number_ = 1;
    }
}

auto operator<<(std::ostream& stream, const TimebaseCounter& tc) -> std::ostream& {
    stream << "TN/FN/MN: " << std::to_string(tc.time_slot_) << "/" << std::to_string(tc.frame_number_) << "/"
           << std::to_string(tc.multi_frame_number_);
    return stream;
}