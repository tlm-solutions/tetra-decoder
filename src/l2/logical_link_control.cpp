#include <bitset>
#include <cassert>

#include <l2/logical_link_control.hpp>

auto LogicalLinkControl::check_fcs(BitVector& vec) -> bool {
    // remove last 32 bits
    auto fcs = vec.take_last(32);

    auto computed_fcs = vec.compute_fcs();

    if (fcs != computed_fcs) {
        std::cout << "  FCS error" << std::endl;
        std::cout << "    FCS           0b" << std::bitset<32>(fcs) << std::endl;
        std::cout << "    computed FCS: 0b" << std::bitset<32>(computed_fcs) << std::endl;
        return false;
    } else {
        std::cout << "  FCS good" << std::endl;
        return true;
    }
}

void LogicalLinkControl::process(const AddressType address, BitVector& vec) {
    std::cout << "LLC received: " << std::endl;
    std::cout << "  Address: " << address << std::endl;
    std::cout << "  Data: " << vec << std::endl;

    std::string llc_pdu[] = {"BL-ADATA without FCS",
                             "BL-DATA without FCS",
                             "BL-UDATA without FCS",
                             "BL-ACK without FCS",
                             "BL-ADATA with FCS",
                             "BL-DATA with FCS",
                             "BL-UDATA with FCS",
                             "BL-ACK with FCS",
                             "AL-SETUP",
                             "AL-DATA/AL-DATA-AR/AL-FINAL/AL-FINAL-AR",
                             "AL-UDATA/AL-UFINAL",
                             "AL-ACK/AL-RNR",
                             "AL-RECONNECT",
                             "Supplementary LLC PDU",
                             "Layer 2 signalling PDU",
                             "AL-DISC"};

    auto pdu_type = vec.take(4);

    std::cout << "  " << llc_pdu[pdu_type] << std::endl;

    switch (pdu_type) {
    case 0b0000:
        // BL-ADATA without FCS
        process_bl_adata_without_fcs(address, vec);
        break;
    case 0b0001:
        // BL-DATA without FCS
        process_bl_data_without_fcs(address, vec);
        break;
    case 0b0010:
        // BL-UDATA without FCS
        process_bl_udata_without_fcs(address, vec);
        break;
    case 0b0011:
        // BL-ACK without FCS
        process_bl_ack_without_fcs(address, vec);
        break;
    case 0b0100:
        // BL-ADATA with FCS
        process_bl_adata_with_fcs(address, vec);
        break;
    case 0b0101:
        // BL-DATA with FCS
        process_bl_data_with_fcs(address, vec);
        break;
    case 0b0110:
        // BL-UDATA with FCS
        process_bl_udata_with_fcs(address, vec);
        break;
    case 0b0111:
        // BL-ACK with FCS
        process_bl_ack_with_fcs(address, vec);
        break;
    case 0b1101:
        process_supplementary_llc_pdu(address, vec);
    default:
        break;
    }
}

void LogicalLinkControl::process_bl_adata_without_fcs(const AddressType address, BitVector& vec) {
    auto n_r = vec.take(1);
    auto n_s = vec.take(1);

    std::cout << "  N(R) = 0b" << std::bitset<1>(n_r) << std::endl;
    std::cout << "  N(S) = 0b" << std::bitset<1>(n_s) << std::endl;

    mle_->service_user_pdu(address, vec);
}

void LogicalLinkControl::process_bl_data_without_fcs(const AddressType address, BitVector& vec) {
    auto n_s = vec.take(1);
    std::cout << "  N(S) = 0b" << std::bitset<1>(n_s) << std::endl;

    mle_->service_user_pdu(address, vec);
}

void LogicalLinkControl::process_bl_udata_without_fcs(const AddressType address, BitVector& vec) {
    mle_->service_user_pdu(address, vec);
}

void LogicalLinkControl::process_bl_ack_without_fcs(const AddressType address, BitVector& vec) {
    auto n_r = vec.take(1);
    std::cout << "  N(R) = 0b" << std::bitset<1>(n_r) << std::endl;

    mle_->service_user_pdu(address, vec);
}

void LogicalLinkControl::process_bl_adata_with_fcs(const AddressType address, BitVector& vec) {
    auto n_r = vec.take(1);
    auto n_s = vec.take(1);

    std::cout << "  N(R) = 0b" << std::bitset<1>(n_r) << std::endl;
    std::cout << "  N(S) = 0b" << std::bitset<1>(n_s) << std::endl;

    if (LogicalLinkControl::check_fcs(vec)) {
        mle_->service_user_pdu(address, vec);
    }
}

void LogicalLinkControl::process_bl_data_with_fcs(const AddressType address, BitVector& vec) {
    auto n_s = vec.take(1);
    std::cout << "  N(S) = 0b" << std::bitset<1>(n_s) << std::endl;

    if (LogicalLinkControl::check_fcs(vec)) {
        mle_->service_user_pdu(address, vec);
    }
}

void LogicalLinkControl::process_bl_udata_with_fcs(const AddressType address, BitVector& vec) {
    if (LogicalLinkControl::check_fcs(vec)) {
        mle_->service_user_pdu(address, vec);
    }
}

void LogicalLinkControl::process_bl_ack_with_fcs(const AddressType address, BitVector& vec) {
    auto n_r = vec.take(1);
    std::cout << "  N(R) = 0b" << std::bitset<1>(n_r) << std::endl;

    if (LogicalLinkControl::check_fcs(vec)) {
        mle_->service_user_pdu(address, vec);
    }
}

void LogicalLinkControl::process_supplementary_llc_pdu(const AddressType address, BitVector& vec) {
    std::string supplementary_llc_pdu[] = {"AL-X-DATA/AL-X-DATA-AR/AL-X-FINAL/AL-X-FINAL-AR", "AL-X-UDATA/AL-X-UFINAL",
                                           "AL-X-UDATA/AL-X-UFINAL", "AL-X-ACK/AL-X-RNR", "Reserved"};

    auto pdu_type = vec.take(2);

    std::cout << "  " << supplementary_llc_pdu[pdu_type] << std::endl;
    std::cout << "  Data: " << vec << std::endl;

    switch (pdu_type) {
    case 0b00:
        break;
    case 0b01:
        break;
    case 0b10:
        break;
    case 0b11:
        break;
    default:
        break;
    }
}

auto operator<<(std::ostream& stream, const LogicalLinkControl& llc) -> std::ostream& { return stream; }
