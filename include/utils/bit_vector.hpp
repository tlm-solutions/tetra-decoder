/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#ifndef TETRA_DECODER_UTILS_BIT_VECTOR_HPP
#define TETRA_DECODER_UTILS_BIT_VECTOR_HPP

#include <cstdint>
#include <utility>
#include <vector>

class BitVector {
  private:
    std::vector<uint8_t> data_;

  public:
    explicit BitVector(std::vector<uint8_t> data)
        : data_(std::move(data)){};
    ~BitVector() noexcept = default;

    auto take_vector(std::size_t numberBits) -> std::vector<uint8_t>;
    void append(std::vector<uint8_t> bits);
    auto take(std::size_t numberBits) -> uint64_t;
    auto take_last() -> uint8_t;
    [[nodiscard]] inline auto bits_left() const noexcept -> std::size_t { return data_.size(); };
    [[nodiscard]] auto is_mac_padding() const noexcept -> bool;

    friend auto operator<<(std::ostream& stream, const BitVector& vec) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const BitVector& vec) -> std::ostream&;

#endif // TETRA_DECODER_UTILS_BIT_VECTOR_HPP
