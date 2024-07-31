/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l3/mobile_link_entity_packet.hpp"
#include "l3/mobile_management_packet.hpp"
#include "l3/short_data_service_packet.hpp"

inline auto operator<<(std::ostream& stream, const std::unique_ptr<LogicalLinkControlPacket>& packet) -> std::ostream& {
    /// process the parsed packet
    auto* cplane_signalling = dynamic_cast<UpperMacCPlaneSignallingPacket*>(packet.get());
    auto* llc = dynamic_cast<LogicalLinkControlPacket*>(cplane_signalling);

    stream << *cplane_signalling;
    stream << *llc;
    if (auto* mle = dynamic_cast<MobileLinkEntityPacket*>(llc)) {
        stream << *mle;
        if (auto* cmce = dynamic_cast<CircuitModeControlEntityPacket*>(llc)) {
            stream << *cmce;
            if (auto* sds = dynamic_cast<ShortDataServicePacket*>(llc)) {
                stream << *sds;
            }
        }
        if (auto* mm = dynamic_cast<MobileManagementPacket*>(llc)) {
            stream << *mm;
        }
    }

    return stream;
}