#include <cassert>
#include <iostream>

#include <l3/circuit_mode_control_entity.hpp>

void CircuitModeControlEntity::process(bool is_downlink, const Address address, BitVector& vec) {
    std::string cmce_downlink_pdu[] = {"D-ALERT",        "D-CALL-PROCEEDING",
                                       "D-CONNECT",      "D-CONNECT ACKNOWLEDGE",
                                       "D-DISCONNECT",   "D-INFO",
                                       "D-RELEASE",      "D-SETUP",
                                       "D-STATUS",       "D-TX CEASED",
                                       "D-TX CONTINUE",  "D-TX GRANTED",
                                       "D-TX WAIT",      "D-TX INTERRUPT",
                                       "D-CALL-RESTORE", "D-SDS-DATA",
                                       "D-FACILITY",     "Reserved",
                                       "Reserved",       "Reserved",
                                       "Reserved",       "Reserved",
                                       "Reserved",       "Reserved",
                                       "Reserved",       "Reserved",
                                       "Reserved",       "Reserved",
                                       "Reserved",       "Reserved",
                                       "Reserved",       "CMCE FUNCTION NOT SUPPORTED"};
    std::string cmce_uplink_pdu[] = {"U-ALERT",      "Reserved",    "U-CONNECT",      "Reserved",
                                     "U-DISCONNECT", "U-INFO",      "U-RELEASE",      "U-SETUP",
                                     "U-STATUS",     "U-TX CEASED", "U-TX DEMAND",    "Reserved",
                                     "Reserved",     "Reserved",    "U-CALL-RESTORE", "U-SDS-DATA",
                                     "U-FACILITY",   "Reserved",    "Reserved",       "Reserved",
                                     "Reserved",     "Reserved",    "Reserved",       "Reserved",
                                     "Reserved",     "Reserved",    "Reserved",       "Reserved",
                                     "Reserved",     "Reserved",    "Reserved",       "CMCE FUNCTION NOT SUPPORTED"};

    auto pdu_type = vec.take<5>();

    if (is_downlink) {
        std::cout << "CMCE " << cmce_downlink_pdu[pdu_type] << std::endl;
        switch (pdu_type) {
        case 0b01111:
            process_d_sds_data(address, vec);
            break;
        default:
            break;
        }
    } else {
        std::cout << "CMCE " << cmce_uplink_pdu[pdu_type] << std::endl;
        switch (pdu_type) {
        case 0b01111:
            process_u_sds_data(address, vec);
            break;
        default:
            break;
        }
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

    if (short_data_type_identifier == 3) {
        auto length_identifier = vec.take<11>();
        std::cout << "  length_identifier: " << static_cast<unsigned>(length_identifier) << std::endl;
        /// XXX: we do not account for the length identifier. External subscriber number or DM-MS address could be
        /// present here. The problem is that the fragmentation is somehow broken, we do not know the reason yet.
        // auto sds = BitVector(vec.take_vector(length_identifier), length_identifier);
        std::cout << "  BitsLeft = " << vec.bits_left() << " " << vec << std::endl;
        // sds_->process(to_address, from_address, sds);
        sds_.process(to_address, from_address, vec);
    } else {
        // XXX: we should take the length_identifier into account...
        sds_.process(to_address, from_address, vec);
    }
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

    if (short_data_type_identifier == 3) {
        auto length_identifier = vec.take<11>();
        std::cout << "  length_identifier: " << static_cast<unsigned>(length_identifier) << std::endl;
        /// XXX: we do not account for the length identifier. External subscriber number or DM-MS address could be
        /// present here. The problem is that the fragmentation is somehow broken, we do not know the reason yet.
        // auto sds = BitVector(vec.take_vector(length_identifier), length_identifier);
        std::cout << "  BitsLeft = " << vec.bits_left() << " " << vec << std::endl;
        // sds_->process(to_address, from_address, sds);
        sds_.process(to_address, from_address, vec);
    } else {
        // XXX: we should take the length_identifier into account...
        sds_.process(to_address, from_address, vec);
    }
}
