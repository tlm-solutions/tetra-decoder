#include <algorithm>
#include <iostream>
#include <stdexcept>

#include <utils/BitVec.hpp>

BitVec::BitVec(const std::vector<uint8_t> &data) : _data(data) {}

std::vector<uint8_t> BitVec::takeVec(const size_t numberBits) {
  if (numberBits > bitsLeft()) {
    throw std::runtime_error(std::to_string(numberBits) +
                             " bits not left in BitVec (" +
                             std::to_string(bitsLeft()) + ")");
  }

  std::vector<uint8_t> res;

  std::copy_n(_data.begin(), numberBits, std::back_inserter(res));

  // delete first n entries
  std::vector<decltype(_data)::value_type>(_data.begin() + numberBits,
                                           _data.end())
      .swap(_data);

  return res;
}

uint64_t BitVec::take(const size_t numberBits) {
  std::vector<uint8_t> bits = takeVec(numberBits);

  uint64_t ret = 0;

  for (auto it = bits.begin(); it != bits.end(); it++) {
    if (it != bits.begin()) {
      ret <<= 1;
    }
    ret |= (*it & 0x1);
  }

  return ret;
}

size_t BitVec::bitsLeft() { return _data.size(); }
