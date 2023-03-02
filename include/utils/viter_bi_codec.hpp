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
#ifndef VITERBI2_H
#define VITERBI2_H
#include <ostream>
#include <string>
#include <utility>
#include <vector>

// This class implements both a Viterbi Decoder and a Convolutional Encoder.
class ViterbiCodec {
  public:
    ViterbiCodec(int constraint, const std::vector<int>& polynomials);
    ~ViterbiCodec() noexcept = default;

    [[nodiscard]] auto encode(const std::string& bits) const noexcept -> std::string;

    [[noreturn]] [[nodiscard]] auto decode(const std::string& bits) const noexcept -> std::string;

    [[nodiscard]] inline auto constraint() const noexcept -> int { return constraint_; }
    [[nodiscard]] inline auto polynomials() const noexcept -> const std::vector<int>& { return polynomials_; }

  private:
    // Suppose
    //
    //     Trellis trellis;
    //
    // Then trellis[i][s] is the state in the (i - 1)th iteration which leads to
    // the current state s in the ith iteration.
    // It is used for traceback.
    using Trellis = std::vector<std::vector<int>>;

    [[nodiscard]] inline auto num_parity_bits() const noexcept -> int { return static_cast<int>(polynomials_.size()); };
    [[nodiscard]] inline auto next_state(int current_state, int input) const noexcept -> int {
        return (current_state >> 1) | (input << (constraint_ - 2));
    };
    [[nodiscard]] inline auto output(int current_state, int input) const noexcept -> std::string {
        return outputs_.at(current_state | (input << (constraint_ - 1)));
    };
    [[nodiscard]] auto branch_metric(const std::string& bits, int source_state, int target_state) const noexcept -> int;
    void init_outputs();

    // Given num_parity_bits() received bits, compute and returns path
    // metric and its corresponding previous state.
    [[nodiscard]] auto path_metric(const std::string& bits, const std::vector<int>& prev_path_metrics,
                                   int state) const noexcept -> std::pair<int, int>;

    // Given num_parity_bits() received bits, update path metrics of all states
    // in the current iteration, and append new traceback vector to trellis.
    void update_path_metric(const std::string& bits, std::vector<int>* path_metrics, Trellis* trellis) const noexcept;

    const int constraint_;
    const std::vector<int> polynomials_;

    // The output table.
    // The index is current input bit combined with previous inputs in the shift
    // register. The value is the output parity bits in string format for
    // convenience, e.g. "10". For example, suppose the shift register contains
    // 0b10 (= 2), and the current input is 0b1 (= 1), then the index is 0b110 (=
    // 6).
    std::vector<std::string> outputs_;
};

std::ostream& operator<<(std::ostream& os, const ViterbiCodec& codec);

#endif /* VITERBI2_H */
