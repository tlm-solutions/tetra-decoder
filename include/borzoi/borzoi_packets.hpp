/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l2/logical_link_control_packet.hpp"
#include "l2/slot.hpp"

struct BorzoiSendTetraPacket {
    std::string time;
    std::string station;
    const std::unique_ptr<LogicalLinkControlPacket>& packet;

    /// Construct a packet for Borzoi containing the parsed packet, the current time and the uuid of this instance of
    /// tetra decoder.
    BorzoiSendTetraPacket(const std::unique_ptr<LogicalLinkControlPacket>& packet, std::string borzoi_uuid);
};

struct BorzoiSendTetraSlots {
    std::string time;
    std::string station;
    const Slots& slots;

    /// Construct a packet for Borzoi containing the received slot, the current time and the uuid of this instance of
    /// tetra decoder.
    BorzoiSendTetraSlots(const Slots& slots, std::string borzoi_uuid);
};