/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "borzoi/borzoi_packets.hpp"
#include "nlohmann/slots.hpp"
#include <nlohmann/json.hpp>

namespace nlohmann {
template <> struct adl_serializer<BorzoiSendTetraSlots> {
    static void to_json(json& j, const BorzoiSendTetraSlots& bsts) {
        j = json::object();
        j["time"] = bsts.time;
        j["station"] = bsts.station;
        adl_serializer<Slots>::to_json(j, bsts.slots);
    }
};
} // namespace nlohmann
