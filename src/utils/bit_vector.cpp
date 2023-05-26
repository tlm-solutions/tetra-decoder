/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#include <algorithm>
#include <iostream>
#include <stdexcept>

#include <utils/bit_vector.hpp>

auto BitVector::take_vector(const size_t numberBits) -> std::vector<uint8_t> {
    if (numberBits > bits_left()) {
        throw std::runtime_error(std::to_string(numberBits) + " bits not left in BitVec (" +
                                 std::to_string(bits_left()) + ")");
    }

    std::vector<uint8_t> res;

    std::copy_n(data_.begin(), numberBits, std::back_inserter(res));

    // delete first n entries
    std::vector<decltype(data_)::value_type>(data_.begin() + numberBits, data_.end()).swap(data_);

    return res;
}

void BitVector::append(std::vector<uint8_t> bits) { data_.insert(data_.end(), bits.begin(), bits.end()); }

auto BitVector::take(const size_t numberBits) -> uint64_t {
    std::vector<uint8_t> bits = take_vector(numberBits);

    uint64_t ret = 0;

    for (auto it = bits.begin(); it != bits.end(); it++) {
        if (it != bits.begin()) {
            ret <<= 1;
        }
        ret |= (*it & 0x1);
    }

    return ret;
}

auto BitVector::take_last() -> uint8_t {
    if (1 > bits_left()) {
        throw std::runtime_error("1 bit not left in BitVec (" + std::to_string(bits_left()) + ")");
    }

    auto last = data_.back();
    data_.pop_back();

    return last;
}

auto BitVector::is_mac_padding() const noexcept -> bool {
    auto it = data_.begin();

    if (*it++ != 1)
        return false;

    for (; it != data_.end(); it++) {
        if (*it != 0)
            return false;
    }

    return true;
}

auto operator<<(std::ostream& stream, const BitVector& vec) -> std::ostream& {
    stream << "BitVec: ";
    for (unsigned char it : vec.data_) {
        stream << std::to_string(it);
    }

    return stream;
}
