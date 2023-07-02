/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#include <iq_stream_decoder.hpp>

void IQStreamDecoder::process_complex(std::complex<float> symbol) noexcept {
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
