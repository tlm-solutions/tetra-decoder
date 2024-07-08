/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "l3/circuit_mode_control_entity_packet.hpp"

auto operator<<(std::ostream& stream, const SdsData& sds) -> std::ostream& {
    if (sds.area_selection_) {
        stream << "  Area Selction: " << std::bitset<4>(*sds.area_selection_) << std::endl;
    }
    stream << "  SDS Address: " << sds.address_ << std::endl;
    stream << "  Data: " << sds.data_ << std::endl;
    for (const auto& [key, value] : sds.optional_elements_) {
        stream << "  " << to_string(key) << " " << value << std::endl;
    }
    return stream;
};

auto operator<<(std::ostream& stream, const CircuitModeControlEntityPacket& cmce) -> std::ostream& {
    stream << "  CMCE: " << to_string(cmce.packet_type_) << std::endl;
    if (cmce.sds_data_) {
        stream << *cmce.sds_data_;
    }
    return stream;
};