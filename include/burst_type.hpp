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

enum class BurstType {
    ControlUplinkBurst,
    NormalUplinkBurst,
    NormalUplinkBurstSplit,
    NormalDownlinkBurst,
    NormalDownlinkBurstSplit,
    SynchronizationBurst,
};

[[nodiscard]] inline auto is_uplink_burst(BurstType burst_type) noexcept -> bool {
    return burst_type == BurstType::ControlUplinkBurst || burst_type == BurstType::NormalUplinkBurst ||
           burst_type == BurstType::NormalUplinkBurstSplit;
};

[[nodiscard]] inline auto is_downlink_burst(BurstType burst_type) noexcept -> bool {
    return !is_uplink_burst(burst_type);
}

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