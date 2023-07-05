/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

// thread pool executing work but outputting it the order of the input
template <typename ReturnType> class StreamingOrderedOutputThreadPoolExecutor {

  public:
    StreamingOrderedOutputThreadPoolExecutor(int numWorkers);
    ~StreamingOrderedOutputThreadPoolExecutor() = default;

    // append work to the queue
    void queueWork(std::function<ReturnType()> work);

    // wait and get a finished item
    ReturnType get();

  private:
    void worker();

    // locks for input to worker threads
    std::condition_variable cv_input_item;
    std::mutex cv_input_item_m;

    // locks for output (get)
    std::condition_variable cv_output_item;
    std::mutex cv_output_item_m;

    // queue of work with and incrementing index
    std::deque<std::pair<uint64_t, std::function<ReturnType()>>> inputQueue{};
    // output queue. this is a map so we can do a lookup on the current index for ordered output
    std::map<uint64_t, ReturnType> outputMap{};

    // contains the value of the next input item
    uint64_t inputCounter = 0;
    // contains the index of the next output item
    uint64_t outputCounter = 0;

    std::vector<std::thread> workers;
};
