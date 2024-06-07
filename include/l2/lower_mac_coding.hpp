/*
 *  tetra-kit
 *  Copyright (C) 2020  LarryTh <dev@logami.fr>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include "utils/viter_bi_codec.hpp"
#include <array>
#include <cstdint>
#include <map>
#include <vector>

struct LowerMacCoding {

    /**
     * @brief Fibonacci LFSR descrambling - 8.2.5
     *
     */
    template <std::size_t Size, typename Type>
    static auto descramble(const std::array<Type, Size>& input, uint32_t scrambling_code) noexcept
        -> std::array<Type, Size> {
        static std::map<uint32_t, std::vector<Type>> table_by_scrambling_code;
        std::array<Type, Size> output{};

        static_assert(Size <= 432);

        if (table_by_scrambling_code.count(scrambling_code) == 0) {
            auto& table = table_by_scrambling_code[scrambling_code];
            table.resize(432);
            const uint8_t poly[14] = {32, 26, 23, 22, 16, 12, 11,
                                      10, 8,  7,  5,  4,  2,  1}; // Feedback polynomial - see 8.2.5.2 (8.39)

            uint32_t lfsr = scrambling_code; // linear feedback shift register initialization (=0 + 3
                                             // for BSCH, calculated from Color code ch 19 otherwise)
            for (std::size_t i = 0; i < 432; i++) {
                uint32_t bit = 0;
                // apply poly (Xj + ...)
                for (std::size_t j = 0; j < 14; j++) {
                    bit = bit ^ (lfsr >> (32 - poly[j]));
                }
                bit = bit & 1; // finish apply feedback polynomial (+ 1)
                lfsr = (lfsr >> 1) | (bit << 31);

                table[i] = bit;
            }
        }

        auto& table = table_by_scrambling_code[scrambling_code];

        for (std::size_t i = 0; i < Size; i++) {
            output[i] = input[i] ^ table[i];
        }

        return output;
    }

    /**
     * @brief (K,a) block deinterleaver - 8.2.4
     *
     */
    template <std::size_t Size, typename Type>
    static auto deinterleave(const std::array<Type, Size>& data, const std::size_t a) noexcept
        -> std::array<Type, Size> {
        std::array<Type, Size> res{};
        for (std::size_t i = 0; i < Size; i++) {
            auto k = 1 + (a * (i + 1)) % ViterbiCodec::K;
            res[i] = data[k - 1]; // to interleave: DataOut[i-1] = DataIn[k-1]
        }

        return res;
    }

    /**
     * @brief Depuncture with 2/3 rate - 8.2.3.1.3
     *
     */
    template <std::size_t InSize, typename InType, std::size_t OutSize = 4 * InSize * 2 / 3>
    [[nodiscard]] static auto depuncture23(const std::array<InType, InSize>& data) noexcept
        -> std::array<int16_t, OutSize> {
        const uint8_t P[] = {0, 1, 2, 5};    // 8.2.3.1.3 - P[1..t]
        std::array<int16_t, OutSize> res{0}; // 8.2.3.1.2 with flag 0 for erase bit in Viterbi routine

        uint8_t t = 3;      // 8.2.3.1.3
        uint8_t period = 8; // 8.2.3.1.2

        for (uint32_t j = 1; j <= InSize; j++) {
            uint32_t i = j; // punct->i_func(j);
            uint32_t k =
                period * ((i - 1) / t) + P[i - t * ((i - 1) / t)]; // punct->period * ((i-1)/t) + P[i - t*((i-1)/t)];
            res[k - 1] = data[j - 1] ? 1 : -1;
        }

        return res;
    }

    /**
     * @brief Viterbi decoding of RCPC code 16-state mother code of rate 1/4
     * - 8.2.3.1.1
     *
     */
    template <std::size_t InSize, std::size_t OutSize = InSize / ViterbiCodec::R>
    [[nodiscard]] static auto viter_bi_decode_1614(const ViterbiCodec& codec,
                                                   const std::array<int16_t, InSize>& data) noexcept
        -> std::array<bool, OutSize> {
        auto decoded = codec.Decode(std::vector(data.cbegin(), data.cend()));
        std::array<bool, OutSize> out{};
        for (auto i = 0; i < decoded.size(); i++) {
            for (auto j = 0; j < 8; j++) {
                out[i * 8 + j] = (decoded[i] & (1 << (7 - j))) ? 1 : 0;
            }
        }

        return out;
    }

    /**
     * @brief Reed-Muller decoder and FEC correction 30 bits in, 14 bits out
     *
     * FEC thanks to Lollo Gollo @logollo see "issue #21"
     *
     */

    static auto reed_muller_3014_decode(const std::array<bool, 30>& input) noexcept -> std::array<bool, 14> {
        std::array<bool, 14> output;
        uint8_t q[14][5];

        q[0][0] = input[0];
        q[0][1] = (input[13 + 3] + input[13 + 5] + input[13 + 6] + input[13 + 7] + input[13 + 11]) % 2;
        q[0][2] = (input[13 + 1] + input[13 + 2] + input[13 + 5] + input[13 + 6] + input[13 + 8] + input[13 + 9]) % 2;
        q[0][3] = (input[13 + 2] + input[13 + 3] + input[13 + 4] + input[13 + 5] + input[13 + 9] + input[13 + 10]) % 2;
        q[0][4] = (input[13 + 1] + input[13 + 4] + input[13 + 5] + input[13 + 7] + input[13 + 8] + input[13 + 10] +
                   input[13 + 11]) %
                  2;
        output[0] = (q[0][0] + q[0][1] + q[0][2] + q[0][3] + q[0][4]) >= 3;

        q[1][0] = input[1];
        q[1][1] = (input[13 + 1] + input[13 + 4] + input[13 + 5] + input[13 + 9] + input[13 + 11]) % 2;
        q[1][2] = (input[13 + 1] + input[13 + 2] + input[13 + 5] + input[13 + 6] + input[13 + 7] + input[13 + 10]) % 2;
        q[1][3] = (input[13 + 2] + input[13 + 3] + input[13 + 4] + input[13 + 5] + input[13 + 7] + input[13 + 8]) % 2;
        q[1][4] = (input[13 + 3] + input[13 + 5] + input[13 + 6] + input[13 + 8] + input[13 + 9] + input[13 + 10] +
                   input[13 + 11]) %
                  2;
        output[1] = (q[1][0] + q[1][1] + q[1][2] + q[1][3] + q[1][4]) >= 3;

        q[2][0] = input[2];
        q[2][1] = (input[13 + 2] + input[13 + 5] + input[13 + 8] + input[13 + 10] + input[13 + 11]) % 2;
        q[2][2] = (input[13 + 1] + input[13 + 3] + input[13 + 5] + input[13 + 7] + input[13 + 9] + input[13 + 10]) % 2;
        q[2][3] = (input[13 + 4] + input[13 + 5] + input[13 + 6] + input[13 + 7] + input[13 + 8] + input[13 + 9]) % 2;
        q[2][4] = (input[13 + 1] + input[13 + 2] + input[13 + 3] + input[13 + 4] + input[13 + 5] + input[13 + 6] +
                   input[13 + 11]) %
                  2;
        output[2] = (q[2][0] + q[2][1] + q[2][2] + q[2][3] + q[2][4]) >= 3;

        q[3][0] = input[3];
        q[3][1] =
            (input[13 + 7] + input[13 + 8] + input[13 + 9] + input[13 + 12] + input[13 + 13] + input[13 + 14]) % 2;
        q[3][2] = (input[13 + 1] + input[13 + 2] + input[13 + 3] + input[13 + 11] + input[13 + 12] + input[13 + 13] +
                   input[13 + 14]) %
                  2;
        q[3][3] = (input[13 + 2] + input[13 + 4] + input[13 + 6] + input[13 + 8] + input[13 + 10] + input[13 + 11] +
                   input[13 + 12] + input[13 + 13] + input[13 + 14]) %
                  2;
        q[3][4] = (input[13 + 1] + input[13 + 3] + input[13 + 4] + input[13 + 6] + input[13 + 7] + input[13 + 9] +
                   input[13 + 10] + input[13 + 12] + input[13 + 13] + input[13 + 14]) %
                  2;
        output[3] = (q[3][0] + q[3][1] + q[3][2] + q[3][3] + q[3][4]) >= 3;

        q[4][0] = input[4];
        q[4][1] = (input[13 + 1] + input[13 + 4] + input[13 + 5] + input[13 + 11] + input[13 + 12] + input[13 + 13] +
                   input[13 + 15]) %
                  2;
        q[4][2] = (input[13 + 3] + input[13 + 5] + input[13 + 6] + input[13 + 8] + input[13 + 10] + input[13 + 11] +
                   input[13 + 12] + input[13 + 13] + input[13 + 15]) %
                  2;
        q[4][3] = (input[13 + 1] + input[13 + 2] + input[13 + 5] + input[13 + 6] + input[13 + 7] + input[13 + 9] +
                   input[13 + 10] + input[13 + 12] + input[13 + 13] + input[13 + 15]) %
                  2;
        q[4][4] = (input[13 + 2] + input[13 + 3] + input[13 + 4] + input[13 + 5] + input[13 + 7] + input[13 + 8] +
                   input[13 + 9] + input[13 + 12] + input[13 + 13] + input[13 + 15]) %
                  2;
        output[4] = (q[4][0] + q[4][1] + q[4][2] + q[4][3] + q[4][4]) >= 3;

        q[5][0] = input[5];
        q[5][1] =
            (input[13 + 7] + input[13 + 9] + input[13 + 10] + input[13 + 12] + input[13 + 14] + input[13 + 15]) % 2;
        q[5][2] = (input[13 + 2] + input[13 + 4] + input[13 + 6] + input[13 + 11] + input[13 + 12] + input[13 + 14] +
                   input[13 + 15]) %
                  2;
        q[5][3] = (input[13 + 1] + input[13 + 2] + input[13 + 3] + input[13 + 8] + input[13 + 10] + input[13 + 11] +
                   input[13 + 12] + input[13 + 14] + input[13 + 15]) %
                  2;
        q[5][4] = (input[13 + 1] + input[13 + 3] + input[13 + 4] + input[13 + 6] + input[13 + 7] + input[13 + 8] +
                   input[13 + 9] + input[13 + 12] + input[13 + 14] + input[13 + 15]) %
                  2;
        output[5] = (q[5][0] + q[5][1] + q[5][2] + q[5][3] + q[5][4]) >= 3;

        q[6][0] = input[6];
        q[6][1] = (input[13 + 3] + input[13 + 5] + input[13 + 6] + input[13 + 11] + input[13 + 13] + input[13 + 14] +
                   input[13 + 15]) %
                  2;
        q[6][2] = (input[13 + 1] + input[13 + 4] + input[13 + 5] + input[13 + 8] + input[13 + 10] + input[13 + 11] +
                   input[13 + 13] + input[13 + 14] + input[13 + 15]) %
                  2;
        q[6][3] = (input[13 + 1] + input[13 + 2] + input[13 + 5] + input[13 + 6] + input[13 + 7] + input[13 + 8] +
                   input[13 + 9] + input[13 + 13] + input[13 + 14] + input[13 + 15]) %
                  2;
        q[6][4] = (input[13 + 2] + input[13 + 3] + input[13 + 4] + input[13 + 5] + input[13 + 7] + input[13 + 9] +
                   input[13 + 10] + input[13 + 13] + input[13 + 14] + input[13 + 15]) %
                  2;
        output[6] = (q[6][0] + q[6][1] + q[6][2] + q[6][3] + q[6][4]) >= 3;

        q[7][0] = input[7];
        q[7][1] = (input[13 + 2] + input[13 + 5] + input[13 + 7] + input[13 + 9] + input[13 + 12] + input[13 + 13] +
                   input[13 + 14] + input[13 + 15] + input[13 + 16]) %
                  2;
        q[7][2] = (input[13 + 1] + input[13 + 3] + input[13 + 5] + input[13 + 8] + input[13 + 11] + input[13 + 12] +
                   input[13 + 13] + input[13 + 14] + input[13 + 15] + input[13 + 16]) %
                  2;
        q[7][3] = (input[13 + 4] + input[13 + 5] + input[13 + 6] + input[13 + 10] + input[13 + 11] + input[13 + 12] +
                   input[13 + 13] + input[13 + 14] + input[13 + 15] + input[13 + 16]) %
                  2;
        q[7][4] = (input[13 + 1] + input[13 + 2] + input[13 + 3] + input[13 + 4] + input[13 + 5] + input[13 + 6] +
                   input[13 + 7] + input[13 + 8] + input[13 + 9] + input[13 + 10] + input[13 + 12] + input[13 + 13] +
                   input[13 + 14] + input[13 + 15] + input[13 + 16]) %
                  2;
        output[7] = (q[7][0] + q[7][1] + q[7][2] + q[7][3] + q[7][4]) >= 3;

        q[8][0] = input[8];
        q[8][1] =
            (input[13 + 2] + input[13 + 3] + input[13 + 9] + input[13 + 12] + input[13 + 13] + input[13 + 16]) % 2;
        q[8][2] = (input[13 + 1] + input[13 + 7] + input[13 + 8] + input[13 + 11] + input[13 + 12] + input[13 + 13] +
                   input[13 + 16]) %
                  2;
        q[8][3] = (input[13 + 3] + input[13 + 4] + input[13 + 6] + input[13 + 7] + input[13 + 10] + input[13 + 11] +
                   input[13 + 12] + input[13 + 13] + input[13 + 16]) %
                  2;
        q[8][4] = (input[13 + 1] + input[13 + 2] + input[13 + 4] + input[13 + 6] + input[13 + 8] + input[13 + 9] +
                   input[13 + 10] + input[13 + 12] + input[13 + 13] + input[13 + 16]) %
                  2;
        output[8] = (q[8][0] + q[8][1] + q[8][2] + q[8][3] + q[8][4]) >= 3;

        q[9][0] = input[9];
        q[9][1] =
            (input[13 + 1] + input[13 + 3] + input[13 + 8] + input[13 + 12] + input[13 + 14] + input[13 + 16]) % 2;
        q[9][2] =
            (input[13 + 4] + input[13 + 6] + input[13 + 10] + input[13 + 12] + input[13 + 14] + input[13 + 16]) % 2;
        q[9][3] = (input[13 + 2] + input[13 + 7] + input[13 + 9] + input[13 + 11] + input[13 + 12] + input[13 + 14] +
                   input[13 + 16]) %
                  2;
        q[9][4] = (input[13 + 1] + input[13 + 2] + input[13 + 3] + input[13 + 4] + input[13 + 6] + input[13 + 7] +
                   input[13 + 8] + input[13 + 9] + input[13 + 10] + input[13 + 11] + input[13 + 12] + input[13 + 14] +
                   input[13 + 16]) %
                  2;
        output[9] = (q[9][0] + q[9][1] + q[9][2] + q[9][3] + q[9][4]) >= 3;

        q[10][0] = input[10];
        q[10][1] =
            (input[13 + 1] + input[13 + 2] + input[13 + 7] + input[13 + 13] + input[13 + 14] + input[13 + 16]) % 2;
        q[10][2] = (input[13 + 3] + input[13 + 8] + input[13 + 9] + input[13 + 11] + input[13 + 13] + input[13 + 14] +
                    input[13 + 16]) %
                   2;
        q[10][3] = (input[13 + 1] + input[13 + 4] + input[13 + 6] + input[13 + 9] + input[13 + 10] + input[13 + 11] +
                    input[13 + 13] + input[13 + 14] + input[13 + 16]) %
                   2;
        q[10][4] = (input[13 + 2] + input[13 + 3] + input[13 + 4] + input[13 + 6] + input[13 + 7] + input[13 + 8] +
                    input[13 + 10] + input[13 + 13] + input[13 + 14] + input[13 + 16]) %
                   2;
        output[10] = (q[10][0] + q[10][1] + q[10][2] + q[10][3] + q[10][4]) >= 3;

        q[11][0] = input[11];
        q[11][1] =
            (input[13 + 2] + input[13 + 6] + input[13 + 9] + input[13 + 12] + input[13 + 15] + input[13 + 16]) % 2;
        q[11][2] = (input[13 + 4] + input[13 + 7] + input[13 + 10] + input[13 + 11] + input[13 + 12] + input[13 + 15] +
                    input[13 + 16]) %
                   2;
        q[11][3] = (input[13 + 1] + input[13 + 3] + input[13 + 6] + input[13 + 7] + input[13 + 8] + input[13 + 11] +
                    input[13 + 12] + input[13 + 15] + input[13 + 16]) %
                   2;
        q[11][4] = (input[13 + 1] + input[13 + 2] + input[13 + 3] + input[13 + 4] + input[13 + 8] + input[13 + 9] +
                    input[13 + 10] + input[13 + 12] + input[13 + 15] + input[13 + 16]) %
                   2;
        output[11] = (q[11][0] + q[11][1] + q[11][2] + q[11][3] + q[11][4]) >= 3;

        q[12][0] = input[12];
        q[12][1] = (input[13 + 5] + input[13 + 8] + input[13 + 10] + input[13 + 11] + input[13 + 13] + input[13 + 15] +
                    input[13 + 16]) %
                   2;
        q[12][2] = (input[13 + 1] + input[13 + 3] + input[13 + 4] + input[13 + 5] + input[13 + 6] + input[13 + 11] +
                    input[13 + 13] + input[13 + 15] + input[13 + 16]) %
                   2;
        q[12][3] = (input[13 + 1] + input[13 + 2] + input[13 + 3] + input[13 + 5] + input[13 + 7] + input[13 + 9] +
                    input[13 + 10] + input[13 + 13] + input[13 + 15] + input[13 + 16]) %
                   2;
        q[12][4] = (input[13 + 2] + input[13 + 4] + input[13 + 5] + input[13 + 6] + input[13 + 7] + input[13 + 8] +
                    input[13 + 9] + input[13 + 13] + input[13 + 15] + input[13 + 16]) %
                   2;
        output[12] = (q[12][0] + q[12][1] + q[12][2] + q[12][3] + q[12][4]) >= 3;

        q[13][0] = input[13];
        q[13][1] =
            (input[13 + 2] + input[13 + 4] + input[13 + 7] + input[13 + 14] + input[13 + 15] + input[13 + 16]) % 2;
        q[13][2] = (input[13 + 6] + input[13 + 9] + input[13 + 10] + input[13 + 11] + input[13 + 14] + input[13 + 15] +
                    input[13 + 16]) %
                   2;
        q[13][3] = (input[13 + 1] + input[13 + 3] + input[13 + 4] + input[13 + 8] + input[13 + 9] + input[13 + 11] +
                    input[13 + 14] + input[13 + 15] + input[13 + 16]) %
                   2;
        q[13][4] = (input[13 + 1] + input[13 + 2] + input[13 + 3] + input[13 + 6] + input[13 + 7] + input[13 + 8] +
                    input[13 + 10] + input[13 + 14] + input[13 + 15] + input[13 + 16]) %
                   2;
        output[13] = (q[13][0] + q[13][1] + q[13][2] + q[13][3] + q[13][4]) >= 3;

        // check deviation from input
        // int deviation = 0;
        // for (int cnt = 0; cnt < 14; cnt++)
        // {
        //     deviation += (input[cnt] != output[cnt]) ? 1 : 0;
        // }
        // printf("FEC correction %.2f\n", deviation / 14.);
        // print_vector(data, 14);
        // print_vector(res, 14);

        // return vector_extract(data, 0, 14);

        return output;
    }

    /**
     * @brief Calculated CRC16 ITU-T X.25 - CCITT
     *
     */
    template <std::size_t CheckSize, std::size_t InSize>
    [[nodiscard]] static auto check_crc_16_ccitt(const std::array<bool, InSize>& data) noexcept -> bool {
        uint16_t crc = 0xFFFF; // CRC16-CCITT initial value

        for (std::size_t i = 0; i < CheckSize; i++) {
            auto bit = static_cast<uint16_t>(data[i]);

            crc ^= bit << 15;
            if (crc & 0x8000) {
                crc <<= 1;
                crc ^= 0x1021; // CRC16-CCITT polynomial
            } else {
                crc <<= 1;
            }
        }

        return crc == 0x1D0F; // CRC16-CCITT reminder value
    }
};