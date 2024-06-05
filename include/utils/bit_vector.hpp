/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#pragma once

#include <cstdint>
#include <ostream>
#include <vector>

class BitVector {
  private:
    std::vector<uint8_t> data_;
    std::size_t len_;
    std::size_t read_offset_;

  public:
    BitVector()
        : data_()
        , len_(0)
        , read_offset_(0){};
    explicit BitVector(const std::vector<uint8_t>& vec)
        : data_(vec)
        , len_(vec.size())
        , read_offset_(0){};
    BitVector(const BitVector& other)
        : data_()
        , len_(0)
        , read_offset_(0) {
        append(other);
    };
    BitVector(const uint8_t* const data, const std::size_t len)
        : data_(data, data + len)
        , len_(len)
        , read_offset_(0){};
    ~BitVector() noexcept = default;

    auto append(const BitVector& other) -> void;

    /// Take N unsigned bits from the start. N is known at compile time
    template <std::size_t N> [[nodiscard]] auto take() -> unsigned _BitInt(N) {
        auto bits = take_vector(N);

        unsigned _BitInt(N) ret = (bits[0] & 0x1);

        // This condition is implicitly there on the first iteation of the loop, but not detected by some compilers
        // leading to a false warning: shift count >= width of type [-Wshift-count-overflow] with N == 1
        if (N > 1) {
            for (std::size_t i = 1; i < N; i++) {
                ret <<= 1;
                ret |= (bits[i] & 0x1);
            }
        }

        return ret;
    };

    [[nodiscard]] auto compute_fcs() -> uint32_t;

    [[nodiscard]] auto take_vector(std::size_t number_bits) -> const uint8_t* const;

  private:
    [[nodiscard]] auto take_last_vector(std::size_t number_bits) -> const uint8_t* const;

  public:
    /// Take a dynamic number of bits from the back. the size is not known at compile time
    [[nodiscard]] auto take_last(std::size_t number_bits) -> uint64_t;
    [[nodiscard]] auto take_last() -> unsigned _BitInt(1);
    [[nodiscard]] inline auto bits_left() const noexcept -> std::size_t { return len_; };
    [[nodiscard]] auto is_mac_padding() const noexcept -> bool;

    friend auto operator<<(std::ostream& stream, const BitVector& vec) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const BitVector& vec) -> std::ostream&;