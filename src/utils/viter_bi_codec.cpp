/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#include <utils/viter_bi_codec.hpp>

std::vector<uint8_t> ViterbiCodec::Decode(const std::vector<int16_t>& bits) const {
    auto vitdec = ViterbiDecoder_Core<K, R, uint16_t, int16_t>(branch_table, config);
    using Decoder = ViterbiDecoder_SSE_u16<K, R>;

    const size_t total_bits = bits.size() / R;
    const size_t total_tail_bits = K - 1u;
    const size_t total_data_bits = total_bits - total_tail_bits;

    std::vector<uint8_t> output_bytes(total_bits / 8);

    vitdec.set_traceback_length(total_data_bits);

    vitdec.reset();
    Decoder::template update<uint64_t>(vitdec, bits.data(), bits.size());
    vitdec.chainback(output_bytes.data(), total_data_bits, 0);
    const uint64_t error = vitdec.get_error();
    // std::cout << "error=" << error << std::endl;

    return output_bytes;
}
