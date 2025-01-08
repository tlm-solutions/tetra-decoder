/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#include "iq_stream_decoder.hpp"
#include "l2/lower_mac.hpp"
#include <armadillo>
#include <memory>

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

template <std::size_t Len, class iterator_type>
auto IQStreamDecoder::symbols_to_bitstream(iterator_type it) -> std::vector<bool> {
    std::vector<bool> bits(Len * 2);
    for (std::size_t i = 0; i < Len; ++it, ++i) {
        // symbol 0:
        //  imag  > 0 -> 0
        //  imag <= 0 -> 1
        // symbol 1:
        //  real  > 0 -> 0
        //  real <= 0 -> 1
        bits[i * 2] = it->imag() <= 0.0;
        bits[(i * 2) + 1] = it->real() <= 0.0;
    }
    return bits;
}

template <std::size_t Len, class iterator_type>
auto IQStreamDecoder::symbols_to_softstream(iterator_type it) -> std::vector<int16_t> {
    std::vector<int16_t> soft_bits(Len * 2);
    for (std::size_t i = 0; i < Len; ++it, ++i) {
        // symbol 0:
        //  imag  > 0 -> 0 -> -1
        //  imag <= 0 -> 1 -> 1
        // symbol 1:
        //  real  > 0 -> 0 -> -1
        //  real <= 0 -> 1 -> 1
        soft_bits[i * 2] = -1 * it->imag();
        soft_bits[(i * 2) + 1] = -1 * it->real();
    }
    return soft_bits;
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

template <std::size_t ChannelSize>
auto IQStreamDecoder::solve_channel(const std::vector<std::complex<float>>& pilots, const QueueT& signal_queue,
                                    const std::size_t signal_offset) -> arma::cx_fvec {
    // Calculate the minimum variance unbiased estimator
    auto h = arma::cx_fmat(/*n_rows=*/pilots.size(), /*n_cols=*/ChannelSize, arma::fill::zeros);

    for (auto row = 0; row < h.n_rows; row++) {
        auto index = row;
        for (auto col = 0; col < h.n_cols; col++) {
            if (index >= 0) {
                h.row(row).col(col) = signal_queue[signal_offset + index];
            }
            index--;
        }
    }

    // std::cout << h << '\n';

    auto signal = arma::cx_fvec(pilots.size());
    for (auto i = 0; i < signal.size(); i++) {
        signal[i] = signal_queue[signal_offset + i];
    }

    // std::cout << signal << '\n';

    auto h_hermitian = arma::cx_fmat(/*n_rows=*/ChannelSize, /*n_cols=*/pilots.size(), arma::fill::zeros);

    h_hermitian = h.t();
    h_hermitian = arma::conj(h_hermitian);
    // std::cout << h_hermitian << '\n';

    auto h_hermitian_h = h_hermitian * h;
    // std::cout << h_hermitian_h << '\n';

    // auto h_hermitian_h_inv = arma::inv(h_hermitian_h);
    // std::cout << h_hermitian_h_inv << '\n';

    auto h_hermitian_signal = h_hermitian * signal;
    // std::cout << h_hermitian_signal << '\n';

    auto c_mvue = arma::solve(h_hermitian_h, h_hermitian_signal);
    // std::cout << c_mvue << '\n';

    return c_mvue;
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

            auto channel = solve_channel<3>(training_seq_x_, symbol_buffer_hard_decision_, 44);
            std::cout << channel << std::endl;

            auto softbits = symbols_to_softstream<103>(symbol_buffer_.cbegin());

            auto lower_mac_process_cub =
                std::bind(&LowerMac::process<int16_t>, lower_mac_, softbits, BurstType::ControlUplinkBurst);
            lower_mac_worker_queue_->queue_work(lower_mac_process_cub);
        }

        if (detectedP >= SEQUENCE_DETECTION_THRESHOLD) {
            // std::cout << "Potential NUB_Split found" << std::endl;

            auto softbits = symbols_to_softstream<231>(symbol_buffer_.cbegin());

            auto lower_mac_process_nubs =
                std::bind(&LowerMac::process<int16_t>, lower_mac_, softbits, BurstType::NormalUplinkBurstSplit);
            lower_mac_worker_queue_->queue_work(lower_mac_process_nubs);
        }

        if (detectedN >= SEQUENCE_DETECTION_THRESHOLD) {
            // std::cout << "Potential NUB found" << std::endl;

            auto softbits = symbols_to_softstream<231>(symbol_buffer_.cbegin());

            auto lower_mac_process_nub =
                std::bind(&LowerMac::process<int16_t>, lower_mac_, softbits, BurstType::NormalUplinkBurst);
            lower_mac_worker_queue_->queue_work(lower_mac_process_nub);
        }
    } else {
        // TODO: this path needs to change!
        std::vector<std::complex<float>> stream = {symbol};
        auto bits = symbols_to_bitstream<231>(stream.cbegin());
        for (const auto& bit : bits) {
            bit_stream_decoder_->process_bit(bit);
        }
    }
}
