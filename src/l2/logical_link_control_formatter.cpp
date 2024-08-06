/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "l2/logical_link_control_packet.hpp"
#include <bitset>

auto operator<<(std::ostream& stream, const BasicLinkInformation& bli) -> std::ostream& {
    stream << "  Basic Link Information: " << to_string(bli.basic_link_type_);
    if (bli.n_r_) {
        stream << " N(R): " << std::bitset<1>(*bli.n_r_);
    }
    if (bli.n_s_) {
        stream << " N(S): " << std::bitset<1>(*bli.n_s_);
    }
    if (bli.fcs_good_) {
        stream << " FCS good: " << (*bli.fcs_good_ ? "true" : "false");
    }
    stream << std::endl;
    return stream;
}

auto operator<<(std::ostream& stream, const LogicalLinkControlPacket& llc) -> std::ostream& {
    if (llc.basic_link_information_) {
        stream << *llc.basic_link_information_;
    }
    stream << "  TL-SDU: " << llc.tl_sdu_ << std::endl;
    return stream;
}