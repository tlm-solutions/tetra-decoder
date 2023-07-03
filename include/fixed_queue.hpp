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

    typename Container::iterator begin() { return this->c.begin(); }

    typename Container::reverse_iterator rbegin() { return this->c.rbegin(); }

    typename Container::iterator end() { return this->c.end(); }

    typename Container::reverse_iterator rend() { return this->c.rend(); }
};
