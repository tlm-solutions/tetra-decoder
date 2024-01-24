/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#include <thread>
#if defined(__linux__)
#include <pthread.h>
#endif

#include <iq_stream_decoder.hpp>

IQStreamDecoder::IQStreamDecoder(std::shared_ptr<LowerMac> lower_mac,
                                 std::shared_ptr<BitStreamDecoder> bit_stream_decoder, bool is_uplink)
    : lower_mac_(lower_mac)
    , bit_stream_decoder_(bit_stream_decoder)
    , is_uplink_(is_uplink) {
    std::transform(training_seq_n_.crbegin(), training_seq_n_.crend(),
                   std::back_inserter(training_seq_n_reversed_conj_), [](auto v) { return std::conj(v); });
    std::transform(training_seq_p_.crbegin(), training_seq_p_.crend(),
                   std::back_inserter(training_seq_p_reversed_conj_), [](auto v) { return std::conj(v); });
    std::transform(training_seq_x_.crbegin(), training_seq_x_.crend(),
                   std::back_inserter(training_seq_x_reversed_conj_), [](auto v) { return std::conj(v); });

    threadPool_ = std::make_shared<StreamingOrderedOutputThreadPoolExecutor<std::vector<std::function<void()>>>>(4);

    upperMacWorkerThread_ = std::thread(&IQStreamDecoder::upperMacWorker, this);

#if defined(__linux__)
    auto handle = upperMacWorkerThread_.native_handle();
    pthread_setname_np(handle, "UpperMacWorker");
#endif
}

IQStreamDecoder::~IQStreamDecoder() {
    // TODO: replace this crude hack that keeps the StreamingOrderedOutputThreadPoolExecutor<...> get function from
    // blocking on programm stop
    threadPool_->queueWork([]() { return std::vector<std::function<void()>>(); });
    upperMacWorkerThread_.join();
}

void IQStreamDecoder::upperMacWorker() {
    while (!stop) {
        for (auto func : threadPool_->get()) {
            func();
        }
    }
}

std::complex<float> IQStreamDecoder::hard_decision(std::complex<float> symbol) {
    if (symbol.real() > 0) {
        if (symbol.imag() > 0) {
            return std::complex<float>(1, 1);
        } else {
            return std::complex<float>(1, -1);
        }
    } else {
        if (symbol.imag() > 0) {
            return std::complex<float>(-1, 1);
        } else {
            return std::complex<float>(-1, -1);
        }
    }
}

std::vector<uint8_t> IQStreamDecoder::symbols_to_bitstream(std::vector<std::complex<float>> const& stream) {
    std::vector<uint8_t> bits;

    for (auto it = stream.begin(); it != stream.end(); it++) {

        auto real = it->real();
        auto imag = it->imag();

        if (real > 0.0) {
            if (imag > 0.0) {
                // I
                bits.push_back(0);
                bits.push_back(0);
            } else {
                // IV
                bits.push_back(1);
                bits.push_back(0);
            }
        } else {
            if (imag > 0.0) {
                // II
                bits.push_back(0);
                bits.push_back(1);
            } else {
                // III
                bits.push_back(1);
                bits.push_back(1);
            }
        }
    }

    return bits;
}

void IQStreamDecoder::abs_convolve_same_length(const QueueT& queueA, const std::size_t offsetA,
                                               const std::complex<float>* const itb, const std::size_t len,
                                               float* res) {
    std::complex<float> acc = {0.0, 0.0};
    for (std::size_t i = 0; i < len; ++i) {
        acc += queueA[offsetA + i] * itb[i];
    }
    *res = std::abs(acc);
}

std::vector<std::complex<float>> IQStreamDecoder::channel_estimation(std::vector<std::complex<float>> const& stream,
                                                                     std::vector<std::complex<float>> const& pilots) {
    // TODO: implement channel estimation
    return stream;
}

void IQStreamDecoder::process_complex(std::complex<float> symbol) noexcept {
    if (is_uplink_) {
        float detectedN;
        float detectedP;
        float detectedX;

        // Control Uplink Burst or Normal Uplink Burst
        symbol_buffer_.push(symbol);
        symbol_buffer_hard_decision_.push(hard_decision(symbol));

        // convolve hard_decision with flipped conjugate of n, p and x and check if abs > SEQUENCE_DETECTION_THRESHOLD
        // to find potential correlation peaks
        //
        // find NUB
        // 2 tail + 108 coded symbols + middle of 11 training sequence (6) = 116
        abs_convolve_same_length(symbol_buffer_hard_decision_, 109, training_seq_n_reversed_conj_.data(),
                                 training_seq_n_reversed_conj_.size(), &detectedN);
        // find NUB_Split
        // 2 tail + 108 coded symbols + middle of 11 training sequence (6) = 116
        abs_convolve_same_length(symbol_buffer_hard_decision_, 109, training_seq_p_reversed_conj_.data(),
                                 training_seq_p_reversed_conj_.size(), &detectedP);
        // find CUB
        // 2 tail + 42 coded symbols + middle of 15 training sequence (8) = 52
        abs_convolve_same_length(symbol_buffer_hard_decision_, 44, training_seq_x_reversed_conj_.data(),
                                 training_seq_x_reversed_conj_.size(), &detectedX);

        // use actual signal for further processing
        auto start = symbol_buffer_.cbegin();

        if (detectedX >= SEQUENCE_DETECTION_THRESHOLD) {
            // std::cout << "Potential CUB found" << std::endl;

            auto end = start;
            std::advance(end, 103);

            auto corrected = channel_estimation({start, end}, training_seq_x_reversed_conj_);
            auto bitstream = symbols_to_bitstream(corrected);
            threadPool_->queueWork(std::bind(&LowerMac::process, lower_mac_, bitstream, BurstType::ControlUplinkBurst));
        }

        if (detectedP >= SEQUENCE_DETECTION_THRESHOLD) {
            // std::cout << "Potential NUB_Split found" << std::endl;

            auto end = start;
            std::advance(end, 231);

            auto corrected = channel_estimation({start, end}, training_seq_p_reversed_conj_);
            threadPool_->queueWork(std::bind(&LowerMac::process, lower_mac_, symbols_to_bitstream(corrected),
                                             BurstType::NormalUplinkBurstSplit));
        }

        if (detectedN >= SEQUENCE_DETECTION_THRESHOLD) {
            // std::cout << "Potential NUB found" << std::endl;

            auto end = start;
            std::advance(end, 231);

            auto corrected = channel_estimation({start, end}, training_seq_n_reversed_conj_);
            threadPool_->queueWork(std::bind(&LowerMac::process, lower_mac_, symbols_to_bitstream(corrected),
                                             BurstType::NormalUplinkBurst));
        }
    } else {
        auto bits = symbols_to_bitstream({symbol});
        for (auto it = bits.begin(); it != bits.end(); ++it) {
            bit_stream_decoder_->process_bit(*it);
        }
    }
}
