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
#include <l2/lower_mac.hpp>

/**
 * Tetra downlink decoder for PI/4-DQPSK modulation
 *
 * Implements Channel Estimation
 */
class IQStreamDecoder {
  public:
    IQStreamDecoder(std::shared_ptr<LowerMac> lower_mac, std::shared_ptr<BitStreamDecoder> bit_stream_decoder,
                    bool is_uplink);
    ~IQStreamDecoder() = default;

    void process_complex(std::complex<float> symbol) noexcept;

  private:
    std::complex<float> hard_decision(std::complex<float> symbol);
    std::vector<uint8_t> symbols_to_bitstream(std::vector<std::complex<float>> const& stream);

    std::vector<std::complex<float>> convolve_valid(std::vector<std::complex<float>> const& a,
                                                    std::vector<std::complex<float>> const& b);

    std::vector<std::complex<float>> channel_estimation(std::vector<std::complex<float>> const& stream,
                                                        std::vector<std::complex<float>> const& pilots);

    FixedQueue<std::complex<float>, 300> symbol_buffer_;
    FixedQueue<std::complex<float>, 300> symbol_buffer_hard_decision_;

    // 9.4.4.3.2 Normal training sequence
    const std::vector<std::complex<float>> training_seq_n_ = {{-1, -1}, {-1, 1}, {1, 1},   {1, 1},  {-1, -1}, {1, -1},
                                                              {1, -1},  {-1, 1}, {-1, -1}, {-1, 1}, {1, 1}};
    std::vector<std::complex<float>> training_seq_n_reversed_conj_{};
    const std::vector<std::complex<float>> training_seq_p_ = {{-1.0, 1.0}, {-1.0, -1.0}, {1.0, -1.0}, {1.0, -1.0},
                                                              {-1.0, 1.0}, {1.0, 1.0},   {1.0, 1.0},  {-1.0, -1.0},
                                                              {-1.0, 1.0}, {-1.0, -1.0}, {1.0, -1.0}};
    std::vector<std::complex<float>> training_seq_p_reversed_conj_{};
    // 9.4.4.3.3 Extended training sequence
    const std::vector<std::complex<float>> training_seq_x_ = {
        {1.0, -1.0}, {-1.0, 1.0}, {-1.0, -1.0}, {-1.0, 1.0}, {1.0, 1.0}, {1.0, 1.0}, {-1.0, -1.0}, {1.0, -1.0},
        {1.0, -1.0}, {-1.0, 1.0}, {-1.0, -1.0}, {-1.0, 1.0}, {1.0, 1.0}, {1.0, 1.0}, {-1.0, -1.0}};
    std::vector<std::complex<float>> training_seq_x_reversed_conj_{};

    const float SEQUENCE_DETECTION_THRESHOLD = 1.5;

    std::shared_ptr<LowerMac> lower_mac_{};
    std::shared_ptr<BitStreamDecoder> bit_stream_decoder_{};

    bool is_uplink_{};
};
