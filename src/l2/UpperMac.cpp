#include <bitset>
#include <cassert>

#include <l2/UpperMac.hpp>
#include <utils/BitVec.hpp>

void UpperMac::incrementTn() {
  _tn++;

  // time slot
  if (_tn > 4) {
    _fn++;
    _tn = 1;
  }

  // frame number
  if (_fn > 18) {
    _mn++;
    _fn = 1;
  }

  // multi-frame number
  if (_mn > 60) {
    _mn = 1;
  }

  std::cout << "TN: " << _tn << " FN: " << _fn << " MN: " << _mn << std::endl;
}

/**
 * @brief Process ACCESS-ASSIGN - see 21.4.7.2
 *
 */
void UpperMac::processAACH(const std::vector<uint8_t> &data) {
  assert(data.size() == 14);

  auto vec = BitVec(data);

  auto header = vec.take(2);
  auto field1 = vec.take(6);
  auto field2 = vec.take(6);

  // TODO: parse uplink marker and some other things relevant for the uplink
  if (_fn == 18) {
    _downlinkUsage = DownlinkUsage::CommonControl;
  } else {
    if (header == 0b00) {
      _downlinkUsage = DownlinkUsage::CommonControl;
    } else {
      switch (field1) {
      case 0b00000:
        _downlinkUsage = DownlinkUsage::Unallocated;
        break;
      case 0b00001:
        _downlinkUsage = DownlinkUsage::AssignedControl;
        break;
      case 0b00010:
        _downlinkUsage = DownlinkUsage::CommonControl;
        break;
      case 0b00011:
        _downlinkUsage = DownlinkUsage::CommonAndAssignedControl;
        break;
      default:
        _downlinkUsage = DownlinkUsage::Traffic;
        _downlinkTrafficUsageMarker = field1;
        break;
      }
    }
  }

  std::cout << "AACH downlinkUsage: " << _downlinkUsage
            << " downlinkUsageTrafficMarker: " << _downlinkTrafficUsageMarker
            << std::endl;
}

/**
 * @brief Process SYNC - see 21.4.4.2
 *
 */
void UpperMac::processBSCH(const std::vector<uint8_t> &data) {
  assert(data.size() == 60);

  auto vec = BitVec(data);

  assert(vec.size() == 60);

  _systemCode = vec.take(4);
  _colorCode = vec.take(6);
  _tn = vec.take(2) + 1;
  _fn = vec.take(5);
  _mn = vec.take(6);
  _sharingMode = vec.take(2);
  _tsReservedFrames = vec.take(3);
  _uplaneDtx = vec.take(1);
  _frame18extension = vec.take(1);
  auto reserved = vec.take(1);

  assert(vec.size() == 29);

  _mle->serviceDMleSync(vec);
  updateScramblingCode();

  _syncReceived = true;

  std::cout << *this;
}

void UpperMac::processSCH_HD(const std::vector<uint8_t> &data) {
  assert(data.size() == 124);

  auto vec = BitVec(data);

  auto pduType = vec.take(2);

  switch (pduType) {
  case 0b00:
    // MAC-RESOURCE (downlink)
    processMacResource(vec);
    break;
  case 0b01:
    // MAC-END or MAC-FRAG
    processMacEndAndMacFragment(vec);
    break;
  case 0b10:
    // Broadcast
    processBroadcast(vec);
    break;
  case 0b11:
    // Supplementary MAC PDU (not on STCH, SCH/HD or SCH-P8/HD)
    // MAC-U-SIGNAL (only on STCH)
    processMacPdu(vec);
    break;
  }
}

void UpperMac::processBroadcast(BitVec &vec) {
  auto broadcastType = vec.take(2);
  switch (broadcastType) {
  case 0b00:
    // SYSINFO PDU
    processSysinfoPdu(vec);
    break;
  case 0b01:
    // ACCESS-DEFINE PDU
    processAccessDefinePdu(vec);
    break;
  case 0b10:
    // SYSINFO-DA
    // only QAM
    processSysinfoDa(vec);
    break;
  case 0b11:
    // Reserved
    break;
  }
}

void UpperMac::processSysinfoPdu(BitVec &vec) {
  auto mainCarrier = vec.take(12);
  auto frequencyBand = vec.take(4);
  auto offset = vec.take(2);
  auto duplexSpacing = vec.take(3);
  auto reverseOperation = vec.take(1);
  _numberOfSecondaryControlChannelsInUseOnCaMainCarrier = vec.take(2);
  _msTxpwrMaxCell = vec.take(3);
  _rxlevAccessMin = vec.take(4);
  _accessParameter = vec.take(4);
  _radioDownlinkTimeout = vec.take(4);
  _hyperFrameCipherKeyFlag = vec.take(1);
  if (_hyperFrameCipherKeyFlag == 0) {
    _hyperframeNumber = vec.take(16);
  } else {
    _commonCipherKeyIdentifierOrStaticCipherKeyVersionNumber = vec.take(16);
  }
  auto optionalFieldFlag = vec.take(2);
  if (optionalFieldFlag == 0b00) {
    *_evenMultiframeDefinitionForTsMode = vec.take(20);
  } else if (optionalFieldFlag == 0b01) {
    *_oddMultiframeDefinitionForTsMode = vec.take(20);
  } else if (optionalFieldFlag == 0b10) {
    _defaultDefinitionForAccessCodeAImmediate = vec.take(4);
    _defaultDefinitionForAccessCodeAWaitingtime = vec.take(4);
    _defaultDefinitionForAccessCodeANumberOfRandomAccessTransmissionsOnUplink =
        vec.take(4);
    _defaultDefinitionForAccessCodeAFramelengthFactor = vec.take(1);
    _defaultDefinitionForAccessCodeATimeslotPointer = vec.take(4);
    _defaultDefinitionForAccessCodeAMinimumPduPriority = vec.take(3);
    _sysinfoDefaultDefinitionForAccessCodeA = true;
  } else if (optionalFieldFlag == 0b11) {
    _extendedServicesBroadcastSecurityInformation = vec.take(8);
    _extendedServicesBroadcastSdstlAddressingMethod = vec.take(2);
    _extendedServicesBroadcastGckSupported = vec.take(1);

    auto section = vec.take(2);
    if (section == 0b00) {
      _extendedServicesBroadcastDataPrioritySupported = vec.take(1);
      _extendedServicesBroadcastExtendedAdvancedLinksAndMacUBlckSupported =
          vec.take(1);
      _extendedServicesBroadcastQoSNegotiationSupported = vec.take(1);
      _extendedServicesBroadcastD8pskService = vec.take(1);
      auto sectionInformation = vec.take(3);
      _sysinfoExtendedServicesBroadcastSection1 = true;
    } else if (section == 0b01) {
      _extendedServicesBroadcast25QamService = vec.take(1);
      _extendedServicesBroadcast50QamService = vec.take(1);
      _extendedServicesBroadcast100QamService = vec.take(1);
      _extendedServicesBroadcast150QamService = vec.take(1);
      auto reserved = vec.take(3);
      _sysinfoExtendedServicesBroadcastSection2 = true;
    } else {
      // TODO: Section 2 and 3 are reserved
      auto reserved = vec.take(7);
    }

    _sysinfoExtendedServicesBroadcast = true;
  }

  // downlink main carrier frequency = base frequency + (main carrier × 25 kHz)
  // + offset kHz.
  const int32_t duplex[4] = {0, 6250, -6250, 12500};
  _downlinkFrequency =
      frequencyBand * 100000000 + mainCarrier * 25000 + duplex[offset];

  static const int32_t tetra_duplex_spacing[8][16] = {
      /* values are in kHz */
      [0] = {-1, 1600, 10000, 10000, 10000, 10000, 10000, -1, -1, -1, -1, -1,
             -1, -1, -1, -1},
      [1] = {-1, 4500, -1, 36000, 7000, -1, -1, -1, 45000, 45000, -1, -1, -1,
             -1, -1, -1},
      [2] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      [3] = {-1, -1, -1, 8000, 8000, -1, -1, -1, 18000, 18000, -1, -1, -1, -1,
             -1, -1},
      [4] = {-1, -1, -1, 18000, 5000, -1, 30000, 30000, -1, 39000, -1, -1, -1,
             -1, -1, -1},
      [5] = {-1, -1, -1, -1, 9500, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
      [6] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
      [7] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  };
  uint32_t duplex_spacing = tetra_duplex_spacing[duplexSpacing][frequencyBand];
  /* reserved for future standardization */
  if (duplex_spacing < 0) {
    _uplinkFrequency = 0;
  } else if (reverseOperation) {
    _uplinkFrequency = _downlinkFrequency + duplex_spacing * 1000;
  } else {
    _uplinkFrequency = _downlinkFrequency - duplex_spacing * 1000;
  }

  // This element shall be present when the PDU is sent using π/8-D8PSK
  // modulation. This element shall not be present when the PDU is sent using
  // π/4-DQPSK modulation.
  // auto reserved = vec.take(28);

  _mle->serviceDMleSysinfo(vec);

  _sysinfoReceived = true;

  std::cout << *this;
}

void UpperMac::updateScramblingCode() {
  // 10 MSB of MCC
  uint16_t lmcc = _mle->mcc() & 0x03ff;
  // 14 MSB of MNC
  uint16_t lmnc = _mle->mnc() & 0x3fff;
  // 6 MSB of ColorCode
  uint16_t lcolor_code = _colorCode & 0x003f;

  // 30 MSB bits
  _scramblingCode = lcolor_code | (lmnc << 6) | (lmcc << 20);
  // scrambling initialized to 1 on bits 31-32 - 8.2.5.2 (54)
  _scramblingCode = (_scramblingCode << 2) | 0x0003;
}

std::ostream &operator<<(std::ostream &stream, const UpperMac &upperMac) {
  if (upperMac._syncReceived) {
    stream << "SYNC:" << std::endl;
    stream << "  System code: 0b" << std::bitset<4>(upperMac._systemCode)
           << std::endl;
    stream << "  Color code: " << std::to_string(upperMac._colorCode)
           << std::endl;
    stream << "  TN/FN/MN: " << std::to_string(upperMac._tn) << "/"
           << std::to_string(upperMac._fn) << "/"
           << std::to_string(upperMac._mn) << std::endl;
    stream << "  Scrambling code: " << std::to_string(upperMac._scramblingCode)
           << std::endl;
    std::string sharing_mode_map[] = {"Continuous transmission",
                                      "Carrier sharing", "MCCH sharing",
                                      "Traffic carrier sharing"};
    stream << "  Sharing mode: " << sharing_mode_map[upperMac._sharingMode]
           << std::endl;
    uint8_t ts_reserved_frames_map[] = {1, 2, 3, 4, 6, 9, 12, 18};
    stream << "  TS reserved frames: "
           << std::to_string(ts_reserved_frames_map[upperMac._tsReservedFrames])
           << " frames reserved per 2 multiframes" << std::endl;
    stream << "  "
           << (upperMac._uplaneDtx
                   ? "Discontinuous U-plane transmission is allowed"
                   : "Discontinuous U-plane transmission is not allowed")
           << std::endl;
    stream << "  "
           << (upperMac._frame18extension ? "Frame 18 extension allowed"
                                          : "No frame 18 extension")
           << std::endl;
  }

  if (upperMac._sysinfoReceived) {
    stream << "SYSINFO:" << std::endl;
    stream << "  DL " << std::to_string(upperMac._downlinkFrequency) << "Hz UL "
           << std::to_string(upperMac._uplinkFrequency) << "Hz" << std::endl;
    stream << "  Number of common secondary control channels in use on CA main "
              "carrier: ";
    switch (upperMac._numberOfSecondaryControlChannelsInUseOnCaMainCarrier) {
    case 0b00:
      stream << "None";
      break;
    case 0b01:
      stream << "Timeslot 2 of main carrier";
      break;
    case 0b10:
      stream << "Timeslots 2 and 3 of main carrier";
      break;
    case 0b11:
      stream << "Timeslots 2, 3 and 4 of main carrier";
      break;
    }
    stream << std::endl;
    stream << "  MS_TXPWR_MAX_CELL: ";
    switch (upperMac._msTxpwrMaxCell) {
    case 0b000:
      stream << "Reserved";
      break;
    default:
      stream << std::to_string(10 + 5 * upperMac._msTxpwrMaxCell) << " dBm";
      break;
    }
    stream << std::endl;
    stream << "  RXLEV_ACCESS_MIN: "
           << std::to_string(-125 + 5 * upperMac._rxlevAccessMin) << " dBm"
           << std::endl;
    stream << "  ACCESS_PARAMETER: "
           << std::to_string(-53 + 2 * upperMac._accessParameter) << " dBm"
           << std::endl;
    stream << "  RADIO_DOWNLINK_TIMEOUT: ";
    switch (upperMac._radioDownlinkTimeout) {
    case 0b0000:
      stream << "Disable radio downlink counter";
      break;
    default:
      stream << std::to_string(144 * upperMac._radioDownlinkTimeout)
             << " timeslots";
      break;
    }
    stream << std::endl;
    if (upperMac._hyperFrameCipherKeyFlag) {
      stream << "  Common cipher key identifier: "
             << std::to_string(upperMac._hyperFrameCipherKeyFlag) << std::endl;
    } else {
      stream << "  Cyclic count of hyperframes: "
             << std::to_string(upperMac._hyperframeNumber) << std::endl;
    }

    if (upperMac._evenMultiframeDefinitionForTsMode.has_value()) {
      stream << "  Bit map of common frames for TS mode (even multiframes): 0b"
             << std::bitset<20>(*upperMac._evenMultiframeDefinitionForTsMode)
             << std::endl;
    }
    if (upperMac._oddMultiframeDefinitionForTsMode.has_value()) {
      stream << "  Bit map of common frames for TS mode (odd multiframes): 0b"
             << std::bitset<20>(*upperMac._oddMultiframeDefinitionForTsMode)
             << std::endl;
    }

    if (upperMac._sysinfoDefaultDefinitionForAccessCodeA) {
      stream << "  Default definition for access code A information element:"
             << std::endl;
      stream << "    Immediate: 0b"
             << std::bitset<4>(
                    upperMac._extendedServicesBroadcastSecurityInformation)
             << std::endl;
      stream << "    Waiting time: 0b"
             << std::bitset<4>(
                    upperMac._defaultDefinitionForAccessCodeAWaitingtime)
             << std::endl;
      stream
          << "    Number of random access transmissions on uplink: 0b"
          << std::bitset<4>(
                 upperMac
                     ._defaultDefinitionForAccessCodeANumberOfRandomAccessTransmissionsOnUplink)
          << std::endl;
      stream << "    Frame-length factor: 0b"
             << std::bitset<1>(
                    upperMac._defaultDefinitionForAccessCodeAFramelengthFactor)
             << std::endl;
      stream << "    Timeslot pointer: 0b"
             << std::bitset<4>(
                    upperMac._defaultDefinitionForAccessCodeATimeslotPointer)
             << std::endl;
      stream << "    Minimum PDU priority: 0b"
             << std::bitset<3>(
                    upperMac._defaultDefinitionForAccessCodeAMinimumPduPriority)
             << std::endl;
    }

    if (upperMac._sysinfoExtendedServicesBroadcast) {
      stream << "  Extended services broadcast:" << std::endl;
      stream << "    Security information: 0b"
             << std::bitset<8>(
                    upperMac._extendedServicesBroadcastSecurityInformation)
             << std::endl;
      stream << "    SDS-TL addressing method: 0b"
             << std::bitset<2>(
                    upperMac._extendedServicesBroadcastSdstlAddressingMethod)
             << std::endl;
      stream << "    GCK supported: 0b"
             << std::bitset<1>(upperMac._extendedServicesBroadcastGckSupported)
             << std::endl;
      if (upperMac._sysinfoExtendedServicesBroadcastSection1) {
        stream << "    Data priority supported: 0b"
               << std::bitset<1>(
                      upperMac._extendedServicesBroadcastDataPrioritySupported)
               << std::endl;
        stream
            << "    Extended advanced links and MAC-U-BLCK supported: 0b"
            << std::bitset<1>(
                   upperMac
                       ._extendedServicesBroadcastExtendedAdvancedLinksAndMacUBlckSupported)
            << std::endl;
        stream
            << "    QoS negotiation supported: 0b"
            << std::bitset<1>(
                   upperMac._extendedServicesBroadcastQoSNegotiationSupported)
            << std::endl;
        stream << "    D8PSK service: 0b"
               << std::bitset<1>(
                      upperMac._extendedServicesBroadcastD8pskService)
               << std::endl;
      }

      if (upperMac._sysinfoExtendedServicesBroadcastSection2) {
        stream << "    25 kHz QAM service: 0b"
               << std::bitset<1>(
                      upperMac._extendedServicesBroadcast25QamService)
               << std::endl;
        stream << "    50 kHz QAM service: 0b"
               << std::bitset<1>(
                      upperMac._extendedServicesBroadcast50QamService)
               << std::endl;
        stream << "    100 kHz QAM service: 0b"
               << std::bitset<1>(
                      upperMac._extendedServicesBroadcast100QamService)
               << std::endl;
        stream << "    150 kHz QAM service: 0b"
               << std::bitset<1>(
                      upperMac._extendedServicesBroadcast150QamService)
               << std::endl;
      }
    }
  }

  return stream;
}
