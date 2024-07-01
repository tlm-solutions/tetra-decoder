/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "l3/mobile_management.hpp"
#include "l3/mobile_management_packet.hpp"
#include <memory>

auto MobileManagement::process(const MobileLinkEntityPacket& packet) -> std::unique_ptr<MobileManagementPacket> {
    if (metrics_) {
        auto pdu_type = packet.sdu_.look<4>(0);
        const auto& pdu_name =
            (packet.is_downlink() ? downlink_mm_pdu_description_ : uplink_mm_pdu_description_).at(pdu_type);
        metrics_->increment(pdu_name);
    }

    return std::make_unique<MobileManagementPacket>(packet);
}