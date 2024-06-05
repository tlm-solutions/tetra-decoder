#include <bitset>
#include <cassert>
#include <iostream>

#include <l3/mobile_management.hpp>

void MobileManagement::process(bool is_downlink, const AddressType address, BitVector& vec) {
    std::string mm_downlink_pdu[] = {"D-OTAR",
                                     "D-AUTHENTICATION",
                                     "D-CK CHANGE DEMAND",
                                     "D-DISABLE",
                                     "D-ENABLE",
                                     "D-LOCATION UPDATE ACCEPT",
                                     "D-LOCATION UPDATE COMMAND",
                                     "D-LOCATION UPDATE REJECT",
                                     "Reserved",
                                     "D-LOCATION UPDATE PROCEEDING",
                                     "D-LOCATION UPDATE PROCEEDING",
                                     "D-ATTACH/DETACH GROUP IDENTITY ACK",
                                     "D-MM STATUS",
                                     "Reserved",
                                     "Reserved",
                                     "MM PDU/FUNCTION NOT SUPPORTED"};
    std::string mm_uplink_pdu[] = {"U-AUTHENTICATION",
                                   "U-ITSI DETACH",
                                   "U-LOCATION UPDATE DEMAND",
                                   "U-MM STATUS",
                                   "U-CK CHANGE RESULT",
                                   "U-OTAR",
                                   "U-INFORMATION PROVIDE",
                                   "U-ATTACH/DETACH GROUP IDENTITY",
                                   "U-ATTACH/DETACH GROUP IDENTITY ACK",
                                   "U-TEI PROVIDE",
                                   "Reserved",
                                   "U-DISABLE STATUS",
                                   "Reserved",
                                   "Reserved",
                                   "Reserved",
                                   "MM PDU/FUNCTION NOT SUPPORTED"};

    auto pdu_type = vec.take<4>();

    if (is_downlink) {
        std::cout << "MM " << mm_downlink_pdu[pdu_type] << std::endl;
        switch (pdu_type) {
        default:
            break;
        }
    } else {
        std::cout << "MM " << mm_uplink_pdu[pdu_type] << std::endl;
        switch (pdu_type) {
        default:
            break;
        }
    }
}
