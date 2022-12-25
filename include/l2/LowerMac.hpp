#ifndef L2_LOWERMAC_HPP
#define L2_LOWERMAC_HPP

#include <cstdint>
#include <memory>
#include <vector>

#include <BurstType.hpp>
#include <l2/UpperMac.hpp>
#include <utils/Viterbi.hpp>

class LowerMac {
public:
  LowerMac();
  ~LowerMac(){};

  void process(const std::vector<uint8_t> &frame, BurstType burstType);

private:
  std::unique_ptr<ViterbiCodec> _viterbiCodec1614;
  std::vector<uint8_t> descramble(const std::vector<uint8_t> &data,
                                  const int len, const uint32_t scramblingCode);
  std::vector<uint8_t> deinterleave(const std::vector<uint8_t> &data,
                                    const uint32_t K, const uint32_t a);
  std::vector<uint8_t> depuncture23(const std::vector<uint8_t> &data,
                                    const uint32_t len);
  std::vector<uint8_t> viterbiDecode1614(const std::vector<uint8_t> &data);
  std::vector<uint8_t> reedMuller3014Decode(const std::vector<uint8_t> &data);
  int checkCrc16Ccitt(const std::vector<uint8_t> &data, const int len);

  std::shared_ptr<UpperMac> _upperMac;
};

#endif
