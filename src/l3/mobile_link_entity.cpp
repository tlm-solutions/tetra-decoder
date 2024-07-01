/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#include "l3/mobile_link_entity.hpp"
#include "l2/logical_link_control_packet.hpp"
#include "l3/mobile_link_entity_packet.hpp"
#include "utils/bit_vector.hpp"
#include <cassert>
#include <memory>

auto MobileLinkEntity::process(const LogicalLinkControlPacket& packet) -> std::unique_ptr<LogicalLinkControlPacket> {
    auto data = BitVector(packet.tl_sdu_);

    auto pdu_type = data.take<3>();
    const auto* pdu_name = to_string(MobileLinkEntityProtocolDiscriminator(pdu_type));

    if (metrics_ && pdu_type != kMleProtocol) {
        metrics_->increment(pdu_name);
    }

    if (pdu_type == kMleProtocol) {
        auto pdu_type = data.take<3>();
        const auto& pdu_name =
            (packet.is_downlink() ? downlink_mle_pdu_description_ : uplink_mle_pdu_description_).at(pdu_type);

        if (metrics_ && pdu_type != kExtendedPdu) {
            metrics_->increment(pdu_name);
        }

        if (pdu_type == kExtendedPdu) {
            auto pdu_type = data.take<4>();
            const auto& pdu_name =
                (packet.is_downlink() ? downlink_mle_pdu_extension_description_ : uplink_mle_pdu_extension_description_)
                    .at(pdu_type);

            if (metrics_) {
                metrics_->increment(pdu_name);
            }
        }
    }

    return packet_builder_.parse_logical_link_control(packet);
}