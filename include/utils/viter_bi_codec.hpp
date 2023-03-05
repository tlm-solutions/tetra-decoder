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
  // Note about Polynomial Descriptor of a Convolutional Encoder / Decoder.
  // A generator polymonial is built as follows: Build a binary number
  // representation by placing a 1 in each spot where a connection line from
  // the shift register feeds into the adder, and a zero elsewhere. There are 2
  // ways to arrange the bits:
  // 1. msb-current
  //    The MSB of the polynomial corresponds to the current input, while the
  //    LSB corresponds to the oldest input that still remains in the shift
  //    register.
  //    This representation is used by MATLAB. See
  //    http://radio.feld.cvut.cz/matlab/toolbox/comm/tutor124.html
  // 2. lsb-current
  //    The LSB of the polynomial corresponds to the current input, while the
  //    MSB corresponds to the oldest input that still remains in the shift
  //    register.
  //    This representation is used by the Spiral Viterbi Decoder Software
  //    Generator. See http://www.spiral.net/software/viterbi.html
  // We use 2.
  ViterbiCodec(int constraint, const std::vector<int> &polynomials);

  [[nodiscard]] auto Encode(const std::string &bits) const -> std::string;
  [[nodiscard]] std::string Decode(const std::string &bits) const;

  [[nodiscard]] int constraint() const { return constraint_c; }

  [[nodiscard]] const std::vector<int> &polynomials() const { return polynomials_c; }

private:
  // Suppose
  //
  //     Trellis trellis;
  //
  // Then trellis[i][s] is the state in the (i - 1)th iteration which leads to
  // the current state s in the ith iteration.
  // It is used for traceback.
  typedef std::vector<std::vector<int>> Trellis;

  int num_parity_bits() const;

  void InitializeOutputs();

  int NextState(int current_state, int input) const;

  std::string Output(int current_state, int input) const;

  int BranchMetric(const std::string &bits, int source_state,
                   int target_state) const;

  // Given num_parity_bits() received bits, compute and returns path
  // metric and its corresponding previous state.
  std::pair<int, int> PathMetric(const std::string &bits,
                                 const std::vector<int> &prev_path_metrics,
                                 int state) const;

  // Given num_parity_bits() received bits, update path metrics of all states
  // in the current iteration, and append new traceback vector to trellis.
  void UpdatePathMetrics(const std::string &bits,
                         std::vector<int> *path_metrics,
                         Trellis *trellis) const;

  const int constraint_c;
  const std::vector<int> polynomials_c;

  // The output table.
  // The index is current input bit combined with previous inputs in the shift
  // register. The value is the output parity bits in string format for
  // convenience, e.g. "10". For example, suppose the shift register contains
  // 0b10 (= 2), and the current input is 0b1 (= 1), then the index is 0b110 (=
  // 6).
  std::vector<std::string> outputs_c;
};

std::ostream &operator<<(std::ostream &os, const ViterbiCodec &codec);

#endif /* VITERBI2_H */
