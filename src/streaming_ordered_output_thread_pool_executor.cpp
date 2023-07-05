/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#include <streaming_ordered_output_thread_pool_executor.hpp>

#include <optional>

template <typename ReturnType>
StreamingOrderedOutputThreadPoolExecutor<ReturnType>::StreamingOrderedOutputThreadPoolExecutor(int numWorkers) {
    for (auto i = 0; i < numWorkers; i++) {
        std::thread t(&StreamingOrderedOutputThreadPoolExecutor<ReturnType>::worker, this);
        workers.push_back(std::move(t));
    }
}

template <typename ReturnType> void StreamingOrderedOutputThreadPoolExecutor<ReturnType>::worker() {
    while (true) {
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
            cv_input_item.wait(lk, [&] {
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
        {
            std::lock_guard lk(cv_output_item_m);
            if (auto search = outputMap.find(outputCounter); search != outputMap.end()) {
                result = search->second;
                outputCounter++;
                break;
            }
        }

        std::unique_lock<std::mutex> lk(cv_output_item_m);
        cv_output_item.wait(lk, [&] {
            // find the output item and if found set outputCounter_ to the next item
            if (auto search = outputMap.find(outputCounter); search != outputMap.end()) {
                result = search->second;
                outputCounter++;
                return true;
            }

            return false;
        });
    }

    return *result;
}

template class StreamingOrderedOutputThreadPoolExecutor<std::vector<std::function<void()>>>;
