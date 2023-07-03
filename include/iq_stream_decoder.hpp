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
#include <fixed_queue.hpp>

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
    std::complex<float> hard_decision(std::complex<float> symbol);

    std::vector<std::complex<float>> convolve_valid(std::vector<std::complex<float>> const& a,
                                                    std::vector<std::complex<float>> const& b);

    FixedQueue<std::complex<float>, 300> symbol_buffer_;
    FixedQueue<std::complex<float>, 300> symbol_buffer_hard_decision_;

    // 9.4.4.3.2 Normal training sequence
    const std::vector<std::complex<float>> training_seq_n_ = {{-1, -1}, {-1, 1}, {1, 1},   {1, 1},  {-1, -1}, {1, -1},
                                                              {1, -1},  {-1, 1}, {-1, -1}, {-1, 1}, {1, 1}};
    const std::vector<std::complex<float>> training_seq_p_ = {{-1.0, 1.0}, {-1.0, -1.0}, {1.0, -1.0}, {1.0, -1.0},
                                                              {-1.0, 1.0}, {1.0, 1.0},   {1.0, 1.0},  {-1.0, -1.0},
                                                              {-1.0, 1.0}, {-1.0, -1.0}, {1.0, -1.0}};
    // 9.4.4.3.3 Extended training sequence
    const std::vector<std::complex<float>> training_seq_x_ = {
        {1.0, -1.0}, {-1.0, 1.0}, {-1.0, -1.0}, {-1.0, 1.0}, {1.0, 1.0}, {1.0, 1.0}, {-1.0, -1.0}, {1.0, -1.0},
        {1.0, -1.0}, {-1.0, 1.0}, {-1.0, -1.0}, {-1.0, 1.0}, {1.0, 1.0}, {1.0, 1.0}, {-1.0, -1.0}};

    const float SEQUENCE_DETECTION_THRESHOLD = 1.5;

    std::shared_ptr<BitStreamDecoder> bit_stream_decoder_{};

    bool is_uplink_{};
};
