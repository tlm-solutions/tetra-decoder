/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <burst_type.hpp>
#include <l2/upper_mac.hpp>
#include <reporter.hpp>
#include <utils/viter_bi_codec.hpp>

class LowerMac {
  public:
    LowerMac(std::shared_ptr<Reporter> reporter);
    ~LowerMac() = default;

    // does the signal processing and then returns a list of function that need to be executed for data to be passed to
    // upper mac sequentially.
    // TODO: this is currently only done for uplink bursts
    // Downlink burst get processed and passed to upper mac directly
    [[nodiscard]] auto process(const std::vector<uint8_t>& frame, BurstType burst_type)
        -> std::vector<std::function<void()>>;
    void set_scrambling_code(unsigned int scrambling_code) { upper_mac_->set_scrambling_code(scrambling_code); };

  private:
    std::shared_ptr<Reporter> reporter_{};
    std::shared_ptr<ViterbiCodec> viter_bi_codec_1614_{};
    std::shared_ptr<UpperMac> upper_mac_{};

    static auto descramble(const uint8_t* const data, uint8_t* const res, const std::size_t len,
                           const uint32_t scramblingCode) noexcept -> void;
    static auto deinterleave(const uint8_t* const data, uint8_t* const res, const std::size_t K,
                             const std::size_t a) noexcept -> void;
    [[nodiscard]] static auto depuncture23(const uint8_t* const data, const uint32_t len) noexcept
        -> std::vector<int16_t>;
    static auto reed_muller_3014_decode(const uint8_t* const data, uint8_t* const res) noexcept -> void;
    [[nodiscard]] static auto check_crc_16_ccitt(const uint8_t* const data, const std::size_t len) noexcept -> bool;

    [[nodiscard]] auto viter_bi_decode_1614(const std::vector<int16_t>& data) const noexcept -> std::vector<uint8_t>;
};