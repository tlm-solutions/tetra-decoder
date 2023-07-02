/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <bit_stream_decoder.hpp>
#include <l2/lower_mac.hpp>
#include <reporter.hpp>

/**
 * Tetra downlink decoder for PI/4-DQPSK modulation
 *
 * Following downlink burst types for Phase Modulation are supported: (See TSI
 * EN 300 392-2 V3.8.1 (2016-08) Table 9.2)
 *
 * normal continuous downlink burst (NDB)
 *
 * synchronization continuous downlink burst (SB)
 *
 * Follawing downlink burst types for Phase Modulation are not supported:
 * discontinuous downlink burst (NDB)
 * synchronization discontinuous downlink burst (SB)
 *
 * Uplink burst types are not supported:
 * control uplink burst (CB)
 * normal uplink burst (NUB)
 */
class Decoder {
  public:
    Decoder(unsigned int receive_port, unsigned int send_port, bool packed, std::optional<std::string> input_file,
            std::optional<std::string> output_file, bool iq_or_bit_stream,
            std::optional<unsigned int> uplink_scrambling_code);
    ~Decoder();

    void main_loop();

  private:
    std::shared_ptr<LowerMac> lower_mac_{};
    std::shared_ptr<Reporter> reporter_{};
    std::unique_ptr<BitStreamDecoder> bit_stream_decoder_{};

    bool packed_ = false;

    // input and output file descriptor
    int input_fd_ = 0;

    // optional output file
    std::optional<int> output_file_fd_ = std::nullopt;

    // uplink ?
    std::optional<unsigned int> uplink_scrambling_code_;

    // IQ stream -> true
    // bit stream -> false
    bool iq_or_bit_stream_;

    const std::size_t kRX_BUFFER_SIZE = 4096;
};
