/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include <deque>
#include <mutex>
#include <optional>
#include <utility>

template <typename T> class ThreadSafeFifo {
  public:
    using OptionalT = std::optional<T>;

    ThreadSafeFifo() = default;
    ~ThreadSafeFifo() = default;

    ThreadSafeFifo(const ThreadSafeFifo&) = delete;
    auto operator=(const ThreadSafeFifo&) -> ThreadSafeFifo& = delete;

    ThreadSafeFifo(ThreadSafeFifo&&) = delete;
    auto operator=(ThreadSafeFifo&&) -> ThreadSafeFifo& = delete;

    // get a finished item of a nullopt
    auto get_or_null() -> OptionalT {
        using namespace std::chrono_literals;

        OptionalT result;

        {
            std::lock_guard<std::mutex> lk(mutex_);
            if (!queue_.empty()) {
                result = std::forward<T>(queue_.front());
                queue_.pop_front();
            }
        }

        return result;
    };

    auto empty() -> bool {
        std::lock_guard<std::mutex> lk(mutex_);
        return queue_.empty();
    };

    auto push_back(T&& element) -> void {
        std::lock_guard<std::mutex> lk(mutex_);
        queue_.push_back(std::forward<T>(element));
    };

  private:
    /// the mutex that is used to access the queue.
    std::mutex mutex_;

    /// the wrapped queue
    std::deque<T> queue_;
};
