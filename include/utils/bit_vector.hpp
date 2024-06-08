/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <vector>

/// Construct a vector of bits that allows taking ranges of bits from the internal representation. The internal
/// representation is not copied if bits are taken.
class BitVector {
  private:
    /// The bits we hold
    std::vector<bool> data_;
    /// The length of the currently viewed data to support taking bits from the back.
    std::size_t len_ = 0;
    /// The current read offset to support taking bits from the front.
    std::size_t read_offset_ = 0;

  public:
    BitVector() = default;
    explicit BitVector(const std::vector<bool>& vec)
        : data_(vec)
        , len_(data_.size()){};
    explicit BitVector(std::vector<bool>&& vec)
        : data_(std::move(vec))
        , len_(data_.size()){};

    BitVector(const BitVector&) = default;
    auto operator=(const BitVector&) -> BitVector& = default;
    BitVector(BitVector&&) = default;
    auto operator=(BitVector&&) -> BitVector& = default;

    ~BitVector() noexcept = default;

    [[nodiscard]] inline auto bits_left() const noexcept -> auto{ return len_; };

    /// Append another bitvector to the current one. This will cause data to be copied.
    auto append(const BitVector& other) -> void;

    /// Take N unsigned bits from the start of the bitvector view. N is known at compile time.
    // TODO: assert N != 0
    template <std::size_t N> [[nodiscard]] auto take() -> unsigned _BitInt(N) {
        if (N > bits_left()) {
            throw std::runtime_error(std::to_string(N) + " bits not left in BitVec (" + std::to_string(bits_left()) +
                                     ")");
        }

        const auto bits = data_.begin() + read_offset_;

        // delete first n entries
        read_offset_ += N;
        len_ -= N;

        return to_bit_int<N>(bits);
    };

    /// Take N unsigned bits from the end of the bitvector view. N is known at compile time.
    // TODO: assert N != 0
    template <std::size_t N> [[nodiscard]] auto take_last() -> unsigned _BitInt(N) {
        if (N > bits_left()) {
            throw std::runtime_error(std::to_string(N) + " bits not left in BitVec (" + std::to_string(bits_left()) +
                                     ")");
        }

        const auto bits = data_.begin() + bits_left() - N;

        // delete last n entries
        len_ -= N;

        return to_bit_int<N>(bits);
    }

    /// look at N bits with an offset to the bitvector
    template <std::size_t N> [[nodiscard]] auto look(std::size_t offset) const -> unsigned _BitInt(N) {
        if (N + offset > bits_left()) {
            throw std::runtime_error(std::to_string(N + offset) + " bits not left in BitVec (" +
                                     std::to_string(bits_left()) + ")");
        }

        const auto bits = data_.begin() + read_offset_ + N + offset;

        return to_bit_int<N>(bits);
    }

    [[nodiscard]] auto compute_fcs() -> uint32_t;

    /// bite of a bitvector from the current bitvector
    [[nodiscard]] auto take_vector(std::size_t number_bits) -> BitVector;

    /// take all the remaining bits
    [[nodiscard]] auto take_all() -> uint64_t;

    [[nodiscard]] auto is_mac_padding() const noexcept -> bool;

    friend auto operator<<(std::ostream& stream, const BitVector& vec) -> std::ostream&;

  private:
    template <std::size_t N>
    [[nodiscard]] auto to_bit_int(const std::vector<bool>::const_iterator iterator) const -> unsigned _BitInt(N) {
        unsigned _BitInt(N) ret = iterator[0];

        // This condition is implicitly there on the first iteation of the loop, but not detected by some compilers
        // leading to a false warning: shift count >= width of type [-Wshift-count-overflow] with N == 1
        if (N > 1) {
            for (auto i = 1; i < N; i++) {
                ret <<= 1;
                ret |= iterator[i];
            }
        }

        return ret;
    }
};

auto operator<<(std::ostream& stream, const BitVector& vec) -> std::ostream&;