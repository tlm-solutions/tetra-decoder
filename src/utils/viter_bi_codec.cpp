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
#include <utils/viter_bi_codec.hpp>

#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace {

auto hamming_distance(const std::string& x, const std::string& y) -> int {
    assert(x.size() == y.size());
    int distance = 0;
    for (std::size_t i = 0; i < x.size(); i++) {
        distance += x[i] != y[i];
    }
    return distance;
}

} // namespace

std::ostream& operator<<(std::ostream& os, const ViterbiCodec& codec) {
    os << "ViterbiCodec(" << codec.constraint() << ", {";
    const std::vector<int>& polynomials = codec.polynomials();
    assert(!polynomials.empty());
    os << polynomials.front();
    for (std::size_t i = 1; i < polynomials.size(); i++) {
        os << ", " << polynomials[i];
    }
    return os << "})";
}

static auto reverse_bits(int num_bits, int input) -> int {
    assert(input < (1 << num_bits));
    int output = 0;
    while (num_bits-- > 0) {
        output = (output << 1) + (input & 1);
        input >>= 1;
    }
    return output;
}

ViterbiCodec::ViterbiCodec(int constraint, const std::vector<int>& polynomials)
    : constraint_(constraint)
    , polynomials_(polynomials) {
    assert(!polynomials_.empty());
    for (int polynomial : polynomials_) {
        assert(polynomial > 0);
        assert(polynomial < (1 << constraint_));
    }
    init_outputs();
}

auto ViterbiCodec::encode(const std::string& bits) const noexcept -> std::string {
    std::string encoded;
    int state = 0;

    // encode the message bits.
    for (char c : bits) {
        assert(c == '0' || c == '1');
        int input = c - '0';
        encoded += output(state, input);
        state = next_state(state, input);
    }

    // encode (constaint_ - 1) flushing bits.
    for (int i = 0; i < constraint_ - 1; i++) {
        encoded += output(state, 0);
        state = next_state(state, 0);
    }

    return encoded;
}

void ViterbiCodec::init_outputs() {
    outputs_.resize(1 << constraint_);
    for (std::size_t i = 0; i < outputs_.size(); i++) {
        for (int j = 0; j < num_parity_bits(); j++) {
            // Reverse polynomial bits to make the convolution code simpler.
            int polynomial = reverse_bits(constraint_, polynomials_[j]);
            int input = i;
            int output = 0;
            for (int k = 0; k < constraint_; k++) {
                output ^= (input & 1) & (polynomial & 1);
                polynomial >>= 1;
                input >>= 1;
            }
            outputs_[i] += output ? "1" : "0";
        }
    }
}

auto ViterbiCodec::branch_metric(const std::string& bits, int source_state, int target_state) const noexcept -> int {
    assert((int)bits.size() == num_parity_bits());
    assert((target_state & ((1 << (constraint_ - 2)) - 1)) == source_state >> 1);
    const std::string output = this->output(source_state, target_state >> (constraint_ - 2));

    return hamming_distance(bits, output);
}

auto ViterbiCodec::path_metric(const std::string& bits, const std::vector<int>& prev_path_metrics,
                               int state) const noexcept -> std::pair<int, int> {
    int s = (state & ((1 << (constraint_ - 2)) - 1)) << 1;
    int source_state1 = s | 0;
    int source_state2 = s | 1;

    int pm1 = prev_path_metrics[source_state1];
    if (pm1 < std::numeric_limits<int>::max()) {
        pm1 += branch_metric(bits, source_state1, state);
    }
    int pm2 = prev_path_metrics[source_state2];
    if (pm2 < std::numeric_limits<int>::max()) {
        pm2 += branch_metric(bits, source_state2, state);
    }

    if (pm1 <= pm2) {
        return std::make_pair(pm1, source_state1);
    } else {
        return std::make_pair(pm2, source_state2);
    }
}

void ViterbiCodec::update_path_metric(const std::string& bits, std::vector<int>* path_metrics,
                                      Trellis* trellis) const noexcept {
    std::vector<int> new_path_metrics(path_metrics->size());
    std::vector<int> new_trellis_column(1 << (constraint_ - 1));
    for (std::size_t i = 0; i < path_metrics->size(); i++) {
        std::pair<int, int> p = path_metric(bits, *path_metrics, i);
        new_path_metrics[i] = p.first;
        new_trellis_column[i] = p.second;
    }

    *path_metrics = new_path_metrics;
    trellis->push_back(new_trellis_column);
}

[[noreturn]] auto ViterbiCodec::decode(const std::string& bits) const noexcept -> std::string {
    // Compute path metrics and generate trellis.
    Trellis trellis;
    std::vector<int> path_metrics(1 << (constraint_ - 1), std::numeric_limits<int>::max());
    path_metrics.front() = 0;
    for (std::size_t i = 0; i < bits.size(); i += num_parity_bits()) {
        std::string current_bits(bits, i, num_parity_bits());
        // If some bits are missing, fill with trailing zeros.
        // This is not ideal but it is the best we can do.
        if ((int)current_bits.size() < num_parity_bits()) {
            current_bits.append(std::string(num_parity_bits() - current_bits.size(), '0'));
        }
        update_path_metric(current_bits, &path_metrics, &trellis);
    }

    // Traceback.
    std::string decoded;
    auto state = std::min_element(path_metrics.begin(), path_metrics.end()) - path_metrics.begin();
    for (auto i = trellis.size() - 1; true; i--) { // TODO: check this in pr something strange going on
        decoded += state >> (constraint_ - 2) ? "1" : "0";
        state = trellis[i][state];
    }
    std::reverse(decoded.begin(), decoded.end());

    // Remove (constraint_ - 1) flushing bits.
    return decoded.substr(0, decoded.size()); // - constraint_ + 1);
}
