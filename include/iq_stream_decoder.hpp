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
#include <memory>
#include <thread>

#include <bit_stream_decoder.hpp>
#include <fixed_queue.hpp>
#include <l2/lower_mac.hpp>
#include <signal_handler.hpp>
#include <streaming_ordered_output_thread_pool_executor.hpp>

/**
 * Tetra downlink decoder for PI/4-DQPSK modulation
 *
 * Implements Channel Estimation
 */
class IQStreamDecoder {
  public:
    IQStreamDecoder(std::shared_ptr<LowerMac> lower_mac, std::shared_ptr<BitStreamDecoder> bit_stream_decoder,
                    bool is_uplink);
    ~IQStreamDecoder();

    void process_complex(std::complex<float> symbol) noexcept;

  private:
    using QueueT = FixedQueue<std::complex<float>, 300>;

    void upperMacWorker();

    static std::complex<float> hard_decision(std::complex<float> const& symbol);

    template <class iterator_type> static void symbols_to_bitstream(iterator_type it, uint8_t* bits, std::size_t len);

    static void abs_convolve_same_length(const QueueT& queueA, std::size_t offsetA, const std::complex<float>* itb,
                                         std::size_t len, float* res);

    std::vector<std::complex<float>> channel_estimation(std::vector<std::complex<float>> const& stream,
                                                        std::vector<std::complex<float>> const& pilots);

    QueueT symbol_buffer_;
    QueueT symbol_buffer_hard_decision_;

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

    std::shared_ptr<StreamingOrderedOutputThreadPoolExecutor<std::vector<std::function<void()>>>> thread_pool_;

    std::thread upper_mac_worker_thread_;
};
