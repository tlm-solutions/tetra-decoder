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

void IQStreamDecoder::upperMacWorker() {
    while (true) {
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

std::vector<std::complex<float>> IQStreamDecoder::convolve_valid(std::vector<std::complex<float>> const& a,
                                                                 std::vector<std::complex<float>> const& b) {
    std::vector<std::complex<float>> res;

    // make shure a is longer equal than b
    if (b.size() > a.size()) {
        return convolve_valid(b, a);
    }

    for (int i = 0; i < a.size() - b.size() + 1; i++) {
        std::complex<float> v{};
        auto ita = a.begin();
        std::advance(ita, i);
        auto itb = b.rbegin();
        for (; itb != b.rend(); ita++, itb++) {
            v += *ita * *itb;
        }
        res.push_back(v);
    }

    return res;
}

std::vector<std::complex<float>> IQStreamDecoder::channel_estimation(std::vector<std::complex<float>> const& stream,
                                                                     std::vector<std::complex<float>> const& pilots) {
    // TODO: implement channel estimation
    return stream;
}

void IQStreamDecoder::process_complex(std::complex<float> symbol) noexcept {
    if (is_uplink_) {
        // Control Uplink Burst or Normal Uplink Burst
        symbol_buffer_.push(symbol);
        symbol_buffer_hard_decision_.push(hard_decision(symbol));

        // convolve hard_decision with flipped conjugate of n, p and x and check if abs > SEQUENCE_DETECTION_THRESHOLD
        // to find potential correlation peaks
        //
        // find NUB
        // 2 tail + 108 coded symbols + middle of 11 training sequence (6) = 116
        auto start_n = symbol_buffer_hard_decision_.cbegin();
        std::advance(start_n, 109);
        auto end_n = start_n;
        std::advance(end_n, 11);
        auto find_n = convolve_valid({start_n, end_n}, training_seq_n_reversed_conj_)[0];
        // find NUB_Split
        // 2 tail + 108 coded symbols + middle of 11 training sequence (6) = 116
        auto start_p = symbol_buffer_hard_decision_.cbegin();
        std::advance(start_p, 109);
        auto end_p = start_p;
        std::advance(end_p, 11);
        auto find_p = convolve_valid({start_p, end_p}, training_seq_p_reversed_conj_)[0];
        // find CUB
        // 2 tail + 42 coded symbols + middle of 15 training sequence (8) = 52
        auto start_x = symbol_buffer_hard_decision_.cbegin();
        std::advance(start_x, 44);
        auto end_x = start_x;
        std::advance(end_x, 15);
        auto find_x = convolve_valid({start_x, end_x}, training_seq_x_reversed_conj_)[0];

        // use actual signal for further processing
        auto start = symbol_buffer_.cbegin();

        if (std::abs(find_x) >= SEQUENCE_DETECTION_THRESHOLD) {
            // std::cout << "Potential CUB found" << std::endl;

            auto end = start;
            std::advance(end, 103);

            auto corrected = channel_estimation({start, end}, training_seq_x_reversed_conj_);
            auto bitstream = symbols_to_bitstream(corrected);
            threadPool_->queueWork(std::bind(&LowerMac::process, lower_mac_, bitstream, BurstType::ControlUplinkBurst));
        }

        if (std::abs(find_p) >= SEQUENCE_DETECTION_THRESHOLD) {
            // std::cout << "Potential NUB_Split found" << std::endl;

            auto end = start;
            std::advance(end, 231);

            auto corrected = channel_estimation({start, end}, training_seq_p_reversed_conj_);
            threadPool_->queueWork(std::bind(&LowerMac::process, lower_mac_, symbols_to_bitstream(corrected),
                                             BurstType::NormalUplinkBurstSplit));
        }

        if (std::abs(find_n) >= SEQUENCE_DETECTION_THRESHOLD) {
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
