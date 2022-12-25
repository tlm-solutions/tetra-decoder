#include <l2/LowerMac.hpp>

LowerMac::LowerMac() {
  /*
   * Initialize Viterbi coder/decoder for MAC
   *
   * 8.2.3.1.1 Generator polynomials for the RCPC 16-state mother code of rate
   * 1/4
   *
   * G1 = 1 + D +             D^4 (8.3)
   * G2 = 1 +     D^2 + D^3 + D^4 (8.4)
   * G3 = 1 + D + D^2 +       D^4 (8.5)
   * G4 = 1 + D +       D^3 + D^4 (8.6)
   *
   * NOTE: representing bit order must be reversed for the codec, eg. 1 + D + 0
   * + 0 + D^4 -> 10011
   *
   */
  std::vector<int> polynomials;
  int constraint = 6;

  polynomials.push_back(0b10011);
  polynomials.push_back(0b11101);
  polynomials.push_back(0b10111);
  polynomials.push_back(0b11011);
  _viterbiCodec1614 = std::make_unique<ViterbiCodec>(constraint, polynomials);

  _upperMac = std::make_unique<UpperMac>();
}

static std::vector<uint8_t> vectorExtract(const std::vector<uint8_t> &vec,
                                          size_t pos, size_t length) {
  std::vector<uint8_t> res;

  std::copy(vec.begin() + pos, vec.begin() + pos + length,
            std::back_inserter(res));

  return res;
}

static std::vector<uint8_t> vectorAppend(const std::vector<uint8_t> &vec,
                                         std::vector<uint8_t> &res, size_t pos,
                                         size_t length) {
  std::copy(vec.begin() + pos, vec.begin() + pos + length,
            std::back_inserter(res));

  return res;
}

static void vectorPrint(const std::vector<uint8_t> &vec) {
  for (auto it = vec.begin(); it != vec.end(); it++) {
    std::cout << std::to_string(*it) << " ";
  }
  std::cout << std::endl;
}

void LowerMac::process(const std::vector<uint8_t> &frame, BurstType burstType) {
  std::vector<uint8_t> sb;
  std::vector<uint8_t> bkn1;
  std::vector<uint8_t> bkn2;
  std::vector<uint8_t> bb;

  // The BLCH may be mapped onto block 2 of the downlink slots, when a SCH/HD,
  // SCH-P8/HD or a BSCH is mapped onto block 1. The number of BLCH occurrences
  // on one carrier shall not exceed one per 4 multiframe periods.
  if (burstType == BurstType::SynchronizationBurst) {
    _upperMac->incrementTn();

    // sb contains BSCH
    sb = vectorExtract(frame, 94, 120);
    sb = descramble(sb, 120, 0x0003);
    sb = deinterleave(sb, 120, 11);
    sb = depuncture23(sb, 120);
    sb = viterbiDecode1614(sb);
    if (checkCrc16Ccitt(sb, 76)) {
      sb = vectorExtract(sb, 0, 60);
      _upperMac->processBSCH(sb);
    }

    // bb contains AACH
    bb = vectorExtract(frame, 252, 30);
    bb = descramble(bb, 30, _upperMac->scramblingCode());
    bb = reedMuller3014Decode(bb);
    _upperMac->processAACH(bb);

    // bkn2 block
    // any off SCH/HD, BNCH, STCH
    // see ETSI EN 300 392-2 V3.8.1 (2016-08) Figure 8.6: Error control
    // structure for π4DQPSK logical channels (part 2)
    bkn2 = vectorExtract(frame, 282, 216);
    bkn2 = descramble(bkn2, 216, _upperMac->scramblingCode());
    bkn2 = deinterleave(bkn2, 216, 101);
    bkn2 = depuncture23(bkn2, 216);
    bkn2 = viterbiDecode1614(bkn2);
    // if the crc does not work, then it might be a BLCH
    if (checkCrc16Ccitt(bkn2, 140)) {
      bkn2 = vectorExtract(bkn2, 0, 124);

      // SCH/HD or BNCH mapped
      _upperMac->processSCH_HD(bkn2);
    }
  } else if (burstType == BurstType::NormalDownlinkBurst) {
    _upperMac->incrementTn();

    // bb contains AACH
    bb = vectorExtract(frame, 230, 14);
    bb = vectorAppend(frame, bb, 266, 16);
    bb = descramble(bb, 30, _upperMac->scramblingCode());
    bb = reedMuller3014Decode(bb);
    _upperMac->processAACH(bb);

    // TCH or SCH/F
    bkn1 = vectorExtract(frame, 14, 216);
    bkn1 = vectorAppend(frame, bkn1, 282, 216);
    bkn1 = descramble(bkn1, 432, _upperMac->scramblingCode());
    // TODO: handle TCH or SCH/F

    if (_upperMac->downlinkUsage() == DownlinkUsage::Traffic &&
        _upperMac->tn() <= 17) {

    } else {
      // control channel
      bkn1 = deinterleave(bkn1, 432, 103);
      bkn1 = depuncture23(bkn1, 432);
      bkn1 = viterbiDecode1614(bkn1);
      if (checkCrc16Ccitt(bkn1, 284)) {
        bkn1 = vectorExtract(bkn1, 0, 268);
        // TODO: fix
        _upperMac->processSCH_HD(bkn1);
      }
    }
  } else if (burstType == BurstType::NormalDownlinkBurst_Split) {
    _upperMac->incrementTn();

    // bb contains AACH
    bb = vectorExtract(frame, 230, 14);
    bb = vectorAppend(frame, bb, 266, 16);
    bb = descramble(bb, 30, _upperMac->scramblingCode());
    bb = reedMuller3014Decode(bb);
    _upperMac->processAACH(bb);

    // STCH + TCH
    // STCH + STCH
    // SCH/HD + SCH/HD
    // SCH/HD + BNCH
    bkn1 = vectorExtract(frame, 14, 216);
    bkn2 = vectorExtract(frame, 282, 216);
    // TODO: handle this

    // if (_upperMac->downlinkUsage() == DownlinkUsage::Traffic &&
    // _upperMac->tn() <= 17) {

    //} else {
    bkn1 = descramble(bkn1, 216, _upperMac->scramblingCode());
    bkn1 = deinterleave(bkn1, 216, 101);
    bkn1 = depuncture23(bkn1, 216);
    bkn1 = viterbiDecode1614(bkn1);
    if (checkCrc16Ccitt(bkn1, 140)) {
      bkn1 = vectorExtract(bkn1, 0, 124);
      _upperMac->processSCH_HD(bkn1);
    }
    // control channel
    bkn2 = descramble(bkn2, 216, _upperMac->scramblingCode());
    bkn2 = deinterleave(bkn2, 216, 101);
    bkn2 = depuncture23(bkn2, 216);
    bkn2 = viterbiDecode1614(bkn2);
    if (checkCrc16Ccitt(bkn2, 140)) {
      bkn2 = vectorExtract(bkn2, 0, 124);
      _upperMac->processSCH_HD(bkn2);
    }
    //}
  } else {
    throw std::runtime_error(
        "LowerMac does not implement the burst type supplied");
  }
}
