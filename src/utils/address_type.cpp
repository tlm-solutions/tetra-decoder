#include <ostream>

#include <utils/address_type.hpp>

auto operator<<(std::ostream& stream, const AddressType& address_type) -> std::ostream& {
    if (address_type.country_code_) {
        stream << "Country Code: " << address_type.country_code_.value().to_ulong() << " ";
    }
    if (address_type.network_code_) {
        stream << "Network Code: " << address_type.network_code_.value().to_ulong() << " ";
    }
    if (address_type.sna_) {
        stream << "SNA: " << address_type.sna_.value().to_ulong() << " ";
    }
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
