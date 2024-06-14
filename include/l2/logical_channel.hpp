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
    kSignallingChannelHalfDownlink,
    kSignallingChannelHalfUplink,
    kTrafficChannel,
    kSignallingChannelFull,
    kStealingChannel
};

constexpr auto to_string(LogicalChannel channel) noexcept -> const char* {
    switch (channel) {
    case LogicalChannel::kSignallingChannelHalfDownlink:
        return "SignallingChannelHalfDownlink";
    case LogicalChannel::kSignallingChannelHalfUplink:
        return "SignallingChannelHalfUplink";
    case LogicalChannel::kTrafficChannel:
        return "TrafficChannel";
    case LogicalChannel::kSignallingChannelFull:
        return "SignallingChannelFull";
    case LogicalChannel::kStealingChannel:
        return "StealingChannel";
    }
};

struct LogicalChannelDataAndCrc {
    /// the logical channel
    LogicalChannel channel;
    /// the data on the logical channel
    BitVector data;
    /// true if the crc of the signalling channels is ok
    bool crc_ok;
};