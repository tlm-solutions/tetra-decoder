/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
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
#include <optional>
#include <signal_handler.hpp>
#include <thread>
#include <vector>
#if defined(__linux__)
#include <pthread.h>
#endif

// thread pool executing work but outputting it the order of the input
template <typename ReturnType> class StreamingOrderedOutputThreadPoolExecutor {

  public:
    StreamingOrderedOutputThreadPoolExecutor() = delete;

    explicit StreamingOrderedOutputThreadPoolExecutor(int num_workers) {
        for (auto i = 0; i < num_workers; i++) {
            std::thread t(&StreamingOrderedOutputThreadPoolExecutor<ReturnType>::worker, this);

#if defined(__linux__)
            auto handle = t.native_handle();
            auto thread_name = "StreamWorker" + std::to_string(i);
            pthread_setname_np(handle, thread_name.c_str());
#endif

            workers_.push_back(std::move(t));
        }
    };

    ~StreamingOrderedOutputThreadPoolExecutor() {
        for (auto& t : workers_)
            t.join();
    };

    // append work to the queuetemplate <typename ReturnType>
    void queue_work(std::function<ReturnType()> work) {
        {
            std::lock_guard<std::mutex> lock(cv_input_item_mutex_);
            input_queue_.emplace_back(std::make_pair(input_counter_++, work));
        }
        cv_input_item_.notify_one();
    };

    // wait and get a finished item
    auto get() -> ReturnType {
        using namespace std::chrono_literals;

        for (;;) {
            std::optional<ReturnType> result{};

            {
                std::lock_guard<std::mutex> lk(cv_output_item_mutex_);

                if (auto search = output_map_.find(output_counter_); search != output_map_.end()) {
                    result = search->second;
                    output_map_.erase(search);
                    output_counter_++;
                }
            }

            if (!result.has_value()) {
                std::unique_lock<std::mutex> lk(cv_output_item_mutex_);

                auto res = cv_output_item_.wait_for(lk, 10ms, [&] {
                    // find the output item and if found set outputCounter_ to the next item
                    if (auto search = output_map_.find(output_counter_); search != output_map_.end()) {
                        result = search->second;
                        output_map_.erase(search);
                        output_counter_++;
                        return true;
                    }

                    return false;
                });
            }

            if (result.has_value()) {
                return *result;
            }
        }
    };

  private:
    auto worker() -> void {
        using namespace std::chrono_literals;

        for (;;) {
            std::optional<std::pair<uint64_t, std::function<ReturnType()>>> work{};

            {
                std::lock_guard lk(cv_input_item_mutex_);
                if (!input_queue_.empty()) {
                    work = input_queue_.front();
                    input_queue_.pop_front();
                } else if (stop)
                    break;
            }

            if (!work.has_value()) {
                std::unique_lock<std::mutex> lk(cv_input_item_mutex_);
                cv_input_item_.wait_for(lk, 10ms, [&] {
                    if (!input_queue_.empty()) {
                        work = input_queue_.front();
                        input_queue_.pop_front();
                        return true;
                    }

                    return false;
                });
            }

            if (work.has_value()) {
                auto index = work->first;

                // do the work
                auto result = work->second();

                {
                    std::lock_guard<std::mutex> lock(cv_output_item_mutex_);
                    output_map_[index] = result;
                }
                cv_output_item_.notify_all();
            }
        }
    };

    // locks for input to worker threads
    std::condition_variable cv_input_item_;
    std::mutex cv_input_item_mutex_;

    // locks for output (get)
    std::condition_variable cv_output_item_;
    std::mutex cv_output_item_mutex_;

    // queue_ of work with and incrementing index
    std::deque<std::pair<uint64_t, std::function<ReturnType()>>> input_queue_{};

    // output queue_. this is a map so we can do a lookup on the current index for ordered output
    std::map<uint64_t, ReturnType> output_map_{};

    // contains the value of the next input item
    uint64_t input_counter_ = 0;
    // contains the index of the next output item
    uint64_t output_counter_ = 0;

    std::vector<std::thread> workers_;
};
