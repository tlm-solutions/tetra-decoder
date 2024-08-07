/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#include "utils/bit_vector.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>

auto BitVector::compute_fcs() -> uint32_t {
    uint32_t crc = 0xFFFFFFFF;
    if (len_ < 32) {
        crc <<= (32 - len_);
    }

    for (auto i = 0; i < len_; i++) {
        bool bit = (static_cast<uint32_t>(data_[read_offset_ + i]) ^ (crc >> 31)) & 1;
        crc <<= 1;
        if (bit) {
            crc = crc ^ 0x04C11DB7;
        }
    }
    return ~crc;
}

void BitVector::append(const BitVector& other) {
    // actually need to do a copy here!
    if (read_offset_ > 0) {
        // copy to front
        data_ = std::vector(data_.begin() + read_offset_, data_.begin() + read_offset_ + len_);
        read_offset_ = 0;
    }
    // shrink the size
    data_.resize(len_);

    // copy in other
    data_.insert(data_.end(), other.data_.begin() + other.read_offset_,
                 other.data_.begin() + other.read_offset_ + other.len_);
    len_ += other.len_;
}

auto BitVector::take_vector(std::size_t number_bits) -> BitVector {
    const auto bits = data_.begin() + read_offset_;

    if (number_bits > bits_left()) {
        throw std::runtime_error(std::to_string(number_bits) + " bits not left in BitVec (" +
                                 std::to_string(bits_left()) + ")");
    }

    // delete all entries
    read_offset_ += number_bits;
    len_ -= number_bits;

    return BitVector(std::vector<bool>(bits, bits + number_bits));
}

auto BitVector::take_all() -> uint64_t {
    const auto bits = data_.begin() + read_offset_;
    const auto len = bits_left();

    if (len > 64) {
        throw std::runtime_error("Can only extract 64 bits remaining bits from BitVec, but it contains " +
                                 std::to_string(len) + ".");
    }

    // delete all entries
    read_offset_ += len;
    len_ -= len;

    auto ret = static_cast<uint64_t>(bits[0]);

    for (std::size_t i = 1; i < len; i++) {
        ret <<= 1;
        ret |= static_cast<uint64_t>(bits[i]);
    }

    return ret;
};

auto BitVector::is_mac_padding() const noexcept -> bool {
    if (len_ == 0) {
        return false;
    }

    // first bit must be true
    if (!data_[read_offset_]) {
        return false;
    }

    // all other bits must be false
    for (auto i = 1; i < len_; i++) {
        if (data_[read_offset_ + i]) {
            return false;
        }
    }

    return true;
}

auto operator<<(std::ostream& stream, const BitVector& vec) -> std::ostream& {
    stream << "BitVec: ";
    for (auto i = 0; i < vec.len_; i++) {
        stream << std::to_string(vec.data_[vec.read_offset_ + i]);
    }

    return stream;
}
