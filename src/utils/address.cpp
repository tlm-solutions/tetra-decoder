#include "utils/address.hpp"
#include <bitset>
#include <ostream>

auto operator<<(std::ostream& stream, const Address& address_type) -> std::ostream& {
    if (address_type.country_code_) {
        stream << "Country Code: " << std::bitset<10>(*address_type.country_code_) << " ";
    }
    if (address_type.network_code_) {
        stream << "Network Code: " << std::bitset<14>(*address_type.network_code_) << " ";
    }
    if (address_type.sna_) {
        stream << "SNA: " << std::bitset<8>(*address_type.sna_) << " ";
    }
    if (address_type.ssi_) {
        stream << "SSI: " << std::bitset<24>(*address_type.ssi_) << " ";
    }
    if (address_type.ussi_) {
        stream << "USSI: " << std::bitset<24>(*address_type.ussi_) << " ";
    }
    if (address_type.smi_) {
        stream << "SMI: " << std::bitset<24>(*address_type.smi_) << " ";
    }
    if (address_type.event_label_) {
        stream << "Event Label: " << std::bitset<10>(*address_type.event_label_) << " ";
    }
    if (address_type.usage_marker_) {
        stream << "Usage Marker: " << std::bitset<6>(*address_type.usage_marker_) << " ";
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