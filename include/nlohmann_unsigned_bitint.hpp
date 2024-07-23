/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include <nlohmann/json.hpp>

namespace nlohmann {
template <std::size_t T> struct adl_serializer<unsigned _BitInt(T)> {
    static void to_json(json& j, const unsigned _BitInt(T) & bit_int) { j = static_cast<unsigned>(bit_int); }

    static void from_json(const json& j, unsigned _BitInt(T) & bit_int) { bit_int = j.template get<unsigned>(); }
};
} // namespace nlohmann
