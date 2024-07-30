/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l2/logical_link_control_packet.hpp"
#include "l3/circuit_mode_control_entity_packet.hpp"
#include "l3/mobile_link_entity_packet.hpp"
#include "l3/mobile_management_packet.hpp"
#include "l3/short_data_service_packet.hpp"
#include <nlohmann/json.hpp>

static constexpr const int kPacketApiVersion = 0;

namespace nlohmann {
template <> struct adl_serializer<std::unique_ptr<LogicalLinkControlPacket>> {
    static void to_json(json& j, const std::unique_ptr<LogicalLinkControlPacket>& packet) {
        j["protocol_version"] = kPacketApiVersion;

        if (auto* mle = dynamic_cast<MobileLinkEntityPacket*>(packet.get())) {
            if (auto* cmce = dynamic_cast<CircuitModeControlEntityPacket*>(mle)) {
                if (auto* sds = dynamic_cast<ShortDataServicePacket*>(mle)) {
                    // Emit ShortDataServicePacket packet to json
                    j["key"] = "ShortDataServicePacket";
                    j["value"] = *sds;
                } else {
                    // Emit CircuitModeControlEntityPacket packet to json
                    j["key"] = "CircuitModeControlEntityPacket";
                    j["value"] = *cmce;
                }
            } else if (auto* mm = dynamic_cast<MobileManagementPacket*>(mle)) {
                // Emit MobileManagementPacket packet to json
                j["key"] = "MobileManagementPacket";
                j["value"] = *mm;
            } else {
                // Emit MobileLinkEntityPacket packet to json
                j["key"] = "MobileLinkEntityPacket";
                j["value"] = *mle;
            }
        } else {
            // Emit LogicalLinkControlPacket packet to json
            j["key"] = "LogicalLinkControlPacket";
            j["value"] = *packet;
        }
    }

    static void from_json(const json& j, std::unique_ptr<LogicalLinkControlPacket>& packet) {
        auto protocol_version = j["protocol_version"].template get<int>();
        if (protocol_version != kPacketApiVersion) {
            throw std::runtime_error("Cannot process packets different API version.");
        }

        auto key = j["key"].template get<std::string>();

        if (key == "LogicalLinkControlPacket") {
            packet = std::make_unique<LogicalLinkControlPacket>(j["value"].template get<LogicalLinkControlPacket>());
        } else if (key == "MobileLinkEntityPacket") {
            packet = std::make_unique<LogicalLinkControlPacket>(j["value"].template get<MobileLinkEntityPacket>());
        } else if (key == "MobileManagementPacket") {
            packet = std::make_unique<LogicalLinkControlPacket>(j["value"].template get<MobileManagementPacket>());
        } else if (key == "CircuitModeControlEntityPacket") {
            packet =
                std::make_unique<LogicalLinkControlPacket>(j["value"].template get<CircuitModeControlEntityPacket>());
        } else if (key == "ShortDataServicePacket") {
            packet = std::make_unique<LogicalLinkControlPacket>(j["value"].template get<ShortDataServicePacket>());
        } else {
            throw std::runtime_error("Unknown packet type: " + key);
        }
    }
};
} // namespace nlohmann
