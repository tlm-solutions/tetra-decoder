#include <bitset>
#include <cassert>
#include <iostream>

#include <l3/circuit_mode_control_entity.hpp>

void CircuitModeControlEntity::process(bool is_downlink, BitVector& vec) {
    char* cmce_downlink_pdu[] = {"D-ALERT",        "D-CALL-PROCEEDING",
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
    char* cmce_uplink_pdu[] = {"D-ALERT",      "Reserved",    "D-CONNECT",      "Reserved",
                               "D-DISCONNECT", "D-INFO",      "D-RELEASE",      "D-SETUP",
                               "D-STATUS",     "D-TX CEASED", "D-TX DEMAND",    "Reserved",
                               "Reserved",     "Reserved",    "D-CALL-RESTORE", "D-SDS-DATA",
                               "D-FACILITY",   "Reserved",    "Reserved",       "Reserved",
                               "Reserved",     "Reserved",    "Reserved",       "Reserved",
                               "Reserved",     "Reserved",    "Reserved",       "Reserved",
                               "Reserved",     "Reserved",    "Reserved",       "CMCE FUNCTION NOT SUPPORTED"};

    auto pdu_type = vec.take(5);

    if (is_downlink) {
        std::cout << "CMCE " << cmce_downlink_pdu[pdu_type] << std::endl;
        switch (pdu_type) {
        case 0b01111:
            process_d_sds_data(vec);
            break;
        default:
            break;
        }
    } else {
        std::cout << "CMCE " << cmce_uplink_pdu[pdu_type] << std::endl;
    }
}

void CircuitModeControlEntity::process_d_sds_data(BitVector& vec) {
    auto calling_party_type_identifier = vec.take(2);
    if (calling_party_type_identifier == 1 || calling_party_type_identifier == 2) {
        auto ssi = vec.take(24);
        std::cout << "  SSI: " << ssi << std::endl;
    }
    if (calling_party_type_identifier == 2) {
        auto calling_party_extension = vec.take(24);
        std::cout << "  Calling Party Extension: " << calling_party_extension << std::endl;
    }
    auto short_data_type_identifier = vec.take(2);
    std::cout << "  short_data_type_identifier: " << short_data_type_identifier << std::endl;
    if (short_data_type_identifier == 3) {
        auto length_identifier = vec.take(11);
        std::cout << "  length_identifier: " << length_identifier << std::endl;
        std::cout << "  BitsLeft = " << vec.bits_left() << " " << vec << std::endl;
    }

    // TODO: pass to l3/short_data_service
}
