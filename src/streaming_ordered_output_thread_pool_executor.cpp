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
StreamingOrderedOutputThreadPoolExecutor<ReturnType>::StreamingOrderedOutputThreadPoolExecutor(int numWorkers) {
    for (auto i = 0; i < numWorkers; i++) {
        std::thread t(&StreamingOrderedOutputThreadPoolExecutor<ReturnType>::worker, this);

#if defined(__linux__)
        auto handle = t.native_handle();
        auto threadName = "StreamWorker" + std::to_string(i);
        pthread_setname_np(handle, threadName.c_str());
#endif

        workers.push_back(std::move(t));
    }
}

template <typename ReturnType>
StreamingOrderedOutputThreadPoolExecutor<ReturnType>::~StreamingOrderedOutputThreadPoolExecutor() {
    for (auto& t : workers)
        t.join();
}

template <typename ReturnType> void StreamingOrderedOutputThreadPoolExecutor<ReturnType>::worker() {
    while (!stop) {
        std::optional<std::pair<uint64_t, std::function<ReturnType()>>> work{};

        {
            std::lock_guard lk(cv_input_item_m);
            if (!inputQueue.empty()) {
                work = inputQueue.front();
                inputQueue.pop_front();
            }
        }

        if (!work.has_value()) {
            std::unique_lock<std::mutex> lk(cv_input_item_m);
            cv_input_item.wait_for(lk, 10ms, [&] {
                if (!inputQueue.empty()) {
                    work = inputQueue.front();
                    inputQueue.pop_front();
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
                std::lock_guard<std::mutex> lock(cv_output_item_m);
                outputMap[index] = result;
            }
            cv_output_item.notify_all();
        }
    }
}

template <typename ReturnType>
void StreamingOrderedOutputThreadPoolExecutor<ReturnType>::queueWork(std::function<ReturnType()> work) {
    {
        std::lock_guard<std::mutex> lock(cv_input_item_m);
        inputQueue.push_back(std::make_pair(inputCounter++, work));
    }
    cv_input_item.notify_one();
}

template <typename ReturnType> ReturnType StreamingOrderedOutputThreadPoolExecutor<ReturnType>::get() {
    std::optional<ReturnType> result{};

    while (!result.has_value()) {
        std::unique_lock<std::mutex> lk(cv_output_item_m);
        cv_output_item.wait_for(lk, 10ms, [&] {
            // find the output item and if found set outputCounter_ to the next item
            if (auto search = outputMap.find(outputCounter); search != outputMap.end()) {
                result = search->second;
                outputMap.erase(search);
                outputCounter++;
                return true;
            }

            return false;
        });

        if (auto search = outputMap.find(outputCounter); search != outputMap.end()) {
            result = search->second;
            outputMap.erase(search);
            outputCounter++;
        }
    }

    return *result;
}

template class StreamingOrderedOutputThreadPoolExecutor<std::vector<std::function<void()>>>;
