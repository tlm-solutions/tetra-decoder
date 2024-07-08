#include "utils/address.hpp"
#include <ostream>

auto operator<<(std::ostream& stream, const Address& address_type) -> std::ostream& {
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

auto Address::from_mac_access(BitVector& data) -> Address {
    auto address_type = data.take<2>();

    Address address;
    if (address_type == 0b00) {
        address.set_ssi(data.take<24>());
    } else if (address_type == 0b01) {
        address.set_event_label(data.take<10>());
    } else if (address_type == 0b11) {
        address.set_ussi(data.take<24>());
    } else if (address_type == 0b11) {
        address.set_smi(data.take<24>());
    }

    return address;
}

auto Address::from_mac_data(BitVector& data) -> Address {
    auto address_type = data.take<2>();

    Address address;
    if (address_type == 0b00) {
        address.set_ssi(data.take<24>());
    } else if (address_type == 0b01) {
        address.set_event_label(data.take<10>());
    } else if (address_type == 0b11) {
        address.set_ussi(data.take<24>());
    } else if (address_type == 0b11) {
        address.set_smi(data.take<24>());
    }

    return address;
}

auto Address::from_mac_resource(BitVector& data) -> Address {
    auto address_type = data.take<3>();

    Address address;
    if (address_type == 0b001) {
        address.set_ssi(data.take<24>());
    } else if (address_type == 0b010) {
        address.set_event_label(data.take<10>());
    } else if (address_type == 0b011) {
        address.set_ussi(data.take<24>());
    } else if (address_type == 0b100) {
        address.set_smi(data.take<24>());
    } else if (address_type == 0b101) {
        address.set_ssi(data.take<24>());
        address.set_event_label(data.take<10>());
    } else if (address_type == 0b110) {
        address.set_ssi(data.take<24>());
        address.set_usage_marker(data.take<6>());
    } else if (address_type == 0b111) {
        address.set_smi(data.take<24>());
        address.set_event_label(data.take<10>());
    }

    return address;
}