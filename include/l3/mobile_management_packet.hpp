/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l3/mobile_link_entity_packet.hpp"

struct MobileManagementPacket : public MobileLinkEntityPacket {
    MobileManagementPacket() = delete;

    explicit MobileManagementPacket(const MobileLinkEntityPacket& packet)
        : MobileLinkEntityPacket(packet){};
};