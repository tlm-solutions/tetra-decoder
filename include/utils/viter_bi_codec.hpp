/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#pragma once

#include <limits>

// include for viterbi algorithm size_t
#include <stdio.h>
#include "viterbi/viterbi_decoder_scalar.h"
#include "viterbi/x86/viterbi_decoder_sse_u16.h"

constexpr size_t K = 5;
constexpr size_t R = 4;

class ViterbiCodec {
  public:
    ViterbiCodec(){};
    [[nodiscard]] std::vector<uint8_t> Decode(const std::vector<int16_t>& bits) const;

  private:
    const std::vector<uint8_t> G = {19, 29, 23, 27};

    const int16_t soft_decision_high = +1;
    const int16_t soft_decision_low = -1;
    const uint16_t max_error = uint16_t(soft_decision_high - soft_decision_low) * uint16_t(R);
    const uint16_t error_margin = max_error * uint16_t(3u);

    const ViterbiDecoder_Config<uint16_t> config = {
        .soft_decision_max_error = max_error,
        .initial_start_error = std::numeric_limits<uint16_t>::min(),
        .initial_non_start_error = static_cast<uint16_t>(std::numeric_limits<uint16_t>::min() + error_margin),
        .renormalisation_threshold = static_cast<uint16_t>(std::numeric_limits<uint16_t>::max() - error_margin)};

    const ViterbiBranchTable<K, R, int16_t> branch_table =
        ViterbiBranchTable<K, R, int16_t>(G.data(), soft_decision_high, soft_decision_low);
};
