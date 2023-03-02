/*
* Copyright (C) 2022 Transit Live Mapping Solutions
* All rights reserved.
*
* Authors:
*   Marenz Schmidl
*   Tassilo Tanneberger
 */

#ifndef L2_LOWERMAC_HPP
#define L2_LOWERMAC_HPP

#include <cstdint>
#include <memory>
#include <vector>

#include <burst_type.hpp>
#include <l2/upper_mac.hpp>
#include <utils/viter_bi_codec.hpp>

class LowerMac {
  public:
    LowerMac();
    ~LowerMac() = default;

    void process(const std::vector<uint8_t>& frame, BurstType burst_type);

  private:
    std::unique_ptr<ViterbiCodec> viter_bi_codec_1614_;
    std::shared_ptr<UpperMac> upper_mac_;

    [[nodiscard]] static auto descramble(const std::vector<uint8_t>& data, int len, uint32_t scramblingCode) noexcept -> std::vector<uint8_t>;
    [[nodiscard]] static auto deinterleave(const std::vector<uint8_t>& data, uint32_t K, uint32_t a) noexcept -> std::vector<uint8_t>;
    [[nodiscard]] static auto depuncture23(const std::vector<uint8_t>& data, uint32_t len) noexcept -> std::vector<uint8_t>;
    [[nodiscard]] static auto reed_muller_3014_decode(const std::vector<uint8_t>& data) noexcept -> std::vector<uint8_t>;
    [[nodiscard]] static auto check_crc_16_ccitt(const std::vector<uint8_t>& data, int len) noexcept -> int;

    [[nodiscard]] auto viter_bi_decode_1614(const std::vector<uint8_t>& data) noexcept -> std::vector<uint8_t>;

};

#endif
