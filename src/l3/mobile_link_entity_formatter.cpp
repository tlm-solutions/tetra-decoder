/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "l3/mobile_link_entity_packet.hpp"

auto operator<<(std::ostream& stream, const MobileLinkEntityPacket& mle) -> std::ostream& {
    stream << "  MLE: " << to_string(mle.mle_protocol_) << std::endl;
    stream << "  SDU: " << mle.sdu_ << std::endl;
    return stream;
}