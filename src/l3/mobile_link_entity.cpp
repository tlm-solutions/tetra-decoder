#include <cassert>

#include <l3/mobile_link_entity.hpp>

void MobileLinkEntity::service_user_pdu(const Address address, BitVector& vec) {
    std::string mle_pdu[] = {"Reserved",
                             "MM protocol",
                             "CMCE protocol",
                             "Reserved",
                             "SNDCP protocol",
                             "MLE protocol",
                             "TETRA management entity protocol",
                             "Reserved for testing"};

    if (vec.bits_left() == 0) {
        return;
    }

    auto pdu_type = vec.take<3>();

    std::cout << "MLE " << mle_pdu[pdu_type] << " " << vec << std::endl;

    switch (pdu_type) {
    case 0b001:
        mm_->process(is_downlink_, address, vec);
        break;
    case 0b010:
        cmce_->process(is_downlink_, address, vec);
        break;
    case 0b101:
        service_data_pdu(address, vec);
        break;
    default:
        break;
    }
}

void MobileLinkEntity::service_data_pdu(const Address address, BitVector& vec) {

    std::string mle_downlink_pdu_type[] = {
        "D-NEW-CELL",    "D-PREPARE-FAIL", "D-NWRK-BROADCAST",   "D-NWRK-BROADCAST EXTENSION",
        "D-RESTORE-ACK", "D-RESTORE-FAIL", "D-CHANNEL RESPONSE", "Extended PDU"};
    std::string mle_uplink_pdu_type[] = {
        "U-PREPARE", "U-PREPARE-DA", "U-IRREGULAR CHANNEL ADVICE", "U-CHANNEL CLASS ADVICE",
        "U-RESTORE", "Reserved",     "U-CHANNEL REQUEST",          "Extended PDU"};

    std::string mle_downlink_pdu_type_extension[] = {"D-NWRK-BROADCAST-DA",
                                                     "D-NWRK-BROADCAST REMOVE",
                                                     "Reserved",
                                                     "Reserved",
                                                     "Reserved",
                                                     "Reserved",
                                                     "Reserved",
                                                     "Reserved",
                                                     "Reserved",
                                                     "Reserved",
                                                     "Reserved",
                                                     "Reserved",
                                                     "Reserved",
                                                     "Reserved",
                                                     "Reserved",
                                                     "Reserved"};
    std::string mle_uplink_pdu_type_extension[] = {
        "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
        "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    };

    if (vec.bits_left() == 0) {
        return;
    }

    auto pdu_type = vec.take<3>();

    std::cout << "  " << mle_uplink_pdu_type[pdu_type] << " or " << mle_downlink_pdu_type[pdu_type] << std::endl;
    std::cout << "  " << vec << std::endl;
}