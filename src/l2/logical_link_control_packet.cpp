/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "l2/logical_link_control_packet.hpp"

BasicLinkInformation::BasicLinkInformation(BitVector& data) {
    auto pdu_type = data.take<4>();

    switch (pdu_type) {
    case 0b0000:
        n_r_ = data.take<1>();
        n_s_ = data.take<1>();
        break;
    case 0b0001:
        n_s_ = data.take<1>();
        break;
    case 0b0010:
        break;
    case 0b0011:
        n_r_ = data.take<1>();
        break;
    case 0b0100:
        n_r_ = data.take<1>();
        n_s_ = data.take<1>();
        break;
    case 0b0101:
        n_s_ = data.take<1>();
        break;
    case 0b0110:
        break;
    case 0b0111:
        n_r_ = data.take<1>();
        break;
    default:
        throw std::runtime_error("Did not expect something other than basic link in LogicalLinkControlPacket parser!");
    }

    basic_link_type_ = BasicLinkType(pdu_type);

    if (pdu_type >= 0b0100) {
        auto fcs = data.take_last<32>();
        auto computed_fcs = data.compute_fcs();
        fcs_good_ = fcs == computed_fcs;
    }
}

LogicalLinkControlPacket::LogicalLinkControlPacket(const UpperMacCPlaneSignallingPacket& packet)
    : UpperMacCPlaneSignallingPacket(packet) {
    auto data = BitVector(*tm_sdu_);
    auto pdu_type = data.look<4>(0);

    /// We only implemented packet parsing for Basic Link PDUs at this point in time
    if (pdu_type <= 0b1000) {
        basic_link_information_ = BasicLinkInformation(data);
        tl_sdu_ = data;
    }
}