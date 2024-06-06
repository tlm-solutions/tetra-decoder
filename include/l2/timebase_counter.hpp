/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include <cstdint>
#include <ostream>
#include <tuple>

class TimebaseCounter {
  private:
    uint16_t time_slot_ = 1;
    uint16_t frame_number_ = 1;
    uint16_t multi_frame_number_ = 1;

  public:
    TimebaseCounter() = default;
    TimebaseCounter(const uint16_t time_slot, const uint16_t frame_number, const uint16_t multi_frame_number)
        : time_slot_(time_slot)
        , frame_number_(frame_number)
        , multi_frame_number_(multi_frame_number){};

    [[nodiscard]] auto time_slot() const noexcept -> uint16_t { return time_slot_; };
    [[nodiscard]] auto frame_number() const noexcept -> uint16_t { return frame_number_; };
    [[nodiscard]] auto multi_frame_number() const noexcept -> uint16_t { return multi_frame_number_; };
    /// convert the slot and frame numbers into a single value
    [[nodiscard]] auto count() const noexcept -> unsigned {
        return (time_slot_ - 1) + 4 * (frame_number_ - 1) + 4 * 18 * (multi_frame_number_ - 1);
    }

    [[nodiscard]] auto operator==(const TimebaseCounter& other) const noexcept -> bool {
        return std::tie(time_slot_, frame_number_, multi_frame_number_) ==
               std::tie(other.time_slot_, other.frame_number_, other.multi_frame_number_);
    };

    [[nodiscard]] auto operator!=(const TimebaseCounter& other) const noexcept -> bool { return !(*this == other); }

    auto increment() noexcept -> void;

    friend auto operator<<(std::ostream& stream, const TimebaseCounter& tc) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const TimebaseCounter& tc) -> std::ostream&;