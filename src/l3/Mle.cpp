#include <bitset>
#include <cassert>

#include <l3/Mle.hpp>

void Mle::serviceDMleSync(BitVec &vec) {
  assert(vec.size() == 29);

  _mcc = vec.take(10);
  _mnc = vec.take(14);
  _dNwrkBroadcastBroadcastSupported = vec.take(1);
  _dNwrkBroadcastEnquirySupported = vec.take(1);
  _cellLoadCA = vec.take(2);
  _lateEntrySupported = vec.take(1);

  _syncReceived = true;

  std::cout << *this;
}

void Mle::serviceDMleSysinfo(BitVec &vec) {
  assert(vec.size() == 42);

  // Location area (14)
  _locationArea = vec.take(14);
  // Subscriber class (16)
  _subscriberClass = vec.take(16);
  // BS service details (12)
  _registration = vec.take(1);
  _deRegistration = vec.take(1);
  _priorityCell = vec.take(1);
  _minimumModeService = vec.take(1);
  _migration = vec.take(1);
  _systemWideService = vec.take(1);
  _tetraVoiceService = vec.take(1);
  _circuitModeDataService = vec.take(1);
  auto reserved = vec.take(1);
  _sndcpService = vec.take(1);
  _airInterfaceEncryptionService = vec.take(1);
  _advancedLinkSupported = vec.take(1);

  _sysinfoReceived = true;

  std::cout << *this;
}

std::ostream &operator<<(std::ostream &stream, const Mle &mle) {
  if (mle._syncReceived) {
    stream << "D-MLE-SYNC:" << std::endl;
    stream << "  MCC: " << mle._mcc << std::endl;
    stream << "  MNC: " << mle._mnc << std::endl;
    stream << "  Neighbour cell broadcast: "
           << (mle._dNwrkBroadcastBroadcastSupported ? "supported"
                                                     : "not supported")
           << std::endl;
    stream << "  Neighbour cell enquiry: "
           << (mle._dNwrkBroadcastEnquirySupported ? "supported"
                                                   : "not supported")
           << std::endl;
    stream << "  Cell load CA: ";
    switch (mle._cellLoadCA) {
    case 0b00:
      stream << "Cell load unknown";
      break;
    case 0b01:
      stream << "Low cell load";
      break;
    case 0b10:
      stream << "Medium cell load";
      break;
    case 0b11:
      stream << "High cell load";
      break;
    default:
      break;
    }
    stream << std::endl;
    stream << "  Late entry supported: "
           << (mle._lateEntrySupported ? "Late entry available"
                                       : "Late entry not supported")
           << std::endl;
  }

  if (mle._sysinfoReceived) {
    stream << "D-MLE-SYSINFO:" << std::endl;
    stream << "  Location Area (LA): " << mle._locationArea << std::endl;
    stream << "  Subscriber Class 1..16 allowed: 0b"
           << std::bitset<16>(mle._subscriberClass) << std::endl;
    stream << "  "
           << (mle._registration ? "Registration mandatory on this cell"
                                 : "Registration not required on this cell")
           << std::endl;
    stream << "  "
           << (mle._deRegistration
                   ? "De-registration requested on this cell"
                   : "De-registration not required on this cell")
           << std::endl;
    stream << "  "
           << (mle._priorityCell ? "Cell is a priority cell"
                                 : "Cell is not a priority cell")
           << std::endl;
    stream << "  "
           << (mle._minimumModeService ? "Cell never uses minimum mode"
                                       : "Cell may use minimum mode")
           << std::endl;
    stream << "  "
           << (mle._migration ? "Migration is supported by this cell"
                              : "Migration is not supported by this cell")
           << std::endl;
    stream << "  "
           << (mle._systemWideService
                   ? "Normal mode (system wide services supported)"
                   : "System wide services temporarily not supported")
           << std::endl;
    stream << "  "
           << (mle._tetraVoiceService
                   ? "TETRA voice service is supported on this cell"
                   : "TETRA voice service is not supported on this cell")
           << std::endl;
    stream << "  "
           << (mle._circuitModeDataService
                   ? "Circuit mode data service is supported on this cell"
                   : "Circuit mode data service is not supported on this cell")
           << std::endl;
    stream << "  "
           << (mle._sndcpService
                   ? "SNDCP service is available on this cell"
                   : "SNDCP service is not available on this cell")
           << std::endl;
    stream << "  "
           << (mle._airInterfaceEncryptionService
                   ? "Air interface encryption is available on this cell"
                   : "Air interface encryption is not available on this cell")
           << std::endl;
    stream << "  "
           << (mle._advancedLinkSupported
                   ? "Advanced link is supported on this cell"
                   : "Advanced link is not supported on this cell")
           << std::endl;
  }

  return stream;
}
