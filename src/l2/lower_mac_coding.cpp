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

auto LowerMac::descramble(const std::vector<uint8_t>& data, int len, uint32_t scramblingCode) noexcept
    -> std::vector<uint8_t> {
    const uint8_t poly[14] = {32, 26, 23, 22, 16, 12, 11,
                              10, 8,  7,  5,  4,  2,  1}; // Feedback polynomial - see 8.2.5.2 (8.39)

    std::vector<uint8_t> res;

    uint32_t lfsr = scramblingCode; // linear feedback shift register initialization (=0 + 3
                                    // for BSCH, calculated from Color code ch 19 otherwise)
    for (int i = 0; i < len; i++) {
        uint32_t bit = lfsr >> (32 - poly[0]); // apply poly (Xj + ...)
        for (int j = 1; j < 14; j++) {
            bit = bit ^ (lfsr >> (32 - poly[j]));
        }
        bit = bit & 1; // finish apply feedback polynomial (+ 1)
        lfsr = (lfsr >> 1) | (bit << 31);

        res.push_back(data[i] ^ (bit & 0xff));
    }

    return res;
}

/**
 * @brief (K,a) block deinterleaver - 8.2.4
 *
 */
auto LowerMac::deinterleave(const std::vector<uint8_t>& data, const uint32_t K, const uint32_t a) noexcept
    -> std::vector<uint8_t> {
    std::vector<uint8_t> res(K, 0); // output vector is size K

    for (unsigned int idx = 1; idx <= K; idx++) {
        uint32_t k = 1 + (a * idx) % K;
        res[idx - 1] = data[k - 1]; // to interleave: DataOut[i-1] = DataIn[k-1]
    }

    return res;
}

/**
 * @brief Depuncture with 2/3 rate - 8.2.3.1.3
 *
 */
auto LowerMac::depuncture23(const std::vector<uint8_t>& data, const uint32_t len) noexcept -> std::vector<uint8_t> {
    const uint8_t P[] = {0, 1, 2, 5}; // 8.2.3.1.3 - P[1..t]
    std::vector<uint8_t> res(4 * len * 2 / 3,
                             2); // 8.2.3.1.2 with flag 2 for erase bit in Viterbi routine

    uint8_t t = 3;      // 8.2.3.1.3
    uint8_t period = 8; // 8.2.3.1.2

    for (uint32_t j = 1; j <= len; j++) {
        uint32_t i = j; // punct->i_func(j);
        uint32_t k =
            period * ((i - 1) / t) + P[i - t * ((i - 1) / t)]; // punct->period * ((i-1)/t) + P[i - t*((i-1)/t)];
        res[k - 1] = data[j - 1];
    }

    return res;
}

/**
 * @brief Viterbi decoding of RCPC code 16-state mother code of rate 1/4
 * - 8.2.3.1.1
 *
 */

auto LowerMac::viter_bi_decode_1614(const std::vector<uint8_t>& data) noexcept -> std::vector<uint8_t> {
    std::string string_input;
    for (unsigned char idx : data) {
        string_input += (char)(idx + '0');
    }

    std::string sOut = viter_bi_codec_1614_->decode(string_input);

    std::vector<uint8_t> res;

    for (char idx : sOut) {
        res.push_back((uint8_t)(idx - '0'));
    }

    return res;
}

/**
 * @brief Reed-Muller decoder and FEC correction 30 bits in, 14 bits out
 *
 * FEC thanks to Lollo Gollo @logollo see "issue #21"
 *
 */

auto LowerMac::reed_muller_3014_decode(const std::vector<uint8_t>& data) noexcept -> std::vector<uint8_t> {
    uint8_t q[5];
    std::vector<uint8_t> res(14);

    q[0] = data[0];
    q[1] = (data[13 + 3] + data[13 + 5] + data[13 + 6] + data[13 + 7] + data[13 + 11]) % 2;
    q[2] = (data[13 + 1] + data[13 + 2] + data[13 + 5] + data[13 + 6] + data[13 + 8] + data[13 + 9]) % 2;
    q[3] = (data[13 + 2] + data[13 + 3] + data[13 + 4] + data[13 + 5] + data[13 + 9] + data[13 + 10]) % 2;
    q[4] =
        (data[13 + 1] + data[13 + 4] + data[13 + 5] + data[13 + 7] + data[13 + 8] + data[13 + 10] + data[13 + 11]) % 2;
    res[0] = (q[0] + q[1] + q[2] + q[3] + q[4]) >= 3 ? 1 : 0;

    q[0] = data[1];
    q[1] = (data[13 + 1] + data[13 + 4] + data[13 + 5] + data[13 + 9] + data[13 + 11]) % 2;
    q[2] = (data[13 + 1] + data[13 + 2] + data[13 + 5] + data[13 + 6] + data[13 + 7] + data[13 + 10]) % 2;
    q[3] = (data[13 + 2] + data[13 + 3] + data[13 + 4] + data[13 + 5] + data[13 + 7] + data[13 + 8]) % 2;
    q[4] =
        (data[13 + 3] + data[13 + 5] + data[13 + 6] + data[13 + 8] + data[13 + 9] + data[13 + 10] + data[13 + 11]) % 2;
    res[1] = (q[0] + q[1] + q[2] + q[3] + q[4]) >= 3 ? 1 : 0;

    q[0] = data[2];
    q[1] = (data[13 + 2] + data[13 + 5] + data[13 + 8] + data[13 + 10] + data[13 + 11]) % 2;
    q[2] = (data[13 + 1] + data[13 + 3] + data[13 + 5] + data[13 + 7] + data[13 + 9] + data[13 + 10]) % 2;
    q[3] = (data[13 + 4] + data[13 + 5] + data[13 + 6] + data[13 + 7] + data[13 + 8] + data[13 + 9]) % 2;
    q[4] =
        (data[13 + 1] + data[13 + 2] + data[13 + 3] + data[13 + 4] + data[13 + 5] + data[13 + 6] + data[13 + 11]) % 2;
    res[2] = (q[0] + q[1] + q[2] + q[3] + q[4]) >= 3 ? 1 : 0;

    q[0] = data[3];
    q[1] = (data[13 + 7] + data[13 + 8] + data[13 + 9] + data[13 + 12] + data[13 + 13] + data[13 + 14]) % 2;
    q[2] =
        (data[13 + 1] + data[13 + 2] + data[13 + 3] + data[13 + 11] + data[13 + 12] + data[13 + 13] + data[13 + 14]) %
        2;
    q[3] = (data[13 + 2] + data[13 + 4] + data[13 + 6] + data[13 + 8] + data[13 + 10] + data[13 + 11] + data[13 + 12] +
            data[13 + 13] + data[13 + 14]) %
           2;
    q[4] = (data[13 + 1] + data[13 + 3] + data[13 + 4] + data[13 + 6] + data[13 + 7] + data[13 + 9] + data[13 + 10] +
            data[13 + 12] + data[13 + 13] + data[13 + 14]) %
           2;
    res[3] = (q[0] + q[1] + q[2] + q[3] + q[4]) >= 3 ? 1 : 0;

    q[0] = data[4];
    q[1] =
        (data[13 + 1] + data[13 + 4] + data[13 + 5] + data[13 + 11] + data[13 + 12] + data[13 + 13] + data[13 + 15]) %
        2;
    q[2] = (data[13 + 3] + data[13 + 5] + data[13 + 6] + data[13 + 8] + data[13 + 10] + data[13 + 11] + data[13 + 12] +
            data[13 + 13] + data[13 + 15]) %
           2;
    q[3] = (data[13 + 1] + data[13 + 2] + data[13 + 5] + data[13 + 6] + data[13 + 7] + data[13 + 9] + data[13 + 10] +
            data[13 + 12] + data[13 + 13] + data[13 + 15]) %
           2;
    q[4] = (data[13 + 2] + data[13 + 3] + data[13 + 4] + data[13 + 5] + data[13 + 7] + data[13 + 8] + data[13 + 9] +
            data[13 + 12] + data[13 + 13] + data[13 + 15]) %
           2;
    res[4] = (q[0] + q[1] + q[2] + q[3] + q[4]) >= 3 ? 1 : 0;

    q[0] = data[5];
    q[1] = (data[13 + 7] + data[13 + 9] + data[13 + 10] + data[13 + 12] + data[13 + 14] + data[13 + 15]) % 2;
    q[2] =
        (data[13 + 2] + data[13 + 4] + data[13 + 6] + data[13 + 11] + data[13 + 12] + data[13 + 14] + data[13 + 15]) %
        2;
    q[3] = (data[13 + 1] + data[13 + 2] + data[13 + 3] + data[13 + 8] + data[13 + 10] + data[13 + 11] + data[13 + 12] +
            data[13 + 14] + data[13 + 15]) %
           2;
    q[4] = (data[13 + 1] + data[13 + 3] + data[13 + 4] + data[13 + 6] + data[13 + 7] + data[13 + 8] + data[13 + 9] +
            data[13 + 12] + data[13 + 14] + data[13 + 15]) %
           2;
    res[5] = (q[0] + q[1] + q[2] + q[3] + q[4]) >= 3 ? 1 : 0;

    q[0] = data[6];
    q[1] =
        (data[13 + 3] + data[13 + 5] + data[13 + 6] + data[13 + 11] + data[13 + 13] + data[13 + 14] + data[13 + 15]) %
        2;
    q[2] = (data[13 + 1] + data[13 + 4] + data[13 + 5] + data[13 + 8] + data[13 + 10] + data[13 + 11] + data[13 + 13] +
            data[13 + 14] + data[13 + 15]) %
           2;
    q[3] = (data[13 + 1] + data[13 + 2] + data[13 + 5] + data[13 + 6] + data[13 + 7] + data[13 + 8] + data[13 + 9] +
            data[13 + 13] + data[13 + 14] + data[13 + 15]) %
           2;
    q[4] = (data[13 + 2] + data[13 + 3] + data[13 + 4] + data[13 + 5] + data[13 + 7] + data[13 + 9] + data[13 + 10] +
            data[13 + 13] + data[13 + 14] + data[13 + 15]) %
           2;
    res[6] = (q[0] + q[1] + q[2] + q[3] + q[4]) >= 3 ? 1 : 0;

    q[0] = data[7];
    q[1] = (data[13 + 2] + data[13 + 5] + data[13 + 7] + data[13 + 9] + data[13 + 12] + data[13 + 13] + data[13 + 14] +
            data[13 + 15] + data[13 + 16]) %
           2;
    q[2] = (data[13 + 1] + data[13 + 3] + data[13 + 5] + data[13 + 8] + data[13 + 11] + data[13 + 12] + data[13 + 13] +
            data[13 + 14] + data[13 + 15] + data[13 + 16]) %
           2;
    q[3] = (data[13 + 4] + data[13 + 5] + data[13 + 6] + data[13 + 10] + data[13 + 11] + data[13 + 12] + data[13 + 13] +
            data[13 + 14] + data[13 + 15] + data[13 + 16]) %
           2;
    q[4] = (data[13 + 1] + data[13 + 2] + data[13 + 3] + data[13 + 4] + data[13 + 5] + data[13 + 6] + data[13 + 7] +
            data[13 + 8] + data[13 + 9] + data[13 + 10] + data[13 + 12] + data[13 + 13] + data[13 + 14] +
            data[13 + 15] + data[13 + 16]) %
           2;
    res[7] = (q[0] + q[1] + q[2] + q[3] + q[4]) >= 3 ? 1 : 0;

    q[0] = data[8];
    q[1] = (data[13 + 2] + data[13 + 3] + data[13 + 9] + data[13 + 12] + data[13 + 13] + data[13 + 16]) % 2;
    q[2] =
        (data[13 + 1] + data[13 + 7] + data[13 + 8] + data[13 + 11] + data[13 + 12] + data[13 + 13] + data[13 + 16]) %
        2;
    q[3] = (data[13 + 3] + data[13 + 4] + data[13 + 6] + data[13 + 7] + data[13 + 10] + data[13 + 11] + data[13 + 12] +
            data[13 + 13] + data[13 + 16]) %
           2;
    q[4] = (data[13 + 1] + data[13 + 2] + data[13 + 4] + data[13 + 6] + data[13 + 8] + data[13 + 9] + data[13 + 10] +
            data[13 + 12] + data[13 + 13] + data[13 + 16]) %
           2;
    res[8] = (q[0] + q[1] + q[2] + q[3] + q[4]) >= 3 ? 1 : 0;

    q[0] = data[9];
    q[1] = (data[13 + 1] + data[13 + 3] + data[13 + 8] + data[13 + 12] + data[13 + 14] + data[13 + 16]) % 2;
    q[2] = (data[13 + 4] + data[13 + 6] + data[13 + 10] + data[13 + 12] + data[13 + 14] + data[13 + 16]) % 2;
    q[3] =
        (data[13 + 2] + data[13 + 7] + data[13 + 9] + data[13 + 11] + data[13 + 12] + data[13 + 14] + data[13 + 16]) %
        2;
    q[4] = (data[13 + 1] + data[13 + 2] + data[13 + 3] + data[13 + 4] + data[13 + 6] + data[13 + 7] + data[13 + 8] +
            data[13 + 9] + data[13 + 10] + data[13 + 11] + data[13 + 12] + data[13 + 14] + data[13 + 16]) %
           2;
    res[9] = (q[0] + q[1] + q[2] + q[3] + q[4]) >= 3 ? 1 : 0;

    q[0] = data[10];
    q[1] = (data[13 + 1] + data[13 + 2] + data[13 + 7] + data[13 + 13] + data[13 + 14] + data[13 + 16]) % 2;
    q[2] =
        (data[13 + 3] + data[13 + 8] + data[13 + 9] + data[13 + 11] + data[13 + 13] + data[13 + 14] + data[13 + 16]) %
        2;
    q[3] = (data[13 + 1] + data[13 + 4] + data[13 + 6] + data[13 + 9] + data[13 + 10] + data[13 + 11] + data[13 + 13] +
            data[13 + 14] + data[13 + 16]) %
           2;
    q[4] = (data[13 + 2] + data[13 + 3] + data[13 + 4] + data[13 + 6] + data[13 + 7] + data[13 + 8] + data[13 + 10] +
            data[13 + 13] + data[13 + 14] + data[13 + 16]) %
           2;
    res[10] = (q[0] + q[1] + q[2] + q[3] + q[4]) >= 3 ? 1 : 0;

    q[0] = data[11];
    q[1] = (data[13 + 2] + data[13 + 6] + data[13 + 9] + data[13 + 12] + data[13 + 15] + data[13 + 16]) % 2;
    q[2] =
        (data[13 + 4] + data[13 + 7] + data[13 + 10] + data[13 + 11] + data[13 + 12] + data[13 + 15] + data[13 + 16]) %
        2;
    q[3] = (data[13 + 1] + data[13 + 3] + data[13 + 6] + data[13 + 7] + data[13 + 8] + data[13 + 11] + data[13 + 12] +
            data[13 + 15] + data[13 + 16]) %
           2;
    q[4] = (data[13 + 1] + data[13 + 2] + data[13 + 3] + data[13 + 4] + data[13 + 8] + data[13 + 9] + data[13 + 10] +
            data[13 + 12] + data[13 + 15] + data[13 + 16]) %
           2;
    res[11] = (q[0] + q[1] + q[2] + q[3] + q[4]) >= 3 ? 1 : 0;

    q[0] = data[12];
    q[1] =
        (data[13 + 5] + data[13 + 8] + data[13 + 10] + data[13 + 11] + data[13 + 13] + data[13 + 15] + data[13 + 16]) %
        2;
    q[2] = (data[13 + 1] + data[13 + 3] + data[13 + 4] + data[13 + 5] + data[13 + 6] + data[13 + 11] + data[13 + 13] +
            data[13 + 15] + data[13 + 16]) %
           2;
    q[3] = (data[13 + 1] + data[13 + 2] + data[13 + 3] + data[13 + 5] + data[13 + 7] + data[13 + 9] + data[13 + 10] +
            data[13 + 13] + data[13 + 15] + data[13 + 16]) %
           2;
    q[4] = (data[13 + 2] + data[13 + 4] + data[13 + 5] + data[13 + 6] + data[13 + 7] + data[13 + 8] + data[13 + 9] +
            data[13 + 13] + data[13 + 15] + data[13 + 16]) %
           2;
    res[12] = (q[0] + q[1] + q[2] + q[3] + q[4]) >= 3 ? 1 : 0;

    q[0] = data[13];
    q[1] = (data[13 + 2] + data[13 + 4] + data[13 + 7] + data[13 + 14] + data[13 + 15] + data[13 + 16]) % 2;
    q[2] =
        (data[13 + 6] + data[13 + 9] + data[13 + 10] + data[13 + 11] + data[13 + 14] + data[13 + 15] + data[13 + 16]) %
        2;
    q[3] = (data[13 + 1] + data[13 + 3] + data[13 + 4] + data[13 + 8] + data[13 + 9] + data[13 + 11] + data[13 + 14] +
            data[13 + 15] + data[13 + 16]) %
           2;
    q[4] = (data[13 + 1] + data[13 + 2] + data[13 + 3] + data[13 + 6] + data[13 + 7] + data[13 + 8] + data[13 + 10] +
            data[13 + 14] + data[13 + 15] + data[13 + 16]) %
           2;
    res[13] = (q[0] + q[1] + q[2] + q[3] + q[4]) >= 3 ? 1 : 0;

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
auto LowerMac::check_crc_16_ccitt(const std::vector<uint8_t>& data, int len) noexcept -> int {
    uint16_t crc = 0xFFFF; // CRC16-CCITT initial value

    for (int i = 0; i < len; i++) {
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
