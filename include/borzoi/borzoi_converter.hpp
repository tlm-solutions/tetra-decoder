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
#include <nlohmann/json_fwd.hpp>

struct BorzoiConverter {
    static constexpr const int kPacketApiVersion = 0;

    static auto to_json(const Slots& slots) -> nlohmann::json;

    static auto to_json(const std::unique_ptr<LogicalLinkControlPacket>& packet) -> nlohmann::json;
};
