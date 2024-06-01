#pragma once

#include <deque>
#include <iostream>
#include <queue>

template <typename T, int MaxLen, typename Container = std::deque<T>> class FixedQueue {
  private:
    Container queue_{};

  public:
    using const_iterator = typename Container::const_iterator;
    using const_reverse_iterator = typename Container::const_reverse_iterator;
    using const_reference = typename Container::const_reference;

    FixedQueue() {
        for (auto i = 0; i < MaxLen; i++) {
            T default_value{};
            queue_.push_back(default_value);
        }
    }

    void push(const T& value) {
        if (queue_.size() == MaxLen) {
            queue_.pop_front();
        }
        queue_.push_back(value);
    }

    void pop(const T& value) { std::logic_error("Function not implemented"); }

    typename Container::const_iterator cbegin() { return queue_.cbegin(); }

    typename Container::const_reverse_iterator crbegin() { return queue_.crbegin(); }

    typename Container::const_iterator cend() { return queue_.cend(); }

    typename Container::const_reverse_iterator crend() { return queue_.crend(); }

    typename Container::const_reference operator[](typename Container::size_type pos) const { return queue_[pos]; }
};
