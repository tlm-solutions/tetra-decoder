/*
 * Original Implementation of ViterbiCodec.
 *
 * Author: Min Xu <xukmin@gmail.com>
 * Date: 01/30/2015
 *
 * Copyright 2015 Min Xu <xukmin@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include "viter_bi_codec.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace {

auto HammingDistance(const std::string& x, const std::string& y) -> int {
    assert(x.size() == y.size());
    int distance = 0;
    for (std::size_t i = 0; i < x.size(); i++) {
        distance += x[i] != y[i];
    }
    return distance;
}

} // namespace

auto operator<<(std::ostream& os, const ViterbiCodec& codec) -> std::ostream& {
    os << "ViterbiCodec(" << codec.constraint() << ", {";
    const std::vector<int>& polynomials = codec.polynomials();
    assert(!polynomials.empty());
    os << polynomials.front();
    for (std::size_t i = 1; i < polynomials.size(); i++) {
        os << ", " << polynomials[i];
    }
    return os << "})";
}

static auto ReverseBits(int num_bits, int input) -> int {
    assert(input < (1 << num_bits));
    int output = 0;
    while (num_bits-- > 0) {
        output = (output << 1) + (input & 1);
        input >>= 1;
    }
    return output;
}

ViterbiCodec::ViterbiCodec(int constraint, const std::vector<int>& polynomials)
    : constraint_c(constraint)
    , polynomials_c(polynomials) {
    assert(!polynomials_c.empty());
    for (std::size_t i = 0; i < polynomials_c.size(); i++) {
        assert(polynomials_c[i] > 0);
        assert(polynomials_c[i] < (1 << constraint_c));
    }
    InitializeOutputs();
}

int ViterbiCodec::num_parity_bits() const { return polynomials_c.size(); }

int ViterbiCodec::NextState(int current_state, int input) const {
    return (current_state >> 1) | (input << (constraint_c - 2));
}

std::string ViterbiCodec::Output(int current_state, int input) const {
    return outputs_c.at(current_state | (input << (constraint_c - 1)));
}

std::string ViterbiCodec::Encode(const std::string& bits) const {
    std::string encoded;
    int state = 0;

    // Encode the message bits.
    for (std::size_t i = 0; i < bits.size(); i++) {
        char c = bits[i];
        assert(c == '0' || c == '1');
        int input = c - '0';
        encoded += Output(state, input);
        state = NextState(state, input);
    }

    // Encode (constaint_ - 1) flushing bits.
    for (int i = 0; i < constraint_c - 1; i++) {
        encoded += Output(state, 0);
        state = NextState(state, 0);
    }

    return encoded;
}

void ViterbiCodec::InitializeOutputs() {
    outputs_c.resize(1 << constraint_c);
    for (std::size_t i = 0; i < outputs_c.size(); i++) {
        for (int j = 0; j < num_parity_bits(); j++) {
            // Reverse polynomial bits to make the convolution code simpler.
            int polynomial = ReverseBits(constraint_c, polynomials_c[j]);
            int input = i;
            int output = 0;
            for (int k = 0; k < constraint_c; k++) {
                output ^= (input & 1) & (polynomial & 1);
                polynomial >>= 1;
                input >>= 1;
            }
            outputs_c[i] += output ? "1" : "0";
        }
    }
}

int ViterbiCodec::BranchMetric(const std::string& bits, int source_state, int target_state) const {
    assert((int)bits.size() == num_parity_bits());
    assert((target_state & ((1 << (constraint_c - 2)) - 1)) == source_state >> 1);
    const std::string output = Output(source_state, target_state >> (constraint_c - 2));

    return HammingDistance(bits, output);
}

std::pair<int, int> ViterbiCodec::PathMetric(const std::string& bits, const std::vector<int>& prev_path_metrics,
                                             int state) const {
    int s = (state & ((1 << (constraint_c - 2)) - 1)) << 1;
    int source_state1 = s | 0;
    int source_state2 = s | 1;

    int pm1 = prev_path_metrics[source_state1];
    if (pm1 < std::numeric_limits<int>::max()) {
        pm1 += BranchMetric(bits, source_state1, state);
    }
    int pm2 = prev_path_metrics[source_state2];
    if (pm2 < std::numeric_limits<int>::max()) {
        pm2 += BranchMetric(bits, source_state2, state);
    }

    if (pm1 <= pm2) {
        return std::make_pair(pm1, source_state1);
    } else {
        return std::make_pair(pm2, source_state2);
    }
}

void ViterbiCodec::UpdatePathMetrics(const std::string& bits, std::vector<int>* path_metrics, Trellis* trellis) const {
    std::vector<int> new_path_metrics(path_metrics->size());
    std::vector<int> new_trellis_column(1 << (constraint_c - 1));
    for (std::size_t i = 0; i < path_metrics->size(); i++) {
        std::pair<int, int> p = PathMetric(bits, *path_metrics, i);
        new_path_metrics[i] = p.first;
        new_trellis_column[i] = p.second;
    }

    *path_metrics = new_path_metrics;
    trellis->push_back(new_trellis_column);
}

std::string ViterbiCodec::Decode(const std::string& bits) const {
    // Compute path metrics and generate trellis.
    Trellis trellis;
    std::vector<int> path_metrics(1 << (constraint_c - 1), std::numeric_limits<int>::max());
    path_metrics.front() = 0;
    for (std::size_t i = 0; i < bits.size(); i += num_parity_bits()) {
        std::string current_bits(bits, i, num_parity_bits());
        // If some bits are missing, fill with trailing zeros.
        // This is not ideal but it is the best we can do.
        if ((int)current_bits.size() < num_parity_bits()) {
            current_bits.append(std::string(num_parity_bits() - current_bits.size(), '0'));
        }
        UpdatePathMetrics(current_bits, &path_metrics, &trellis);
    }

    // Traceback.
    std::string decoded;
    int state = std::min_element(path_metrics.begin(), path_metrics.end()) - path_metrics.begin();
    for (int i = trellis.size() - 1; i >= 0; i--) {
        decoded += state >> (constraint_c - 2) ? "1" : "0";
        state = trellis[i][state];
    }
    std::reverse(decoded.begin(), decoded.end());

    // Remove (constraint_c - 1) flushing bits.
    return decoded.substr(0, decoded.size()); // - constraint_c + 1);
}
