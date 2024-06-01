/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#include <streaming_ordered_output_thread_pool_executor.hpp>
#if defined(__linux__)
#include <pthread.h>
#endif

#include <chrono>
#include <optional>

using namespace std::chrono_literals;

template <typename ReturnType>
StreamingOrderedOutputThreadPoolExecutor<ReturnType>::StreamingOrderedOutputThreadPoolExecutor(int num_workers) {
    for (auto i = 0; i < num_workers; i++) {
        std::thread t(&StreamingOrderedOutputThreadPoolExecutor<ReturnType>::worker, this);

#if defined(__linux__)
        auto handle = t.native_handle();
        auto threadName = "StreamWorker" + std::to_string(i);
        pthread_setname_np(handle, threadName.c_str());
#endif

        workers_.push_back(std::move(t));
    }
}

template <typename ReturnType>
StreamingOrderedOutputThreadPoolExecutor<ReturnType>::~StreamingOrderedOutputThreadPoolExecutor() {
    for (auto& t : workers_)
        t.join();
}

template <typename ReturnType> void StreamingOrderedOutputThreadPoolExecutor<ReturnType>::worker() {
    for (;;) {
        std::optional<std::pair<uint64_t, std::function<ReturnType()>>> work{};

        {
            std::lock_guard lk(cv_input_item_m_);
            if (!input_queue_.empty()) {
                work = input_queue_.front();
                input_queue_.pop_front();
            } else if (stop)
                break;
        }

        if (!work.has_value()) {
            std::unique_lock<std::mutex> lk(cv_input_item_m_);
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
                std::lock_guard<std::mutex> lock(cv_output_item_m_);
                output_map_[index] = result;
            }
            cv_output_item_.notify_all();
        }
    }
}

template <typename ReturnType>
void StreamingOrderedOutputThreadPoolExecutor<ReturnType>::queue_work(std::function<ReturnType()> work) {
    {
        std::lock_guard<std::mutex> lock(cv_input_item_m_);
        input_queue_.push_back(std::make_pair(input_counter_++, work));
    }
    cv_input_item_.notify_one();
}

template <typename ReturnType> ReturnType StreamingOrderedOutputThreadPoolExecutor<ReturnType>::get() {
    for (;;) {
        std::optional<ReturnType> result{};

        {
            std::lock_guard<std::mutex> lk(cv_output_item_m_);

            if (auto search = output_map_.find(output_counter_); search != output_map_.end()) {
                result = search->second;
                output_map_.erase(search);
                output_counter_++;
            }
        }

        if (!result.has_value()) {
            std::unique_lock<std::mutex> lk(cv_output_item_m_);
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

        if (result.has_value())
            return *result;
    }
}

template class StreamingOrderedOutputThreadPoolExecutor<std::vector<std::function<void()>>>;
