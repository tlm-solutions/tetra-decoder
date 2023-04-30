#include <ostream>

#include <l2/address_type.hpp>

std::ostream& operator<<(std::ostream& stream, const AddressType& address_type) {
    if (address_type.ssi_.has_value()) {
        stream << "SSI: " << address_type.ssi_.value() << " ";
    }
    if (address_type.ussi_.has_value()) {
        stream << "USSI: " << address_type.ussi_.value() << " ";
    }
    if (address_type.smi_.has_value()) {
        stream << "SMI: " << address_type.smi_.value() << " ";
    }
    if (address_type.event_label_.has_value()) {
        stream << "Event Label: " << address_type.event_label_.value() << " ";
    }
    if (address_type.usage_marker_.has_value()) {
        stream << "Usage Marker: " << address_type.usage_marker_.value() << " ";
    }

    stream << std::endl;
}
