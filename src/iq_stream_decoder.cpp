/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#include "l2/lower_mac.hpp"
#include <memory>

#include <iq_stream_decoder.hpp>

IQStreamDecoder::IQStreamDecoder(
    const std::shared_ptr<StreamingOrderedOutputThreadPoolExecutor<LowerMac::return_type>>& lower_mac_worker_queue,
    const std::shared_ptr<LowerMac>& lower_mac, const std::shared_ptr<BitStreamDecoder>& bit_stream_decoder,
    bool is_uplink)
    : lower_mac_(lower_mac)
    , bit_stream_decoder_(bit_stream_decoder)
    , is_uplink_(is_uplink)
    , lower_mac_worker_queue_(lower_mac_worker_queue) {
    std::transform(training_seq_n_.crbegin(), training_seq_n_.crend(),
                   std::back_inserter(training_seq_n_reversed_conj_), [](auto v) { return std::conj(v); });
    std::transform(training_seq_p_.crbegin(), training_seq_p_.crend(),
                   std::back_inserter(training_seq_p_reversed_conj_), [](auto v) { return std::conj(v); });
    std::transform(training_seq_x_.crbegin(), training_seq_x_.crend(),
                   std::back_inserter(training_seq_x_reversed_conj_), [](auto v) { return std::conj(v); });
}

std::complex<float> IQStreamDecoder::hard_decision(std::complex<float> const& symbol) {
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

template <class iterator_type>
void IQStreamDecoder::symbols_to_bitstream(iterator_type it, uint8_t* const bits, const std::size_t len) {
    for (auto i = 0; i < len; ++it, ++i) {

        auto real = it->real();
        auto imag = it->imag();
        uint8_t symb0, symb1;

        if (real > 0.0) {
            if (imag > 0.0) {
                // I
                symb0 = 0;
                symb1 = 0;
            } else {
                // IV
                symb0 = 1;
                symb1 = 0;
            }
        } else {
            if (imag > 0.0) {
                // II
                symb0 = 0;
                symb1 = 1;
            } else {
                // III
                symb0 = 1;
                symb1 = 1;
            }
        }

        bits[i * 2] = symb0;
        bits[i * 2 + 1] = symb1;
    }
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

        if (detectedX >= SEQUENCE_DETECTION_THRESHOLD) {
            // std::cout << "Potential CUB found" << std::endl;

            auto len = 103;

            std::vector<uint8_t> bits(len * 2);

            symbols_to_bitstream(symbol_buffer_.cbegin(), bits.data(), len);

            auto lower_mac_process_cub = std::bind(&LowerMac::process, lower_mac_, bits, BurstType::ControlUplinkBurst);
            lower_mac_worker_queue_->queue_work(lower_mac_process_cub);
        }

        if (detectedP >= SEQUENCE_DETECTION_THRESHOLD) {
            // std::cout << "Potential NUB_Split found" << std::endl;

            auto len = 231;

            std::vector<uint8_t> bits(len * 2);

            symbols_to_bitstream(symbol_buffer_.cbegin(), bits.data(), len);

            auto lower_mac_process_nubs =
                std::bind(&LowerMac::process, lower_mac_, bits, BurstType::NormalUplinkBurstSplit);
            lower_mac_worker_queue_->queue_work(lower_mac_process_nubs);
        }

        if (detectedN >= SEQUENCE_DETECTION_THRESHOLD) {
            // std::cout << "Potential NUB found" << std::endl;

            auto len = 231;

            std::vector<uint8_t> bits(len * 2);

            symbols_to_bitstream(symbol_buffer_.cbegin(), bits.data(), len);

            auto lower_mac_process_nub = std::bind(&LowerMac::process, lower_mac_, bits, BurstType::NormalUplinkBurst);
            lower_mac_worker_queue_->queue_work(lower_mac_process_nub);
        }
    } else {
        // TODO: this path needs to change!
        std::vector<std::complex<float>> stream = {symbol};
        std::vector<uint8_t> bits(2);
        symbols_to_bitstream(stream.cbegin(), bits.data(), 1);
        for (auto it = bits.begin(); it != bits.end(); ++it) {
            bit_stream_decoder_->process_bit(*it);
        }
    }
}
