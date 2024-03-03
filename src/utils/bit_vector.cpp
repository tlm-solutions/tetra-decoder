/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#include <algorithm>
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
        uint8_t bit = (data_[read_offset_ + i] ^ (crc >> 31)) & 1;
        crc <<= 1;
        if (bit) {
            crc = crc ^ 0x04C11DB7;
        }
    }
    return ~crc;
}

auto BitVector::take_vector(const size_t numberBits) -> const uint8_t* const {
    if (numberBits > bits_left()) {
        throw std::runtime_error(std::to_string(numberBits) + " bits not left in BitVec (" +
                                 std::to_string(bits_left()) + ")");
    }

    auto res = data_.data() + read_offset_;

    // delete first n entries
    read_offset_ += numberBits;
    len_ -= numberBits;

    return res;
}

auto BitVector::take_last_vector(const size_t numberBits) -> const uint8_t* const {
    if (numberBits > bits_left()) {
        throw std::runtime_error(std::to_string(numberBits) + " bits not left in BitVec (" +
                                 std::to_string(bits_left()) + ")");
    }

    auto res = data_.data() + bits_left() - numberBits;

    // take from the back
    len_ -= numberBits;

    return res;
}

void BitVector::append(const BitVector& other) {
    // actually need to do a copy here!
    if (read_offset_ > 0) {
        // copy to front
        std::memcpy(data_.data(), data_.data() + read_offset_, sizeof(uint8_t) * len_);
        read_offset_ = 0;
    }
    // shrink the size
    data_.resize(len_);

    // copy in other
    data_.resize(len_ + other.len_);
    std::memcpy(data_.data() + len_, other.data_.data() + other.read_offset_, sizeof(uint8_t) * other.len_);
    len_ += other.len_;
}

auto BitVector::take(const size_t numberBits) -> uint64_t {
    auto bits = take_vector(numberBits);

    uint64_t ret = 0;

    for (auto i = 0; i < numberBits; i++) {
        if (i != 0)
            ret <<= 1;
        ret |= (bits[i] & 0x1);
    }

    return ret;
}

auto BitVector::take_last(const size_t numberBits) -> uint64_t {
    auto bits = take_last_vector(numberBits);

    uint64_t ret = 0;

    for (auto i = 0; i < numberBits; i++) {
        if (i != 0)
            ret <<= 1;
        ret |= (bits[i] & 0x1);
    }

    return ret;
}

auto BitVector::take_last() -> uint8_t {
    if (1 > bits_left()) {
        throw std::runtime_error("1 bit not left in BitVec (" + std::to_string(bits_left()) + ")");
    }

    len_ -= 1;

    return data_[read_offset_ + len_];
}

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
