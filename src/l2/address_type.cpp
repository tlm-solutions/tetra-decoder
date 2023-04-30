#include <ostream>

#include <l2/address_type.hpp>

std::ostream& operator<<(std::ostream& stream, const AddressType& address_type) {
    if (address_type.ssi_) {
        stream << "SSI: " << address_type.ssi_.value().to_ulong() << " ";
    }
    if (address_type.ussi_) {
        stream << "USSI: " << address_type.ussi_.value().to_ulong() << " ";
    }
    if (address_type.smi_) {
        stream << "SMI: " << address_type.smi_.value().to_ulong() << " ";
    }
    if (address_type.event_label_) {
        stream << "Event Label: " << address_type.event_label_.value().to_ulong() << " ";
    }
    if (address_type.usage_marker_) {
        stream << "Usage Marker: " << address_type.usage_marker_.value().to_ulong() << " ";
    }

    return stream;
}
