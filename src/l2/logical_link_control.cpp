#include <bitset>
#include <cassert>

#include <l2/logical_link_control.hpp>

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
        process_bl_adata_without_fcs(address, vec);
        break;
    case 0b0001:
        process_bl_data_without_fcs(address, vec);
        break;
    case 0b0010:
        // BL-UDATA without FCS
        mle_->service_user_pdu(address, vec);
        break;
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

auto operator<<(std::ostream& stream, const LogicalLinkControl& llc) -> std::ostream& { return stream; }
