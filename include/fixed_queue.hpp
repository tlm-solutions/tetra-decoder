#include <deque>
#include <iostream>
#include <queue>

template <typename T, int MaxLen, typename Container = std::deque<T>>
class FixedQueue : public std::queue<T, Container> {
  public:
    FixedQueue() {
        for (auto i = 0; i < MaxLen; i++) {
            T default_value{};
            std::queue<T, Container>::push(default_value);
        }
    }

    void push(const T& value) {
        if (this->size() == MaxLen) {
            this->c.pop_front();
        }
        std::queue<T, Container>::push(value);
    }

    void pop(const T& value) { std::logic_error("Function not implemented"); }

    typename Container::const_iterator cbegin() { return this->c.cbegin(); }

    typename Container::const_reverse_iterator crbegin() { return this->c.crbegin(); }

    typename Container::const_iterator cend() { return this->c.cend(); }

    typename Container::const_reverse_iterator crend() { return this->c.crend(); }
};
