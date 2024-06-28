/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "l2/logical_link_control.hpp"
#include "l2/logical_link_control_packet_builder.hpp"
#include "l2/upper_mac_packet.hpp"
#include <cassert>
#include <memory>

auto LogicalLinkControl::process(const UpperMacCPlaneSignallingPacket& packet)
    -> std::unique_ptr<UpperMacCPlaneSignallingPacket> {
    auto pdu_type = packet.tm_sdu_->look<4>(0);
    const auto& pdu_name = llc_pdu_description_.at(pdu_type);

    // Skip incrementing the metrics for Supplementary LLC PDU and Layer 2 signalling PDU
    if (metrics_ && (pdu_type != kSupplementaryLlcPdu) && (pdu_type != kLayer2SignallingPdu)) {
        metrics_->increment(pdu_name);
    }

    if (pdu_type == kSupplementaryLlcPdu) {
        auto pdu_type = packet.tm_sdu_->look<2>(4);
        const auto& pdu_name = supplementary_llc_pdu_description_.at(pdu_type);

        if (metrics_) {
            metrics_->increment(pdu_name);
        }
    }

    if (pdu_type == kLayer2SignallingPdu) {
        auto pdu_type = packet.tm_sdu_->look<4>(4);
        const auto& pdu_name = layer_2_signalling_pdu_description_.at(pdu_type);

        if (metrics_) {
            metrics_->increment(pdu_name);
        }
    }

    return packet_builder_.parse_c_plane_signalling(packet);
}