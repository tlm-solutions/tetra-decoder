/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#pragma once

#include <fmt/core.h>
#include <fmt/format.h>

class BurstType {
  public:
    enum Value {
        ControlUplinkBurst,
        NormalUplinkBurst,
        NormalUplinkBurstSplit,
        NormalDownlinkBurst,
        NormalDownlinkBurstSplit,
        SynchronizationBurst,
    };

    BurstType() = default;
    constexpr explicit BurstType(Value value)
        : value_(value) {}

    constexpr operator Value() const { return value_; }

    explicit operator bool() const = delete;
    constexpr bool operator==(BurstType burst_type) const { return value_ == burst_type; }
    constexpr bool operator!=(BurstType burst_type) const { return value_ != burst_type; }
    constexpr bool operator==(BurstType::Value burst_type) const { return value_ == burst_type; }
    constexpr bool operator!=(BurstType::Value burst_type) const { return value_ != burst_type; }

    [[nodiscard]] auto is_uplink_burst() const noexcept -> bool {
        return value_ == ControlUplinkBurst || value_ == NormalUplinkBurst || value_ == NormalUplinkBurstSplit;
    }

    [[nodiscard]] auto is_downlink_burst() const noexcept -> bool { return !is_uplink_burst(); }

  private:
    Value value_{};
};

template <> struct fmt::formatter<BurstType> : formatter<std::string_view> {
    auto format(BurstType burstType, format_context& ctx) const {
        std::string_view name = "unknown";

        switch (burstType) {
        case BurstType::ControlUplinkBurst:
            name = "ControlUplinkBurst";
            break;
        case BurstType::NormalUplinkBurst:
            name = "NormalUplinkBurst";
            break;
        case BurstType::NormalUplinkBurstSplit:
            name = "NormalUplinkBurstSplit";
            break;
        case BurstType::NormalDownlinkBurst:
            name = "NormalDownlinkBurst";
            break;
        case BurstType::NormalDownlinkBurstSplit:
            name = "NormalDownlinkBurstSplit";
            break;
        case BurstType::SynchronizationBurst:
            name = "SynchronizationBurst";
            break;
        }

        return formatter<std::string_view>::format(name, ctx);
    };
};