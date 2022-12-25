#ifndef L3_MLE_HPP
#define L3_MLE_HPP

#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

#include <utils/BitVec.hpp>

class Mle {
public:
  Mle(){};
  ~Mle(){};

  uint32_t mcc() const { return _mcc; }
  uint32_t mnc() const { return _mnc; }

  void serviceDMleSync(BitVec &vec);
  void serviceDMleSysinfo(BitVec &vec);

  friend std::ostream &operator<<(std::ostream &stream, const Mle &mle);

private:
  bool _syncReceived = false;
  uint32_t _mcc = 0;
  uint32_t _mnc = 0;
  uint8_t _dNwrkBroadcastBroadcastSupported;
  uint8_t _dNwrkBroadcastEnquirySupported;
  uint8_t _cellLoadCA;
  uint8_t _lateEntrySupported;

  bool _sysinfoReceived = false;
  uint16_t _locationArea;
  uint16_t _subscriberClass;
  uint8_t _registration;
  uint8_t _deRegistration;
  uint8_t _priorityCell;
  uint8_t _minimumModeService;
  uint8_t _migration;
  uint8_t _systemWideService;
  uint8_t _tetraVoiceService;
  uint8_t _circuitModeDataService;
  uint8_t _sndcpService;
  uint8_t _airInterfaceEncryptionService;
  uint8_t _advancedLinkSupported;
};

std::ostream &operator<<(std::ostream &stream, const Mle &mle);

#endif
