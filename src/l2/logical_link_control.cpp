/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "l2/logical_link_control.hpp"
#include <bitset>
#include <cassert>

auto LogicalLinkControl::check_fcs(BitVector& vec) -> bool {
    // remove last 32 bits
    std::bitset<32> fcs = vec.take_last<32>();

    std::bitset<32> computed_fcs = vec.compute_fcs();

    if (fcs != computed_fcs) {
        std::cout << "  FCS error" << std::endl;
        std::cout << "    FCS           0b" << fcs << std::endl;
        std::cout << "    computed FCS: 0b" << computed_fcs << std::endl;
        return false;
    }

    std::cout << "  FCS good" << std::endl;
    return true;
}

void LogicalLinkControl::process(const Address address, BitVector& vec) {
    std::cout << "LLC received: " << std::endl;
    std::cout << "  Address: " << address << std::endl;
    std::cout << "  Data: " << vec << std::endl;

    auto pdu_type = vec.take<4>();
    const auto& pdu_name = llc_pdu_description_.at(pdu_type);

    // Skip incrementing the metrics for Supplementary LLC PDU and Layer 2 signalling PDU
    if (metrics_ && (pdu_type != 13) && (pdu_type != 14)) {
        metrics_->increment(pdu_name);
    }

    std::cout << "  " << pdu_name << std::endl;

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
        break;
    case 0b1110:
        process_layer_2_signalling_pdu(address, vec);
        break;
    default:
        break;
    }
}

void LogicalLinkControl::process_bl_adata_without_fcs(const Address address, BitVector& vec) {
    auto n_r = vec.take<1>();
    auto n_s = vec.take<1>();

    std::cout << "  N(R) = 0b" << std::bitset<1>(n_r) << std::endl;
    std::cout << "  N(S) = 0b" << std::bitset<1>(n_s) << std::endl;

    mle_.service_user_pdu(address, vec);
}

void LogicalLinkControl::process_bl_data_without_fcs(const Address address, BitVector& vec) {
    auto n_s = vec.take<1>();
    std::cout << "  N(S) = 0b" << std::bitset<1>(n_s) << std::endl;

    mle_.service_user_pdu(address, vec);
}

void LogicalLinkControl::process_bl_udata_without_fcs(const Address address, BitVector& vec) {
    mle_.service_user_pdu(address, vec);
}

void LogicalLinkControl::process_bl_ack_without_fcs(const Address address, BitVector& vec) {
    auto n_r = vec.take<1>();
    std::cout << "  N(R) = 0b" << std::bitset<1>(n_r) << std::endl;

    mle_.service_user_pdu(address, vec);
}

void LogicalLinkControl::process_bl_adata_with_fcs(const Address address, BitVector& vec) {
    auto n_r = vec.take<1>();
    auto n_s = vec.take<1>();

    std::cout << "  N(R) = 0b" << std::bitset<1>(n_r) << std::endl;
    std::cout << "  N(S) = 0b" << std::bitset<1>(n_s) << std::endl;

    if (LogicalLinkControl::check_fcs(vec)) {
        mle_.service_user_pdu(address, vec);
    }
}

void LogicalLinkControl::process_bl_data_with_fcs(const Address address, BitVector& vec) {
    auto n_s = vec.take<1>();
    std::cout << "  N(S) = 0b" << std::bitset<1>(n_s) << std::endl;

    if (LogicalLinkControl::check_fcs(vec)) {
        mle_.service_user_pdu(address, vec);
    }
}

void LogicalLinkControl::process_bl_udata_with_fcs(const Address address, BitVector& vec) {
    if (LogicalLinkControl::check_fcs(vec)) {
        mle_.service_user_pdu(address, vec);
    }
}

void LogicalLinkControl::process_bl_ack_with_fcs(const Address address, BitVector& vec) {
    auto n_r = vec.take<1>();
    std::cout << "  N(R) = 0b" << std::bitset<1>(n_r) << std::endl;

    if (LogicalLinkControl::check_fcs(vec)) {
        mle_.service_user_pdu(address, vec);
    }
}

void LogicalLinkControl::process_supplementary_llc_pdu(const Address address, BitVector& vec) {
    auto pdu_type = vec.take<2>();
    const auto& pdu_name = supplementary_llc_pdu_description_.at(pdu_type);

    std::cout << "  " << pdu_name << std::endl;
    std::cout << "  Data: " << vec << std::endl;

    if (metrics_) {
        metrics_->increment(pdu_name);
    }

    switch (pdu_type) {
    case 0b00:
        break;
    case 0b01:
        break;
    case 0b10:
        break;
    case 0b11:
        break;
    }
}

void LogicalLinkControl::process_layer_2_signalling_pdu(const Address address, BitVector& vec) {
    auto pdu_type = vec.take<4>();
    const auto& pdu_name = layer_2_signalling_pdu_description_.at(pdu_type);

    if (metrics_) {
        metrics_->increment(pdu_name);
    }
}

auto operator<<(std::ostream& stream, const LogicalLinkControl& llc) -> std::ostream& { return stream; }
