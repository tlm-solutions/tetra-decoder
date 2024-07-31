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

    BorzoiSendTetraPacket() = delete;

    /// Construct a packet for Borzoi containing the parsed packet, the current time and the uuid of this instance of
    /// tetra decoder.
    BorzoiSendTetraPacket(const std::unique_ptr<LogicalLinkControlPacket>& packet, std::string borzoi_uuid);

    friend auto operator<<(std::ostream& stream, const BorzoiSendTetraPacket& packet) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const BorzoiSendTetraPacket& packet) -> std::ostream&;

struct BorzoiReceiveTetraPacket {
    std::string time;
    std::string station;
    std::unique_ptr<LogicalLinkControlPacket> packet;

    BorzoiReceiveTetraPacket() = default;

    friend auto operator<<(std::ostream& stream, const BorzoiReceiveTetraPacket& packet) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const BorzoiReceiveTetraPacket& packet) -> std::ostream&;

struct BorzoiSendTetraSlots {
    std::string time;
    std::string station;
    const Slots& slots;

    BorzoiSendTetraSlots() = delete;

    /// Construct a packet for Borzoi containing the received slot, the current time and the uuid of this instance of
    /// tetra decoder.
    BorzoiSendTetraSlots(const Slots& slots, std::string borzoi_uuid);

    friend auto operator<<(std::ostream& stream, const BorzoiSendTetraSlots& packet) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const BorzoiSendTetraSlots& packet) -> std::ostream&;

struct BorzoiReceiveTetraSlots {
    std::string time;
    std::string station;
    Slots slots;

    BorzoiReceiveTetraSlots() = default;

    friend auto operator<<(std::ostream& stream, const BorzoiReceiveTetraSlots& packet) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const BorzoiReceiveTetraSlots& packet) -> std::ostream&;