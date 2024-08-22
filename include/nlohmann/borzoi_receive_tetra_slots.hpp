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
template <> struct adl_serializer<BorzoiReceiveTetraSlots> {
    static void from_json(const json& j, BorzoiReceiveTetraSlots& brtp) {
        brtp.time = j["time"];
        brtp.station = j["station"];
        adl_serializer<Slots>::from_json(j, brtp.slots);
    }
};
} // namespace nlohmann
