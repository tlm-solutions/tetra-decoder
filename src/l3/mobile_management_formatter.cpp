/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "l3/mobile_management_packet.hpp"

auto operator<<(std::ostream& stream, const MobileManagementDownlinkAttachDetachGroupIdentityAcknowledgement& packet)
    -> std::ostream& {
    stream << "D-ATTACH/DETACH GROUP IDENTITY ACKNOWLEDGEMENT: " << to_string(packet.group_identity_accept_reject_)
           << std::endl;
    for (const auto& [key, value] : packet.optional_elements_) {
        stream << "  " << to_string(key) << " " << value << std::endl;
    }
    return stream;
}

auto operator<<(std::ostream& stream, const MobileManagementDownlinkLocationUpdateAccept& packet) -> std::ostream& {
    stream << "D-LOCATION UPDATE ACCEPT: " << to_string(packet.location_update_accept_type_) << std::endl;
    stream << "  Address: " << packet.address_ << std::endl;
    if (packet.subscriber_class_) {
        stream << "  subscriber class: " << std::bitset<16>(*packet.subscriber_class_) << std::endl;
    }
    if (packet.energy_saving_information_) {
        stream << "  energy saving information: " << std::bitset<14>(*packet.energy_saving_information_) << std::endl;
    }
    if (packet.scch_information_) {
        stream << "  scch information: " << std::bitset<4>(*packet.scch_information_) << std::endl;
    }
    if (packet.distribution_on_18th_frame_) {
        stream << "  distribution on 18th frame: " << std::bitset<2>(*packet.distribution_on_18th_frame_) << std::endl;
    }
    for (const auto& [key, value] : packet.optional_elements_) {
        stream << "  " << to_string(key) << " " << value << std::endl;
    }
    return stream;
}

auto operator<<(std::ostream& stream, const MobileManagementPacket& packet) -> std::ostream& {
    stream << "MM " << to_string(packet.packet_type_) << std::endl;
    if (packet.downlink_location_update_accept_) {
        stream << *packet.downlink_location_update_accept_;
    }
    if (packet.downlink_attach_detach_group_identity_acknowledgement_) {
        stream << *packet.downlink_attach_detach_group_identity_acknowledgement_;
    }
    return stream;
}