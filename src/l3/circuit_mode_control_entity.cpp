#include <cassert>
#include <iostream>

#include <l3/circuit_mode_control_entity.hpp>

void CircuitModeControlEntity::process(const Address address, BitVector& vec) {
    auto pdu_type = vec.take<5>();

    const auto& pdu_name = cmce_pdu_description_.at(pdu_type);
    std::cout << "CMCE " << pdu_name << std::endl;

    if (metrics_) {
        metrics_->increment(pdu_name);
    }

    switch (pdu_type) {
    case kSdsData:
        if (is_downlink_) {
            process_d_sds_data(address, vec);
        } else {
            process_u_sds_data(address, vec);
        }
        break;
    default:
        break;
    }
}

void CircuitModeControlEntity::process_d_sds_data(const Address to_address, BitVector& vec) {
    auto calling_party_type_identifier = vec.take<2>();
    Address from_address;

    if (calling_party_type_identifier == 1 || calling_party_type_identifier == 2) {
        from_address.set_ssi(vec.take<24>());
    }
    if (calling_party_type_identifier == 2) {
        from_address.set_country_code(vec.take<10>());
        from_address.set_network_code(vec.take<14>());
    }

    auto short_data_type_identifier = vec.take<2>();

    std::cout << "  Address: " << from_address << std::endl;
    std::cout << "  short_data_type_identifier: " << static_cast<unsigned>(short_data_type_identifier) << std::endl;

    unsigned length_identifier = 0;
    switch (short_data_type_identifier) {
    case 0b00:
        length_identifier = 16;
        break;
    case 0b01:
        length_identifier = 32;
        break;
    case 0b10:
        length_identifier = 64;
        break;
    case 0b11:
        length_identifier = vec.take<11>();
        break;
    }
    std::cout << "  length_identifier: " << static_cast<unsigned>(length_identifier) << std::endl;
    auto sds_data = vec.take_vector(length_identifier);
    sds_.process(to_address, from_address, sds_data);
    std::cout << "  leftover: " << vec << std::endl;
}

void CircuitModeControlEntity::process_u_sds_data(const Address from_address, BitVector& vec) {
    auto area_selection = vec.take<4>();
    auto calling_party_type_identifier = vec.take<2>();
    Address to_address;

    if (calling_party_type_identifier == 0) {
        to_address.set_sna(vec.take<8>());
    }
    if (calling_party_type_identifier == 1 || calling_party_type_identifier == 2) {
        to_address.set_ssi(vec.take<24>());
    }
    if (calling_party_type_identifier == 2) {
        to_address.set_country_code(vec.take<10>());
        to_address.set_network_code(vec.take<14>());
    }

    auto short_data_type_identifier = vec.take<2>();

    std::cout << "  Address: " << to_address << std::endl;
    std::cout << "  short_data_type_identifier: " << static_cast<unsigned>(short_data_type_identifier) << std::endl;

    unsigned length_identifier = 0;
    switch (short_data_type_identifier) {
    case 0b00:
        length_identifier = 16;
        break;
    case 0b01:
        length_identifier = 32;
        break;
    case 0b10:
        length_identifier = 64;
        break;
    case 0b11:
        length_identifier = vec.take<11>();
        break;
    }
    std::cout << "  length_identifier: " << static_cast<unsigned>(length_identifier) << std::endl;
    auto sds_data = vec.take_vector(length_identifier);
    sds_.process(to_address, from_address, sds_data);
    std::cout << "  leftover: " << vec << std::endl;
}