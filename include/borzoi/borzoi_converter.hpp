/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l2/slot.hpp"
#include <nlohmann/json_fwd.hpp>

struct BorzoiConverter {
    static auto to_json(const Slots& slots) -> nlohmann::json;
};
