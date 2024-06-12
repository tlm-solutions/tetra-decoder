/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include <bitset>
#include <iostream>
#include <optional>
#include <sstream>

#include <nlohmann/json.hpp>

class AddressType {
  public:
    AddressType() = default;

    // explicit operator bool() const = delete;
    constexpr auto operator==(AddressType address_type) const -> bool {
        return country_code_ == address_type.country_code_ && network_code_ == address_type.network_code_ &&
               sna_ == address_type.sna_ && ssi_ == address_type.ssi_ && event_label_ == address_type.event_label_ &&
               ussi_ == address_type.ussi_ && smi_ == address_type.smi_ && usage_marker_ == address_type.usage_marker_;
    }

    void set_country_code(uint64_t country_code) { country_code_ = country_code; }
    void set_network_code(uint64_t network_code) { network_code_ = network_code; }
    void set_sna(uint64_t sna) { sna_ = sna; }
    void set_ssi(uint64_t ssi) { ssi_ = ssi; }
    void set_event_label(uint64_t event_label) { event_label_ = event_label; }
    void set_ussi(uint64_t ussi) { ussi_ = ussi; }
    void set_smi(uint64_t smi) { smi_ = smi; }
    void set_usage_marker(uint64_t usage_marker) { usage_marker_ = usage_marker; }

    [[nodiscard]] auto country_code() const noexcept { return country_code_; }
    [[nodiscard]] auto network_code() const noexcept { return network_code_; }
    [[nodiscard]] auto sna() const noexcept { return sna_; }
    [[nodiscard]] auto ssi() const noexcept { return ssi_; }
    [[nodiscard]] auto event_label() const noexcept { return event_label_; }
    [[nodiscard]] auto ussi() const noexcept { return ussi_; }
    [[nodiscard]] auto smi() const noexcept { return smi_; }
    [[nodiscard]] auto usage_marker() const noexcept { return usage_marker_; }

    friend auto operator<<(std::ostream& stream, const AddressType& address_type) -> std::ostream&;

  private:
    std::optional<std::bitset<10>> country_code_;
    std::optional<std::bitset<14>> network_code_;
    std::optional<std::bitset<8>> sna_;
    std::optional<std::bitset<24>> ssi_;
    std::optional<std::bitset<10>> event_label_;
    std::optional<std::bitset<24>> ussi_;
    std::optional<std::bitset<24>> smi_;
    std::optional<std::bitset<6>> usage_marker_;
};

namespace std {
template <> struct hash<AddressType> {
    auto operator()(const AddressType& k) const -> std::size_t {
        auto stream = std::stringstream();
        stream << k;
        return std::hash<std::string>()(stream.str());
    }
};
} // namespace std

namespace nlohmann {
template <> struct adl_serializer<AddressType> {
    static void to_json(json& j, AddressType address_type) {
        j = json::object();
        std::cout << address_type << std::endl << std::flush;

        if (address_type.country_code()) {
            j["country_code"] = address_type.country_code().value().to_ulong();
        }
        if (address_type.network_code()) {
            j["network_code"] = address_type.network_code().value().to_ulong();
        }
        if (address_type.sna()) {
            j["sna"] = address_type.sna().value().to_ulong();
        }
        if (address_type.ssi()) {
            j["ssi"] = address_type.ssi().value().to_ulong();
        }
        if (address_type.event_label()) {
            j["event_label"] = address_type.event_label().value().to_ulong();
        }
        if (address_type.ussi()) {
            j["ussi"] = address_type.ussi().value().to_ulong();
        }
        if (address_type.smi()) {
            j["smi"] = address_type.smi().value().to_ulong();
        }
        if (address_type.usage_marker()) {
            j["usage_marker"] = address_type.usage_marker().value().to_ulong();
        }
    }
};
} // namespace nlohmann

auto operator<<(std::ostream& stream, const AddressType& address_type) -> std::ostream&;
