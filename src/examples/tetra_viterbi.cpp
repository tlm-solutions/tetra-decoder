#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <vector>

#include "viter_bi_codec.hpp"
#include "viterbi/viterbi_decoder_scalar.h"

auto viterbi = std::vector<uint8_t>(
    {1, 0, 2, 2, 0, 2, 2, 2, 1, 0, 2, 2, 1, 2, 2, 2, 1, 1, 2, 2, 1, 2, 2, 2, 0, 0, 2, 2, 1, 2, 2, 2, 1, 0, 2, 2, 1, 2,
     2, 2, 0, 1, 2, 2, 1, 2, 2, 2, 0, 0, 2, 2, 0, 2, 2, 2, 0, 1, 2, 2, 0, 2, 2, 2, 1, 1, 2, 2, 1, 2, 2, 2, 0, 0, 2, 2,
     0, 2, 2, 2, 1, 0, 2, 2, 1, 2, 2, 2, 1, 1, 2, 2, 1, 2, 2, 2, 0, 1, 2, 2, 1, 2, 2, 2, 0, 0, 2, 2, 0, 2, 2, 2, 0, 1,
     2, 2, 1, 2, 2, 2, 0, 0, 2, 2, 1, 2, 2, 2, 0, 1, 2, 2, 1, 2, 2, 2, 1, 0, 2, 2, 0, 2, 2, 2, 0, 1, 2, 2, 1, 2, 2, 2,
     0, 1, 2, 2, 0, 2, 2, 2, 0, 1, 2, 2, 1, 2, 2, 2, 0, 1, 2, 2, 1, 2, 2, 2, 1, 1, 2, 2, 1, 2, 2, 2, 1, 0, 2, 2, 0, 2,
     2, 2, 0, 1, 2, 2, 0, 2, 2, 2, 0, 1, 2, 2, 0, 2, 2, 2, 0, 0, 2, 2, 0, 2, 2, 2, 1, 1, 2, 2, 1, 2, 2, 2, 1, 1, 2, 2,
     0, 2, 2, 2, 0, 0, 2, 2, 0, 2, 2, 2, 0, 0, 2, 2, 1, 2, 2, 2, 0, 0, 2, 2, 0, 2, 2, 2, 1, 1, 2, 2, 0, 2, 2, 2, 0, 1,
     2, 2, 0, 2, 2, 2, 0, 0, 2, 2, 1, 2, 2, 2, 1, 0, 2, 2, 1, 2, 2, 2, 1, 0, 2, 2, 1, 2, 2, 2, 0, 0, 2, 2, 0, 2, 2, 2,
     0, 0, 2, 2, 0, 2, 2, 2, 0, 1, 2, 2, 1, 2, 2, 2, 0, 0, 2, 2, 0, 2, 2, 2, 0, 0, 2, 2, 0, 2, 2, 2, 1, 1, 2, 2, 0, 2,
     2, 2, 1, 1, 2, 2, 1, 2, 2, 2, 1, 1, 2, 2, 0, 2, 2, 2, 1, 1, 2, 2, 0, 2, 2, 2, 1, 1, 2, 2, 1, 2, 2, 2, 0, 0, 2, 2,
     1, 2, 2, 2, 0, 0, 2, 2, 0, 2, 2, 2, 1, 1, 2, 2, 1, 2, 2, 2, 1, 1, 2, 2, 1, 2, 2, 2, 0, 1, 2, 2, 1, 2, 2, 2, 1, 1,
     2, 2, 0, 2, 2, 2, 1, 0, 2, 2, 0, 2, 2, 2, 0, 1, 2, 2, 0, 2, 2, 2, 1, 1, 2, 2, 0, 2, 2, 2, 0, 0, 2, 2, 1, 2, 2, 2,
     1, 0, 2, 2, 0, 2, 2, 2, 1, 1, 2, 2, 0, 2, 2, 2, 1, 0, 2, 2, 0, 2, 2, 2, 0, 1, 2, 2, 0, 2, 2, 2, 0, 0, 2, 2, 1, 2,
     2, 2, 1, 1, 2, 2, 1, 2, 2, 2, 0, 0, 2, 2, 1, 2, 2, 2, 1, 0, 2, 2, 0, 2, 2, 2, 1, 1, 2, 2, 1, 2, 2, 2, 1, 1, 2, 2,
     1, 2, 2, 2, 1, 0, 2, 2, 0, 2, 2, 2, 1, 1, 2, 2, 0, 2, 2, 2, 0, 0, 2, 2, 0, 2, 2, 2, 0, 1, 2, 2, 0, 2, 2, 2, 1, 0,
     2, 2, 0, 2, 2, 2, 1, 0, 2, 2, 1, 2, 2, 2, 1, 0, 2, 2, 1, 2, 2, 2, 1, 0, 2, 2, 1, 2, 2, 2, 0, 1, 2, 2, 1, 2, 2, 2,
     1, 0, 2, 2, 0, 2, 2, 2, 0, 1, 2, 2, 0, 2, 2, 2, 0, 1, 2, 2, 1, 2, 2, 2, 1, 1, 2, 2, 0, 2, 2, 2, 0, 1, 2, 2, 1, 2,
     2, 2, 1, 0, 2, 2, 0, 2, 2, 2, 0, 1, 2, 2, 1, 2, 2, 2, 0, 0, 2, 2, 0, 2, 2, 2, 0, 1, 2, 2, 1, 2, 2, 2, 1, 0, 2, 2,
     1, 2, 2, 2, 0, 0, 2, 2, 1, 2, 2, 2, 1, 0, 2, 2, 0, 2, 2, 2, 0, 0, 2, 2, 0, 2, 2, 2, 0, 0, 2, 2, 1, 2, 2, 2, 0, 0,
     2, 2, 0, 2, 2, 2, 0, 1, 2, 2, 0, 2, 2, 2, 1, 0, 2, 2, 1, 2, 2, 2, 0, 1, 2, 2, 0, 2, 2, 2, 1, 0, 2, 2, 0, 2, 2, 2,
     0, 0, 2, 2, 1, 2, 2, 2, 0, 0, 2, 2, 1, 2, 2, 2, 0, 1, 2, 2, 1, 2, 2, 2, 1, 1, 2, 2, 1, 2, 2, 2, 0, 0, 2, 2, 0, 2,
     2, 2, 0, 0, 2, 2, 1, 2, 2, 2, 0, 1, 2, 2, 1, 2, 2, 2, 1, 0, 2, 2, 0, 2, 2, 2, 1, 1, 2, 2, 1, 2, 2, 2, 1, 0, 2, 2,
     0, 2, 2, 2, 1, 1, 2, 2, 0, 2, 2, 2, 0, 0, 2, 2, 1, 2, 2, 2, 0, 0, 2, 2, 0, 2, 2, 2, 0, 1, 2, 2, 0, 2, 2, 2, 0, 1,
     2, 2, 0, 2, 2, 2, 0, 1, 2, 2, 0, 2, 2, 2, 1, 0, 2, 2, 1, 2, 2, 2, 0, 0, 2, 2, 1, 2, 2, 2, 1, 1, 2, 2, 0, 2, 2, 2,
     0, 1, 2, 2, 1, 2, 2, 2, 1, 0, 2, 2, 0, 2, 2, 2, 0, 1, 2, 2, 0, 2, 2, 2, 1, 1, 2, 2, 0, 2, 2, 2, 1, 1, 2, 2, 1, 2,
     2, 2, 1, 1, 2, 2, 1, 2, 2, 2, 1, 0, 2, 2, 1, 2, 2, 2, 1, 1, 2, 2, 1, 2, 2, 2, 1, 0, 2, 2, 0, 2, 2, 2, 0, 0, 2, 2,
     0, 2, 2, 2, 1, 1, 2, 2, 1, 2, 2, 2, 0, 1, 2, 2, 0, 2, 2, 2, 1, 0, 2, 2, 0, 2, 2, 2, 0, 1, 2, 2, 0, 2, 2, 2, 1, 0,
     2, 2, 1, 2, 2, 2, 1, 0, 2, 2, 0, 2, 2, 2, 1, 1, 2, 2, 0, 2, 2, 2, 1, 1, 2, 2, 0, 2, 2, 2, 1, 1, 2, 2, 0, 2, 2, 2,
     0, 1, 2, 2, 1, 2, 2, 2, 0, 0, 2, 2, 1, 2, 2, 2, 0, 1, 2, 2, 1, 2, 2, 2, 0, 1, 2, 2, 0, 2, 2, 2, 1, 0, 2, 2, 1, 2,
     2, 2, 1, 0, 2, 2, 1, 2, 2, 2, 0, 1, 2, 2, 1, 2, 2, 2, 1, 1, 2, 2, 1, 2, 2, 2, 1, 1, 2, 2, 1, 2, 2, 2, 1, 1, 2, 2,
     0, 2, 2, 2, 0, 1, 2, 2, 0, 2, 2, 2});

auto data = std::vector<uint8_t>(
    {1, 1, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1,
     0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1,
     1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0,
     0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0,
     1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0,
     0, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0,
     1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1,
     0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0});

template <typename soft_t, typename error_t> struct Decoder_Config {
    soft_t soft_decision_high;
    soft_t soft_decision_low;
    ViterbiDecoder_Config<error_t> decoder_config;
};

Decoder_Config<int8_t, uint8_t> get_hard8_decoding_config(const size_t code_rate) {
    const int8_t soft_decision_high = +1;
    const int8_t soft_decision_low = -1;
    const uint8_t max_error = uint8_t(soft_decision_high - soft_decision_low) * uint8_t(code_rate);
    const uint8_t error_margin = max_error * uint8_t(3u);

    ViterbiDecoder_Config<uint8_t> config;
    config.soft_decision_max_error = max_error;
    config.initial_start_error = std::numeric_limits<uint8_t>::min();
    config.initial_non_start_error = config.initial_start_error + error_margin;
    config.renormalisation_threshold = std::numeric_limits<uint8_t>::max() - error_margin;

    return {soft_decision_high, soft_decision_low, config};
}

std::vector<uint8_t> new_viterbi_decode(const std::vector<int8_t>& bits, std::size_t end_state) {
    constexpr size_t K = 5;
    constexpr size_t R = 4;

    const std::vector<uint8_t> G = {19, 29, 23, 27};

    auto config = get_hard8_decoding_config(R);
    auto branch_table = ViterbiBranchTable<K, R, int8_t>(G.data(), config.soft_decision_high, config.soft_decision_low);
    auto vitdec = ViterbiDecoder_Core<K, R, uint8_t, int8_t>(branch_table, config.decoder_config);
    using Decoder = ViterbiDecoder_Scalar<K, R, uint8_t, int8_t>;

    const size_t total_bits = bits.size() / R;
    const size_t total_tail_bits = K - 1u;
    const size_t total_data_bits = total_bits - total_tail_bits;
    const size_t total_input_bytes = total_data_bits / 8;

    // std::vector<uint8_t> output_bytes(total_data_bits / 8);
    std::vector<uint8_t> output_bytes;
    output_bytes.resize(total_bits / 8);

    vitdec.set_traceback_length(total_data_bits);

    vitdec.reset();
    Decoder::template update<uint64_t>(vitdec, bits.data(), bits.size());
    vitdec.chainback(output_bytes.data(), total_data_bits, end_state);
    const uint64_t error = vitdec.get_error();
    std::cout << "error=" << error << std::endl;

    return output_bytes;
}

auto main(int argc, char** argv) -> int {
    std::vector<int> polynomials;
    int constraint = 6;

    polynomials.push_back(0b10011);
    polynomials.push_back(0b11101);
    polynomials.push_back(0b10111);
    polynomials.push_back(0b11011);

    auto viter_bi_codec_1614_ = std::make_unique<ViterbiCodec>(constraint, polynomials);

    std::vector<uint8_t> decoded_data;

    {
        std::string string_input;
        for (unsigned char idx : viterbi) {
            string_input += (char)(idx + '0');
        }

        std::string sOut = viter_bi_codec_1614_->Decode(string_input);

        for (char idx : sOut) {
            decoded_data.push_back((uint8_t)(idx - '0'));
        }
    }

    if (decoded_data == data) {
        std::cout << "Decoded data is equal" << std::endl;
    } else {
        std::cout << "Error: Decoded data not is equal" << std::endl;
        return EXIT_FAILURE;
    }

    auto func = [&](std::size_t end_state) {
        std::vector<int8_t> new_viterbi;

        std::transform(viterbi.cbegin(), viterbi.cend(), std::back_inserter(new_viterbi), [](uint8_t v) {
            if (v == 0) {
                return -1;
            } else if (v == 1) {
                return 1;
            } else {
                return 0;
            }
        });

        auto new_decoded_data = new_viterbi_decode(new_viterbi, end_state);

        std::vector<uint8_t> new_decoded_data_mapped;

        for (auto elem : new_decoded_data)
            for (auto i = 0; i < 8; i++)
                new_decoded_data_mapped.push_back(elem & (1 << (7 - i)) ? 1 : 0);

        for (auto elem : new_decoded_data_mapped)
            std::cout << std::to_string(elem) << " ";
        std::cout << std::endl;

        std::cout << new_decoded_data_mapped.size() << std::endl;
    };

    for (auto i = 0; i < 16; i++) {
        std::cout << "End State: " << std::to_string(i) << std::endl;
        func(i);
    }

    for (auto elem : data)
        std::cout << std::to_string(elem) << " ";
    std::cout << std::endl;
    std::cout << data.size() << std::endl;
}
