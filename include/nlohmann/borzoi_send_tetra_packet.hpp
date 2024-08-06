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
template <> struct adl_serializer<BorzoiSendTetraPacket> {
    static void to_json(json& j, const BorzoiSendTetraPacket& bstp) {
        j = json::object();
        j["time"] = bstp.time;
        j["station"] = bstp.station;
        adl_serializer<std::unique_ptr<LogicalLinkControlPacket>>::to_json(j, bstp.packet);
    }
};
} // namespace nlohmann
