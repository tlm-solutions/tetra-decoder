/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include <cstddef>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <tuple>
#include <variant>

namespace nlohmann {
template <typename... Ts> struct adl_serializer<std::variant<Ts...>> {
    static void to_json(json& j, const std::variant<Ts...>& var) {
        j = json::object();
        j["variant_id"] = var.index();
        std::visit([&j](auto&& arg) { j["variant"] = arg; }, var);
    }

    static void from_json(const json& j, std::variant<Ts...>& var) {
        std::size_t variant_id = j["variant_id"];
        std::size_t variant = j["variant"];

        if constexpr (sizeof...(Ts) > 0) {
            if (variant_id == 0) {
                var = std::variant<Ts...>(typename std::tuple_element<0, std::tuple<Ts...>>::type(variant));
            }
        }
        if constexpr (sizeof...(Ts) > 1) {
            if (variant_id == 1) {
                var = std::variant<Ts...>(typename std::tuple_element<1, std::tuple<Ts...>>::type(variant));
            }
        }
        if constexpr (sizeof...(Ts) > 2) {
            if (variant_id == 2) {
                var = std::variant<Ts...>(typename std::tuple_element<2, std::tuple<Ts...>>::type(variant));
            }
        }
        if constexpr (sizeof...(Ts) > 3) {
            if (variant_id == 3) {
                var = std::variant<Ts...>(typename std::tuple_element<3, std::tuple<Ts...>>::type(variant));
            }
        }
        if constexpr (sizeof...(Ts) > 4) {
            throw std::runtime_error("Cannot deserialize a std::variant with more than four variants.");
        }
    }
};
} // namespace nlohmann
