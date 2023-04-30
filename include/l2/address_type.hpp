/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#ifndef TETRA_DECODER_ADDRESS_TYPE_HPP
#define TETRA_DECODER_ADDRESS_TYPE_HPP

#include <bitset>
#include <optional>

class AddressType {
  public:
    AddressType() = default;

    explicit operator bool() const = delete;
    constexpr bool operator==(AddressType address_type) const {
        return ssi_ == address_type.ssi_ && event_label_ == address_type.event_label_ && ussi_ == address_type.ussi_ &&
               smi_ == address_type.smi_ && usage_marker_ == address_type.usage_marker_;
    }
    constexpr bool operator!=(AddressType address_type) const { return !(*this == address_type); }

    void set_ssi(std::bitset<24> ssi) { ssi_ = ssi; }
    void set_event_label(std::bitset<10> event_label) { event_label_ = event_label; }
    void set_ussi(std::bitset<24> ussi) { ussi_ = ussi; }
    void set_smi(std::bitset<24> smi) { smi_ = smi; }
    void set_usage_marker(std::bitset<6> usage_marker) { usage_marker_ = usage_marker; }

    friend std::ostream& operator<<(std::ostream& stream, const AddressType& address_type);

  private:
    std::optional<std::bitset<24>> ssi_;
    std::optional<std::bitset<10>> event_label_;
    std::optional<std::bitset<24>> ussi_;
    std::optional<std::bitset<24>> smi_;
    std::optional<std::bitset<6>> usage_marker_;
};

std::ostream& operator<<(std::ostream& stream, const AddressType& address_type);

#endif // TETRA_DECODER_BURSTTYPE_HPP
