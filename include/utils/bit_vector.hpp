#ifndef UTILS_BITVEC_HPP
#define UTILS_BITVEC_HPP

#include <cstdint>
#include <vector>

class BitVec {
public:
  BitVec(const std::vector<uint8_t> &data);
  ~BitVec(){};

  std::vector<uint8_t> takeVec(const size_t numberBits);
  uint64_t take(const size_t numberBits);
  uint8_t takeLast();
  size_t bitsLeft();
  size_t size() { return bitsLeft(); }
  bool isMacPadding();

  friend std::ostream &operator<<(std::ostream &stream, const BitVec &vec);

private:
  std::vector<uint8_t> _data;
};

std::ostream &operator<<(std::ostream &stream, const BitVec &vec);

#endif
