/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "borzoi/borzoi_packets.hpp"
#include "nlohmann/std_unique_ptr_logical_link_control_packet.hpp"
#include <nlohmann/json.hpp>

namespace nlohmann {
template <> struct adl_serializer<BorzoiReceiveTetraPacket> {
    static void from_json(const json& j, BorzoiReceiveTetraPacket& brtp) {
        brtp.time = j["time"];
        brtp.station = j["station"];
        adl_serializer<std::unique_ptr<LogicalLinkControlPacket>>::from_json(j, brtp.packet);
    }
};
} // namespace nlohmann
