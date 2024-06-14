/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "utils/bit_vector.hpp"

enum class LogicalChannel {
    kSignalingChannelHalfDownlink,
    kSignalingChannelHalfUplink,
    kTrafficChannel,
    kSignalingChannelFull,
    kStealingChannel
};

constexpr auto to_string(LogicalChannel channel) noexcept -> const char* {
    switch (channel) {
    case LogicalChannel::kSignalingChannelHalfDownlink:
        return "SignalingChannelHalfDownlink";
    case LogicalChannel::kSignalingChannelHalfUplink:
        return "SignalingChannelHalfUplink";
    case LogicalChannel::kTrafficChannel:
        return "TrafficChannel";
    case LogicalChannel::kSignalingChannelFull:
        return "SignalingChannelFull";
    case LogicalChannel::kStealingChannel:
        return "StealingChannel";
    }
};

struct LogicalChannelDataAndCrc {
    /// the logical channel
    LogicalChannel channel;
    /// the data on the logical channel
    BitVector data;
    /// true if the crc of the signaling channels is ok
    bool crc_ok;
};