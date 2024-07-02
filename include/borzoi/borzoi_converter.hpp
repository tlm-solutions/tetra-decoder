/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l2/slot.hpp"
#include "l3/short_data_service_packet.hpp"

struct BorzoiConverter {
    static auto to_json(ShortDataServicePacket* packet) -> nlohmann::json;
    static auto to_json(const Slots& slots) -> nlohmann::json;
};
