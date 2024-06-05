/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include <utils/bit_vector.hpp>

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
    const auto len = bits_left();

    // delete all entries
    read_offset_ += len;
    len_ -= len;

    return BitVector(bits, len);
}

auto BitVector::take_all() -> uint64_t {
    const auto bits = data_.begin() + read_offset_;
    const auto len = bits_left();

    if (bits_left() > 64) {
        throw std::runtime_error("Can only extract 64 bits remaining bits from BitVec, but it contains " +
                                 std::to_string(len) + ".");
    }

    // delete all entries
    read_offset_ += len;
    len_ -= len;

    uint64_t ret = bits[0];

    for (std::size_t i = 1; i < len; i++) {
        ret <<= 1;
        ret |= bits[i];
    }

    return ret;
};

auto BitVector::is_mac_padding() const noexcept -> bool {
    if (len_ == 0)
        return false;

    if (data_[read_offset_] != 1)
        return false;

    for (auto i = 1; i < len_; i++) {
        if (data_[read_offset_ + i] != 0)
            return false;
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
