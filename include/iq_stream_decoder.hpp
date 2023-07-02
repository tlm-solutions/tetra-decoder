/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#pragma once

#include <complex>

#include <bit_stream_decoder.hpp>

/**
 * Tetra downlink decoder for PI/4-DQPSK modulation
 *
 * Implements Channel Estimation
 */
class IQStreamDecoder {
  public:
    IQStreamDecoder(std::shared_ptr<BitStreamDecoder> bit_stream_decoder, bool is_uplink)
        : bit_stream_decoder_(bit_stream_decoder)
        , is_uplink_(is_uplink){};
    ~IQStreamDecoder() = default;

    void process_complex(std::complex<float> symbol) noexcept;

  private:
    std::shared_ptr<BitStreamDecoder> bit_stream_decoder_{};

    bool is_uplink_{};
};
