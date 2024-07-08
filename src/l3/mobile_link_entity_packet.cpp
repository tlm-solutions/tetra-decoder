/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "l3/mobile_link_entity_packet.hpp"

MobileLinkEntityPacket::MobileLinkEntityPacket(const LogicalLinkControlPacket& packet)
    : LogicalLinkControlPacket(packet) {
    sdu_ = BitVector(tl_sdu_);

    auto discriminator = sdu_.take<3>();
    mle_protocol_ = MobileLinkEntityProtocolDiscriminator(discriminator);

    // TODO: add the special handling for the MLE protocol
};