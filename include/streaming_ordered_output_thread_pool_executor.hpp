/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>
#if defined(__linux__)
#include <pthread.h>
#endif

struct TerminationToken {};

// thread pool executing work but outputting it the order of the input
template <typename ReturnType> class StreamingOrderedOutputThreadPoolExecutor {

  public:
    using OptionalReturnType = std::optional<ReturnType>;

    StreamingOrderedOutputThreadPoolExecutor() = delete;

    StreamingOrderedOutputThreadPoolExecutor(std::atomic_bool& input_termination_flag,
                                             std::atomic_bool& output_termination_flag, int num_workers)
        : input_termination_flag_(input_termination_flag)
        , output_termination_flag_(output_termination_flag) {
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
        /// Queue up a termination token
        for (auto& t : workers_)
            t.join();
    };

    // append work to the queuetemplate <typename ReturnTypeOrTerminationToken>
    void queue_work(std::function<ReturnType()> work) {
        {
            std::lock_guard<std::mutex> lock(cv_input_item_mutex_);
            input_queue_.emplace_back(std::make_pair(input_counter_++, work));
        }
        cv_input_item_.notify_one();
    };

    // get a finished item of a nullopt
    auto get_or_null() -> OptionalReturnType {
        using namespace std::chrono_literals;

        OptionalReturnType result{};

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

        /// propagate the flag if all processing threads have terminated
        if (!result.has_value() && alive_thread_count_.load() == 0) {
            output_termination_flag_.store(true);
        }

        return result;
    };

    auto empty() -> bool {
        std::unique_lock<std::mutex> lk(cv_output_item_mutex_);
        return output_map_.empty();
    };

  private:
    auto worker() -> void {
        using namespace std::chrono_literals;

        alive_thread_count_++;

        for (;;) {
            std::optional<std::pair<uint64_t, std::function<ReturnType()>>> work{};

            {
                std::lock_guard lk(cv_input_item_mutex_);
                if (!input_queue_.empty()) {
                    work = input_queue_.front();
                    input_queue_.pop_front();
                } else if (input_termination_flag_.load()) {
                    break;
                }
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

        alive_thread_count_--;
    };

    /// the termination flag on the input
    std::atomic_bool& input_termination_flag_;
    /// the termination flag on the input for the next stage
    std::atomic_bool& output_termination_flag_;

    /// the counter that decrements to 0 if all thread terminated. It is used for the last thread to propagate the
    /// termiantion signal.
    std::atomic_int alive_thread_count_ = 0;

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
