/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#include <iq_stream_decoder.hpp>

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

void IQStreamDecoder::process_complex(std::complex<float> symbol) noexcept {
    if (is_uplink_) {
        // Control Uplink Burst or Normal Uplink Burst
        symbol_buffer_.push(symbol);
        symbol_buffer_hard_decision_.push(hard_decision(symbol));

        // convolve hard_decision with flipped conjugate of n, p and x and check if abs > SEQUENCE_DETECTION_THRESHOLD
        // to find potential correlation peaks
        auto find_n =
            convolve_valid({symbol_buffer_hard_decision_.begin(), symbol_buffer_hard_decision_.end()}, training_seq_n_);
        auto find_p =
            convolve_valid({symbol_buffer_hard_decision_.begin(), symbol_buffer_hard_decision_.end()}, training_seq_p_);
        auto find_x =
            convolve_valid({symbol_buffer_hard_decision_.begin(), symbol_buffer_hard_decision_.end()}, training_seq_x_);

        // find CUB
        // 2 tail + 42 coded symbols + middle of 15 training sequence (8) = 52
        if (std::abs(find_x[51]) > SEQUENCE_DETECTION_THRESHOLD) {
            // potentially found CUB
            std::cout << "Potential CUB found" << std::endl;
        }

        // find NUB
        // 2 tail + 108 coded symbols + middle of 11 training sequence (6) = 116
        if (std::abs(find_p[115]) > SEQUENCE_DETECTION_THRESHOLD) {
            std::cout << "Potential NUB_Split found" << std::endl;
        }

        if (std::abs(find_n[115]) > SEQUENCE_DETECTION_THRESHOLD &&
            std::abs(find_x[115]) <= SEQUENCE_DETECTION_THRESHOLD) {
            std::cout << "Potential NUB found" << std::endl;
        }
    } else {
        // Simple hard decission symbol mapper for now...
        auto real = symbol.real();
        auto imag = symbol.imag();

        if (real > 0.0) {
            if (imag > 0.0) {
                // I
                bit_stream_decoder_->process_bit(0);
                bit_stream_decoder_->process_bit(0);
            } else {
                // IV
                bit_stream_decoder_->process_bit(1);
                bit_stream_decoder_->process_bit(0);
            }
        } else {
            if (imag > 0.0) {
                // II
                bit_stream_decoder_->process_bit(0);
                bit_stream_decoder_->process_bit(1);
            } else {
                // III
                bit_stream_decoder_->process_bit(1);
                bit_stream_decoder_->process_bit(1);
            }
        }
    }
}
