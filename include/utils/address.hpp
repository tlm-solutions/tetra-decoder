/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "nlohmann/std_optional.hpp"    // IWYU pragma: keep
#include "nlohmann/unsigned_bitint.hpp" // IWYU pragma: keep
#include "utils/bit_vector.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
#include <optional>
#include <sstream>

class Address {
  public:
    Address() = default;

    /// extract the address from the address type and addres fields in MAC-ACCESS
    static auto from_mac_access(BitVector& data) -> Address;
    /// extract the address from the address type and addres fields in MAC-DATA
    static auto from_mac_data(BitVector& data) -> Address;
    /// extract the address from the address type and addres fields in MAC-RESOURCE
    static auto from_mac_resource(BitVector& data) -> Address;

    // explicit operator bool() const = delete;
    constexpr auto operator==(Address address_type) const -> bool {
        return country_code_ == address_type.country_code_ && network_code_ == address_type.network_code_ &&
               sna_ == address_type.sna_ && ssi_ == address_type.ssi_ && event_label_ == address_type.event_label_ &&
               ussi_ == address_type.ussi_ && smi_ == address_type.smi_ && usage_marker_ == address_type.usage_marker_;
    }

    void set_country_code(unsigned _BitInt(10) country_code) { country_code_ = country_code; }
    void set_network_code(unsigned _BitInt(14) network_code) { network_code_ = network_code; }
    void set_sna(unsigned _BitInt(8) sna) { sna_ = sna; }
    void set_ssi(unsigned _BitInt(24) ssi) { ssi_ = ssi; }
    void set_event_label(unsigned _BitInt(10) event_label) { event_label_ = event_label; }
    void set_ussi(unsigned _BitInt(24) ussi) { ussi_ = ussi; }
    void set_smi(unsigned _BitInt(24) smi) { smi_ = smi; }
    void set_usage_marker(unsigned _BitInt(6) usage_marker) { usage_marker_ = usage_marker; }

    [[nodiscard]] auto country_code() const noexcept { return country_code_; }
    [[nodiscard]] auto network_code() const noexcept { return network_code_; }
    [[nodiscard]] auto sna() const noexcept { return sna_; }
    [[nodiscard]] auto ssi() const noexcept { return ssi_; }
    [[nodiscard]] auto event_label() const noexcept { return event_label_; }
    [[nodiscard]] auto ussi() const noexcept { return ussi_; }
    [[nodiscard]] auto smi() const noexcept { return smi_; }
    [[nodiscard]] auto usage_marker() const noexcept { return usage_marker_; }

    auto merge(const Address& other) {
        if (other.country_code_) {
            country_code_ = other.country_code_;
        }
        if (other.network_code_) {
            network_code_ = other.network_code_;
        }
        if (other.sna_) {
            sna_ = other.sna_;
        }
        if (other.ssi_) {
            ssi_ = other.ssi_;
        }
        if (other.event_label_) {
            event_label_ = other.event_label_;
        }
        if (other.ussi_) {
            ussi_ = other.ussi_;
        }
        if (other.smi_) {
            smi_ = other.smi_;
        }
        if (other.usage_marker_) {
            usage_marker_ = other.usage_marker_;
        }
    };

    auto merge(const std::optional<Address>& address) {
        if (address) {
            merge(*address);
        }
    }

    // Overload this operator for usage of the Address as a map key
    auto operator<(const Address& other) const -> bool {
        return std::tie(country_code_, network_code_, sna_, ssi_, event_label_, ussi_, smi_, usage_marker_) <
               std::tie(other.country_code_, other.network_code_, other.sna_, other.ssi_, other.event_label_,
                        other.ussi_, other.smi_, other.usage_marker_);
    }

    friend auto operator<<(std::ostream& stream, const Address& address_type) -> std::ostream&;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Address, country_code_, network_code_, sna_, ssi_, event_label_, ussi_, smi_,
                                   usage_marker_)

  private:
    std::optional<unsigned _BitInt(10)> country_code_;
    std::optional<unsigned _BitInt(14)> network_code_;
    std::optional<unsigned _BitInt(8)> sna_;
    std::optional<unsigned _BitInt(24)> ssi_;
    std::optional<unsigned _BitInt(10)> event_label_;
    std::optional<unsigned _BitInt(24)> ussi_;
    std::optional<unsigned _BitInt(24)> smi_;
    std::optional<unsigned _BitInt(6)> usage_marker_;
};

namespace std {
template <> struct hash<Address> {
    auto operator()(const Address& k) const -> std::size_t {
        auto stream = std::stringstream();
        stream << k;
        return std::hash<std::string>()(stream.str());
    }
};
} // namespace std

auto operator<<(std::ostream& stream, const Address& address_type) -> std::ostream&;
