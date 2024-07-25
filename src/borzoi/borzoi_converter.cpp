/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "borzoi/borzoi_converter.hpp"
#include "l3/circuit_mode_control_entity_packet.hpp"
#include "l3/mobile_link_entity_packet.hpp"
#include "l3/mobile_management_packet.hpp"
#include "l3/short_data_service_packet.hpp"

inline static auto get_time() -> std::string {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::stringstream ss;
    ss << std::put_time(&tm, "%FT%T%z");
    return ss.str();
}

auto BorzoiConverter::to_json(const std::unique_ptr<LogicalLinkControlPacket>& packet) -> nlohmann::json {
    nlohmann::json data = nlohmann::json::object();

    data["protocol_version"] = BorzoiConverter::kPacketApiVersion;
    data["time"] = get_time();

    if (auto* mle = dynamic_cast<MobileLinkEntityPacket*>(packet.get())) {
        if (auto* cmce = dynamic_cast<CircuitModeControlEntityPacket*>(mle)) {
            if (auto* sds = dynamic_cast<ShortDataServicePacket*>(mle)) {
                // Emit ShortDataServicePacket packet to json
                data["key"] = "ShortDataServicePacket";
                data["value"] = *sds;
            } else {
                // Emit CircuitModeControlEntityPacket packet to json
                data["key"] = "CircuitModeControlEntityPacket";
                data["value"] = *cmce;
            }
        } else if (auto* mm = dynamic_cast<MobileManagementPacket*>(mle)) {
            // Emit MobileManagementPacket packet to json
            data["key"] = "MobileManagementPacket";
            data["value"] = *mm;
        } else {
            // Emit MobileLinkEntityPacket packet to json
            data["key"] = "MobileLinkEntityPacket";
            data["value"] = *mle;
        }
    } else {
        // Emit LogicalLinkControlPacket packet to json
        data["key"] = "LogicalLinkControlPacket";
        data["value"] = *packet;
    }

    return data;
}

auto BorzoiConverter::to_json(const Slots& slots) -> nlohmann::json {
    auto message = nlohmann::json::object();

    message["time"] = get_time();
    message["burst_type"] = static_cast<unsigned>(slots.get_burst_type());
    /// This may but should not throw.
    auto first_slot = slots.get_first_slot().get_logical_channel_data_and_crc();
    message["first_slot_logical_channel"] = static_cast<unsigned>(first_slot.channel);
    message["first_slot_data"] = nlohmann::json::array();
    while (first_slot.data.bits_left()) {
        unsigned bit = first_slot.data.take<1>();
        message["first_slot_logical_data"].push_back(bit);
    }
    message["first_slot_crc_ok"] = first_slot.crc_ok;
    auto second_slot_present = slots.has_second_slot();
    message["second_slot_present"] = second_slot_present;
    if (second_slot_present) {
        auto second_slot = slots.get_second_slot().get_logical_channel_data_and_crc();
        message["second_slot_logical_channel"] = static_cast<unsigned>(first_slot.channel);
        message["second_slot_data"] = nlohmann::json::array();
        while (first_slot.data.bits_left()) {
            unsigned bit = first_slot.data.take<1>();
            message["second_slot_data"].push_back(bit);
        }
        message["second_slot_crc_ok"] = first_slot.crc_ok;
    }
    return message;
}