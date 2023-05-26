/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#ifndef TETRA_DECODER_BURSTTYPE_HPP
#define TETRA_DECODER_BURSTTYPE_HPP

class BurstType {
  public:
    enum Value {
        ControlUplinkBurst,
        NormalUplinkBurst,
        NormalUplinkBurstSplit,
        NormalDownlinkBurst,
        NormalDownlinkBurstSplit,
        SynchronizationBurst,
    };

    BurstType() = default;
    constexpr BurstType(Value value)
        : value_(value) {}

    constexpr operator Value() const { return value_; }

    explicit operator bool() const = delete;
    constexpr bool operator==(BurstType burst_type) const { return value_ == burst_type; }
    constexpr bool operator!=(BurstType burst_type) const { return value_ != burst_type; }
    constexpr bool operator==(BurstType::Value burst_type) const { return value_ == burst_type; }
    constexpr bool operator!=(BurstType::Value burst_type) const { return value_ != burst_type; }

    [[nodiscard]] auto is_uplink_burst() const noexcept -> bool {
        return value_ == ControlUplinkBurst || value_ == NormalUplinkBurst || value_ == NormalUplinkBurstSplit;
    }

    [[nodiscard]] auto is_downlink_burst() const noexcept -> bool { return !is_uplink_burst(); }

  private:
    Value value_{};
};

#endif // TETRA_DECODER_BURSTTYPE_HPP
