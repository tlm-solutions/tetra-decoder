/*
 * Copyright (C) 2025 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "iq_stream_decoder.hpp"
#include <iostream>

auto main(int /*argc*/, char** /*argv*/) -> int {
    const std::vector<std::complex<float>> training_seq_x = {
        {1.0, -1.0}, {-1.0, 1.0}, {-1.0, -1.0}, {-1.0, 1.0}, {1.0, 1.0}, {1.0, 1.0}, {-1.0, -1.0}, {1.0, -1.0},
        {1.0, -1.0}, {-1.0, 1.0}, {-1.0, -1.0}, {-1.0, 1.0}, {1.0, 1.0}, {1.0, 1.0}, {-1.0, -1.0}};

    // Fill the queue with the training sequence
    IQStreamDecoder::QueueT queue;
    for (auto i = 0; i < training_seq_x.size(); i++) {
        std::complex<float> v = {0, 0};
        // if (i > 0) {
        //     std::complex<float> k = {-0.5, -0.5};
        //     v += training_seq_x[i - 1] * k;
        // }
        // if (i < training_seq_x.size() - 1) {
        //     std::complex<float> k = {-0.5, -0.5};
        //     v += training_seq_x[i + 1] * k;
        // }
        std::complex<float> k = {0.5, 0.2};
        v += training_seq_x[i] * k;

        queue.push(v);
    }
    // for (const auto& elem : training_seq_x) {
    //     queue.push(elem);
    // }

    auto result =
        IQStreamDecoder::solve_channel<3>(training_seq_x, queue, /*signal_offset=*/300 - training_seq_x.size());

    std::cout << result << '\n';

    return EXIT_SUCCESS;
}