#ifndef L2_UPPERMAC_HPP
#define L2_UPPERMAC_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include <l3/Mle.hpp>

enum DownlinkUsage {
  CommonControl,
  Unallocated,
  AssignedControl,
  CommonAndAssignedControl,
  Traffic
};

class UpperMac {
public:
  UpperMac() : _mle(std::make_shared<Mle>()){};
  ~UpperMac(){};

  void incrementTn();

  void processAACH(const std::vector<uint8_t> &data);
  void processBSCH(const std::vector<uint8_t> &data);
  void processSCH_HD(const std::vector<uint8_t> &data);

  uint32_t scramblingCode() const { return _scramblingCode; }
  uint16_t colorCode() const { return _colorCode; }
  int32_t downlinkFrequency() const { return _downlinkFrequency; }
  int32_t uplinkFrequency() const { return _uplinkFrequency; }
  enum DownlinkUsage downlinkUsage() const { return _downlinkUsage; }

  uint16_t tn() const { return _tn; }
  uint16_t fn() const { return _fn; }
  uint16_t mn() const { return _mn; }

  friend std::ostream &operator<<(std::ostream &stream,
                                  const UpperMac &upperMac);

private:
  void updateScramblingCode();

  void processMacResource(BitVec &vec){};
  void processMacEndAndMacFragment(BitVec &vec){};
  void processBroadcast(BitVec &vec);
  void processMacPdu(BitVec &vec){};

  void processSysinfoPdu(BitVec &vec);
  void processAccessDefinePdu(BitVec &vec){};
  void processSysinfoDa(BitVec &vec){};

  // SYNC PDU
  bool _syncReceived = false;
  uint8_t _systemCode;
  uint32_t _colorCode = 0;
  // time slot
  uint16_t _tn = 1;
  // frame number
  uint16_t _fn = 1;
  // multi frame number
  uint16_t _mn = 1;
  uint8_t _sharingMode;
  uint8_t _tsReservedFrames;
  uint8_t _uplaneDtx;
  uint8_t _frame18extension;

  uint32_t _scramblingCode = 0;

  // SYSINFO PDU
  bool _sysinfoReceived = false;
  int32_t _downlinkFrequency = 0;
  int32_t _uplinkFrequency = 0;
  uint8_t _numberOfSecondaryControlChannelsInUseOnCaMainCarrier;
  uint8_t _msTxpwrMaxCell;
  uint8_t _rxlevAccessMin;
  uint8_t _accessParameter;
  uint8_t _radioDownlinkTimeout;
  uint8_t _hyperFrameCipherKeyFlag;
  uint8_t _hyperframeNumber;
  uint8_t _commonCipherKeyIdentifierOrStaticCipherKeyVersionNumber;
  std::optional<uint32_t> _evenMultiframeDefinitionForTsMode;
  std::optional<uint32_t> _oddMultiframeDefinitionForTsMode;

  bool _sysinfoDefaultDefinitionForAccessCodeA = false;
  uint8_t _defaultDefinitionForAccessCodeAImmediate;
  uint8_t _defaultDefinitionForAccessCodeAWaitingtime;
  uint8_t
      _defaultDefinitionForAccessCodeANumberOfRandomAccessTransmissionsOnUplink;
  uint8_t _defaultDefinitionForAccessCodeAFramelengthFactor;
  uint8_t _defaultDefinitionForAccessCodeATimeslotPointer;
  uint8_t _defaultDefinitionForAccessCodeAMinimumPduPriority;

  bool _sysinfoExtendedServicesBroadcast = false;
  uint8_t _extendedServicesBroadcastSecurityInformation;
  uint8_t _extendedServicesBroadcastSdstlAddressingMethod;
  uint8_t _extendedServicesBroadcastGckSupported;

  bool _sysinfoExtendedServicesBroadcastSection1 = false;
  uint8_t _extendedServicesBroadcastDataPrioritySupported;
  uint8_t _extendedServicesBroadcastExtendedAdvancedLinksAndMacUBlckSupported;
  uint8_t _extendedServicesBroadcastQoSNegotiationSupported;
  uint8_t _extendedServicesBroadcastD8pskService;

  bool _sysinfoExtendedServicesBroadcastSection2 = false;
  uint8_t _extendedServicesBroadcast25QamService;
  uint8_t _extendedServicesBroadcast50QamService;
  uint8_t _extendedServicesBroadcast100QamService;
  uint8_t _extendedServicesBroadcast150QamService;

  // AACH
  enum DownlinkUsage _downlinkUsage;
  int _downlinkTrafficUsageMarker;

  std::shared_ptr<Mle> _mle;
};

std::ostream &operator<<(std::ostream &stream, const UpperMac &upperMac);

#endif
