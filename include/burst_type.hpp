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
        NormalUplinkBurst_Split,
        NormalDownlinkBurst,
        NormalDownlinkBurst_Split,
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

    [[nodiscard]] auto isUplinkBurst() const noexcept -> bool {
        switch (value_) {
        case ControlUplinkBurst:
        case NormalUplinkBurst:
        case NormalUplinkBurst_Split:
            return true;
        case NormalDownlinkBurst:
        case NormalDownlinkBurst_Split:
        case SynchronizationBurst:
        default:
            return false;
        }
    }

    [[nodiscard]] auto isDownlinkBurst() const noexcept -> bool { return !isUplinkBurst(); }

  private:
    Value value_;
};

#endif // TETRA_DECODER_BURSTTYPE_HPP
