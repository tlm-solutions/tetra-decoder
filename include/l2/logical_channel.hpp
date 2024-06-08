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

struct LogicalChannelDataAndCrc {
    /// the logical channel
    LogicalChannel channel;
    /// the data on the logical channel
    BitVector data;
    /// true if the crc of the signaling channels is ok
    bool crc_ok;
};