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
#include <l2/lower_mac.hpp>

/**
 * @brief Fibonacci LFSR descrambling - 8.2.5
 *
 */
static void xor_kernel(uint8_t* const res, const uint8_t* const data, const uint8_t* const table,
                       const std::size_t len) {
    for (std::size_t i = 0; i < len; i++)
        res[i] = data[i] ^ table[i];
}

auto LowerMac::descramble(const std::vector<uint8_t>& data, const std::size_t len,
                          const uint32_t scramblingCode) noexcept -> std::vector<uint8_t> {

    static std::vector<uint8_t> table;
    std::vector<uint8_t> res(len);

    assert(len <= 432);

    if (table.size() == 0) {
        table.resize(432);
        const uint8_t poly[14] = {32, 26, 23, 22, 16, 12, 11,
                                  10, 8,  7,  5,  4,  2,  1}; // Feedback polynomial - see 8.2.5.2 (8.39)

        uint32_t lfsr = scramblingCode; // linear feedback shift register initialization (=0 + 3
                                        // for BSCH, calculated from Color code ch 19 otherwise)
        for (std::size_t i = 0; i < 432; i++) {
            uint32_t bit = 0;
            // apply poly (Xj + ...)
            for (std::size_t j = 0; j < 14; j++) {
                bit = bit ^ (lfsr >> (32 - poly[j]));
            }
            bit = bit & 1; // finish apply feedback polynomial (+ 1)
            lfsr = (lfsr >> 1) | (bit << 31);

            table[i] = bit & 0xff;
        }
    }

    xor_kernel(res.data(), data.data(), table.data(), len);

    return res;
}

/**
 * @brief (K,a) block deinterleaver - 8.2.4
 *
 */
auto LowerMac::deinterleave(const std::vector<uint8_t>& data, const std::size_t K, const std::size_t a) noexcept
    -> std::vector<uint8_t> {
    std::vector<uint8_t> res(K, 0); // output vector is size K

    for (std::size_t i = 0; i < K; i++) {
        auto k = 1 + (a * (i + 1)) % K;
        res[i] = data[k - 1]; // to interleave: DataOut[i-1] = DataIn[k-1]
    }

    return res;
}

/**
 * @brief Depuncture with 2/3 rate - 8.2.3.1.3
 *
 */
auto LowerMac::depuncture23(const std::vector<uint8_t>& data, const uint32_t len) noexcept -> std::vector<int16_t> {
    const uint8_t P[] = {0, 1, 2, 5}; // 8.2.3.1.3 - P[1..t]
    std::vector<int16_t> res(4 * len * 2 / 3,
                             0); // 8.2.3.1.2 with flag 0 for erase bit in Viterbi routine

    uint8_t t = 3;      // 8.2.3.1.3
    uint8_t period = 8; // 8.2.3.1.2

    for (uint32_t j = 1; j <= len; j++) {
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
auto LowerMac::viter_bi_decode_1614(const std::vector<int16_t>& data) const noexcept -> std::vector<uint8_t> {
    auto decoded = viter_bi_codec_1614_->Decode(data);
    std::vector<std::uint8_t> out(decoded.size() * 8);
    for (auto i = 0; i < decoded.size(); i++)
        for (auto j = 0; j < 8; j++)
            out[i * 8 + j] = (decoded[i] & (1 << (7 - j))) ? 1 : 0;

    return out;
}

/**
 * @brief Reed-Muller decoder and FEC correction 30 bits in, 14 bits out
 *
 * FEC thanks to Lollo Gollo @logollo see "issue #21"
 *
 */

auto LowerMac::reed_muller_3014_decode(const std::vector<uint8_t>& data) noexcept -> std::vector<uint8_t> {
    uint8_t q[14][5];
    std::vector<uint8_t> res(14);

    q[0][0] = data[0];
    q[0][1] = (data[13 + 3] + data[13 + 5] + data[13 + 6] + data[13 + 7] + data[13 + 11]) % 2;
    q[0][2] = (data[13 + 1] + data[13 + 2] + data[13 + 5] + data[13 + 6] + data[13 + 8] + data[13 + 9]) % 2;
    q[0][3] = (data[13 + 2] + data[13 + 3] + data[13 + 4] + data[13 + 5] + data[13 + 9] + data[13 + 10]) % 2;
    q[0][4] =
        (data[13 + 1] + data[13 + 4] + data[13 + 5] + data[13 + 7] + data[13 + 8] + data[13 + 10] + data[13 + 11]) % 2;
    res[0] = (q[0][0] + q[0][1] + q[0][2] + q[0][3] + q[0][4]) >= 3 ? 1 : 0;

    q[1][0] = data[1];
    q[1][1] = (data[13 + 1] + data[13 + 4] + data[13 + 5] + data[13 + 9] + data[13 + 11]) % 2;
    q[1][2] = (data[13 + 1] + data[13 + 2] + data[13 + 5] + data[13 + 6] + data[13 + 7] + data[13 + 10]) % 2;
    q[1][3] = (data[13 + 2] + data[13 + 3] + data[13 + 4] + data[13 + 5] + data[13 + 7] + data[13 + 8]) % 2;
    q[1][4] =
        (data[13 + 3] + data[13 + 5] + data[13 + 6] + data[13 + 8] + data[13 + 9] + data[13 + 10] + data[13 + 11]) % 2;
    res[1] = (q[1][0] + q[1][1] + q[1][2] + q[1][3] + q[1][4]) >= 3 ? 1 : 0;

    q[2][0] = data[2];
    q[2][1] = (data[13 + 2] + data[13 + 5] + data[13 + 8] + data[13 + 10] + data[13 + 11]) % 2;
    q[2][2] = (data[13 + 1] + data[13 + 3] + data[13 + 5] + data[13 + 7] + data[13 + 9] + data[13 + 10]) % 2;
    q[2][3] = (data[13 + 4] + data[13 + 5] + data[13 + 6] + data[13 + 7] + data[13 + 8] + data[13 + 9]) % 2;
    q[2][4] =
        (data[13 + 1] + data[13 + 2] + data[13 + 3] + data[13 + 4] + data[13 + 5] + data[13 + 6] + data[13 + 11]) % 2;
    res[2] = (q[2][0] + q[2][1] + q[2][2] + q[2][3] + q[2][4]) >= 3 ? 1 : 0;

    q[3][0] = data[3];
    q[3][1] = (data[13 + 7] + data[13 + 8] + data[13 + 9] + data[13 + 12] + data[13 + 13] + data[13 + 14]) % 2;
    q[3][2] =
        (data[13 + 1] + data[13 + 2] + data[13 + 3] + data[13 + 11] + data[13 + 12] + data[13 + 13] + data[13 + 14]) %
        2;
    q[3][3] = (data[13 + 2] + data[13 + 4] + data[13 + 6] + data[13 + 8] + data[13 + 10] + data[13 + 11] +
               data[13 + 12] + data[13 + 13] + data[13 + 14]) %
              2;
    q[3][4] = (data[13 + 1] + data[13 + 3] + data[13 + 4] + data[13 + 6] + data[13 + 7] + data[13 + 9] + data[13 + 10] +
               data[13 + 12] + data[13 + 13] + data[13 + 14]) %
              2;
    res[3] = (q[3][0] + q[3][1] + q[3][2] + q[3][3] + q[3][4]) >= 3 ? 1 : 0;

    q[4][0] = data[4];
    q[4][1] =
        (data[13 + 1] + data[13 + 4] + data[13 + 5] + data[13 + 11] + data[13 + 12] + data[13 + 13] + data[13 + 15]) %
        2;
    q[4][2] = (data[13 + 3] + data[13 + 5] + data[13 + 6] + data[13 + 8] + data[13 + 10] + data[13 + 11] +
               data[13 + 12] + data[13 + 13] + data[13 + 15]) %
              2;
    q[4][3] = (data[13 + 1] + data[13 + 2] + data[13 + 5] + data[13 + 6] + data[13 + 7] + data[13 + 9] + data[13 + 10] +
               data[13 + 12] + data[13 + 13] + data[13 + 15]) %
              2;
    q[4][4] = (data[13 + 2] + data[13 + 3] + data[13 + 4] + data[13 + 5] + data[13 + 7] + data[13 + 8] + data[13 + 9] +
               data[13 + 12] + data[13 + 13] + data[13 + 15]) %
              2;
    res[4] = (q[4][0] + q[4][1] + q[4][2] + q[4][3] + q[4][4]) >= 3 ? 1 : 0;

    q[5][0] = data[5];
    q[5][1] = (data[13 + 7] + data[13 + 9] + data[13 + 10] + data[13 + 12] + data[13 + 14] + data[13 + 15]) % 2;
    q[5][2] =
        (data[13 + 2] + data[13 + 4] + data[13 + 6] + data[13 + 11] + data[13 + 12] + data[13 + 14] + data[13 + 15]) %
        2;
    q[5][3] = (data[13 + 1] + data[13 + 2] + data[13 + 3] + data[13 + 8] + data[13 + 10] + data[13 + 11] +
               data[13 + 12] + data[13 + 14] + data[13 + 15]) %
              2;
    q[5][4] = (data[13 + 1] + data[13 + 3] + data[13 + 4] + data[13 + 6] + data[13 + 7] + data[13 + 8] + data[13 + 9] +
               data[13 + 12] + data[13 + 14] + data[13 + 15]) %
              2;
    res[5] = (q[5][0] + q[5][1] + q[5][2] + q[5][3] + q[5][4]) >= 3 ? 1 : 0;

    q[6][0] = data[6];
    q[6][1] =
        (data[13 + 3] + data[13 + 5] + data[13 + 6] + data[13 + 11] + data[13 + 13] + data[13 + 14] + data[13 + 15]) %
        2;
    q[6][2] = (data[13 + 1] + data[13 + 4] + data[13 + 5] + data[13 + 8] + data[13 + 10] + data[13 + 11] +
               data[13 + 13] + data[13 + 14] + data[13 + 15]) %
              2;
    q[6][3] = (data[13 + 1] + data[13 + 2] + data[13 + 5] + data[13 + 6] + data[13 + 7] + data[13 + 8] + data[13 + 9] +
               data[13 + 13] + data[13 + 14] + data[13 + 15]) %
              2;
    q[6][4] = (data[13 + 2] + data[13 + 3] + data[13 + 4] + data[13 + 5] + data[13 + 7] + data[13 + 9] + data[13 + 10] +
               data[13 + 13] + data[13 + 14] + data[13 + 15]) %
              2;
    res[6] = (q[6][0] + q[6][1] + q[6][2] + q[6][3] + q[6][4]) >= 3 ? 1 : 0;

    q[7][0] = data[7];
    q[7][1] = (data[13 + 2] + data[13 + 5] + data[13 + 7] + data[13 + 9] + data[13 + 12] + data[13 + 13] +
               data[13 + 14] + data[13 + 15] + data[13 + 16]) %
              2;
    q[7][2] = (data[13 + 1] + data[13 + 3] + data[13 + 5] + data[13 + 8] + data[13 + 11] + data[13 + 12] +
               data[13 + 13] + data[13 + 14] + data[13 + 15] + data[13 + 16]) %
              2;
    q[7][3] = (data[13 + 4] + data[13 + 5] + data[13 + 6] + data[13 + 10] + data[13 + 11] + data[13 + 12] +
               data[13 + 13] + data[13 + 14] + data[13 + 15] + data[13 + 16]) %
              2;
    q[7][4] = (data[13 + 1] + data[13 + 2] + data[13 + 3] + data[13 + 4] + data[13 + 5] + data[13 + 6] + data[13 + 7] +
               data[13 + 8] + data[13 + 9] + data[13 + 10] + data[13 + 12] + data[13 + 13] + data[13 + 14] +
               data[13 + 15] + data[13 + 16]) %
              2;
    res[7] = (q[7][0] + q[7][1] + q[7][2] + q[7][3] + q[7][4]) >= 3 ? 1 : 0;

    q[8][0] = data[8];
    q[8][1] = (data[13 + 2] + data[13 + 3] + data[13 + 9] + data[13 + 12] + data[13 + 13] + data[13 + 16]) % 2;
    q[8][2] =
        (data[13 + 1] + data[13 + 7] + data[13 + 8] + data[13 + 11] + data[13 + 12] + data[13 + 13] + data[13 + 16]) %
        2;
    q[8][3] = (data[13 + 3] + data[13 + 4] + data[13 + 6] + data[13 + 7] + data[13 + 10] + data[13 + 11] +
               data[13 + 12] + data[13 + 13] + data[13 + 16]) %
              2;
    q[8][4] = (data[13 + 1] + data[13 + 2] + data[13 + 4] + data[13 + 6] + data[13 + 8] + data[13 + 9] + data[13 + 10] +
               data[13 + 12] + data[13 + 13] + data[13 + 16]) %
              2;
    res[8] = (q[8][0] + q[8][1] + q[8][2] + q[8][3] + q[8][4]) >= 3 ? 1 : 0;

    q[9][0] = data[9];
    q[9][1] = (data[13 + 1] + data[13 + 3] + data[13 + 8] + data[13 + 12] + data[13 + 14] + data[13 + 16]) % 2;
    q[9][2] = (data[13 + 4] + data[13 + 6] + data[13 + 10] + data[13 + 12] + data[13 + 14] + data[13 + 16]) % 2;
    q[9][3] =
        (data[13 + 2] + data[13 + 7] + data[13 + 9] + data[13 + 11] + data[13 + 12] + data[13 + 14] + data[13 + 16]) %
        2;
    q[9][4] = (data[13 + 1] + data[13 + 2] + data[13 + 3] + data[13 + 4] + data[13 + 6] + data[13 + 7] + data[13 + 8] +
               data[13 + 9] + data[13 + 10] + data[13 + 11] + data[13 + 12] + data[13 + 14] + data[13 + 16]) %
              2;
    res[9] = (q[9][0] + q[9][1] + q[9][2] + q[9][3] + q[9][4]) >= 3 ? 1 : 0;

    q[10][0] = data[10];
    q[10][1] = (data[13 + 1] + data[13 + 2] + data[13 + 7] + data[13 + 13] + data[13 + 14] + data[13 + 16]) % 2;
    q[10][2] =
        (data[13 + 3] + data[13 + 8] + data[13 + 9] + data[13 + 11] + data[13 + 13] + data[13 + 14] + data[13 + 16]) %
        2;
    q[10][3] = (data[13 + 1] + data[13 + 4] + data[13 + 6] + data[13 + 9] + data[13 + 10] + data[13 + 11] +
                data[13 + 13] + data[13 + 14] + data[13 + 16]) %
               2;
    q[10][4] = (data[13 + 2] + data[13 + 3] + data[13 + 4] + data[13 + 6] + data[13 + 7] + data[13 + 8] +
                data[13 + 10] + data[13 + 13] + data[13 + 14] + data[13 + 16]) %
               2;
    res[10] = (q[10][0] + q[10][1] + q[10][2] + q[10][3] + q[10][4]) >= 3 ? 1 : 0;

    q[11][0] = data[11];
    q[11][1] = (data[13 + 2] + data[13 + 6] + data[13 + 9] + data[13 + 12] + data[13 + 15] + data[13 + 16]) % 2;
    q[11][2] =
        (data[13 + 4] + data[13 + 7] + data[13 + 10] + data[13 + 11] + data[13 + 12] + data[13 + 15] + data[13 + 16]) %
        2;
    q[11][3] = (data[13 + 1] + data[13 + 3] + data[13 + 6] + data[13 + 7] + data[13 + 8] + data[13 + 11] +
                data[13 + 12] + data[13 + 15] + data[13 + 16]) %
               2;
    q[11][4] = (data[13 + 1] + data[13 + 2] + data[13 + 3] + data[13 + 4] + data[13 + 8] + data[13 + 9] +
                data[13 + 10] + data[13 + 12] + data[13 + 15] + data[13 + 16]) %
               2;
    res[11] = (q[11][0] + q[11][1] + q[11][2] + q[11][3] + q[11][4]) >= 3 ? 1 : 0;

    q[12][0] = data[12];
    q[12][1] =
        (data[13 + 5] + data[13 + 8] + data[13 + 10] + data[13 + 11] + data[13 + 13] + data[13 + 15] + data[13 + 16]) %
        2;
    q[12][2] = (data[13 + 1] + data[13 + 3] + data[13 + 4] + data[13 + 5] + data[13 + 6] + data[13 + 11] +
                data[13 + 13] + data[13 + 15] + data[13 + 16]) %
               2;
    q[12][3] = (data[13 + 1] + data[13 + 2] + data[13 + 3] + data[13 + 5] + data[13 + 7] + data[13 + 9] +
                data[13 + 10] + data[13 + 13] + data[13 + 15] + data[13 + 16]) %
               2;
    q[12][4] = (data[13 + 2] + data[13 + 4] + data[13 + 5] + data[13 + 6] + data[13 + 7] + data[13 + 8] + data[13 + 9] +
                data[13 + 13] + data[13 + 15] + data[13 + 16]) %
               2;
    res[12] = (q[12][0] + q[12][1] + q[12][2] + q[12][3] + q[12][4]) >= 3 ? 1 : 0;

    q[13][0] = data[13];
    q[13][1] = (data[13 + 2] + data[13 + 4] + data[13 + 7] + data[13 + 14] + data[13 + 15] + data[13 + 16]) % 2;
    q[13][2] =
        (data[13 + 6] + data[13 + 9] + data[13 + 10] + data[13 + 11] + data[13 + 14] + data[13 + 15] + data[13 + 16]) %
        2;
    q[13][3] = (data[13 + 1] + data[13 + 3] + data[13 + 4] + data[13 + 8] + data[13 + 9] + data[13 + 11] +
                data[13 + 14] + data[13 + 15] + data[13 + 16]) %
               2;
    q[13][4] = (data[13 + 1] + data[13 + 2] + data[13 + 3] + data[13 + 6] + data[13 + 7] + data[13 + 8] +
                data[13 + 10] + data[13 + 14] + data[13 + 15] + data[13 + 16]) %
               2;
    res[13] = (q[13][0] + q[13][1] + q[13][2] + q[13][3] + q[13][4]) >= 3 ? 1 : 0;

    // check deviation from input
    // int deviation = 0;
    // for (int cnt = 0; cnt < 14; cnt++)
    // {
    //     deviation += (data[cnt] != res[cnt]) ? 1 : 0;
    // }
    // printf("FEC correction %.2f\n", deviation / 14.);
    // print_vector(data, 14);
    // print_vector(res, 14);

    // return vector_extract(data, 0, 14);

    return res;
}

/**
 * @brief Calculated CRC16 ITU-T X.25 - CCITT
 *
 */
auto LowerMac::check_crc_16_ccitt(const std::vector<uint8_t>& data, const std::size_t len) noexcept -> bool {
    uint16_t crc = 0xFFFF; // CRC16-CCITT initial value

    for (std::size_t i = 0; i < len; i++) {
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
