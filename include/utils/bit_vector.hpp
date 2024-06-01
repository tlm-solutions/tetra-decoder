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
    [[nodiscard]] auto take(std::size_t numberBits) -> uint64_t;
    [[nodiscard]] auto compute_fcs() -> uint32_t;

    [[nodiscard]] auto take_vector(std::size_t numberBits) -> const uint8_t* const;

  private:
    [[nodiscard]] auto take_last_vector(std::size_t numberBits) -> const uint8_t* const;

  public:
    [[nodiscard]] auto take_last(std::size_t numberBits) -> uint64_t;
    [[nodiscard]] auto take_last() -> uint8_t;
    [[nodiscard]] inline auto bits_left() const noexcept -> std::size_t { return len_; };
    [[nodiscard]] auto is_mac_padding() const noexcept -> bool;

    friend auto operator<<(std::ostream& stream, const BitVector& vec) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const BitVector& vec) -> std::ostream&;