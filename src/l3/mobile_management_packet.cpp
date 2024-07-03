/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "l3/mobile_management_packet.hpp"

MobileManagementPacket::MobileManagementPacket(const MobileLinkEntityPacket& packet)
    : MobileLinkEntityPacket(packet) {
    auto data = BitVector(sdu_);
    auto pdu_type = data.take<4>();
    if (is_downlink()) {
        packet_type_ = MobileManagementDownlinkPacketType(pdu_type);
    } else {
        packet_type_ = MobileManagementUplinkPacketType(pdu_type);
    }
};