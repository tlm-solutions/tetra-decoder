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

enum class PacketType {
    kLogicalLinkControlPacket,
    kMobileLinkEntityPacket,
    kCircuitModeControlEntityPacket,
    kMobileManagementPacket,
    kShortDataServicePacket,
};

namespace nlohmann {
template <> struct adl_serializer<std::unique_ptr<LogicalLinkControlPacket>> {
    static void to_json(json& j, const std::unique_ptr<LogicalLinkControlPacket>& packet) {
        j["protocol_version"] = kPacketApiVersion;

        if (auto* mle = dynamic_cast<MobileLinkEntityPacket*>(packet.get())) {
            if (auto* cmce = dynamic_cast<CircuitModeControlEntityPacket*>(mle)) {
                if (auto* sds = dynamic_cast<ShortDataServicePacket*>(mle)) {
                    // Emit ShortDataServicePacket packet to json
                    j["key"] = PacketType::kShortDataServicePacket;
                    j["value"] = *sds;
                } else {
                    // Emit CircuitModeControlEntityPacket packet to json
                    j["key"] = PacketType::kCircuitModeControlEntityPacket;
                    j["value"] = *cmce;
                }
            } else if (auto* mm = dynamic_cast<MobileManagementPacket*>(mle)) {
                // Emit MobileManagementPacket packet to json
                j["key"] = PacketType::kMobileManagementPacket;
                j["value"] = *mm;
            } else {
                // Emit MobileLinkEntityPacket packet to json
                j["key"] = PacketType::kMobileLinkEntityPacket;
                j["value"] = *mle;
            }
        } else {
            // Emit LogicalLinkControlPacket packet to json
            j["key"] = PacketType::kLogicalLinkControlPacket;
            j["value"] = *packet;
        }
    }

    static void from_json(const json& j, std::unique_ptr<LogicalLinkControlPacket>& packet) {
        auto protocol_version = std::stoi(j["protocol_version"].template get<std::string>());
        if (protocol_version != kPacketApiVersion) {
            throw std::runtime_error("Cannot process packets different API version.");
        }

        auto key = PacketType(std::stoi(j["key"].template get<std::string>()));

        switch (key) {
        case PacketType::kLogicalLinkControlPacket:
            packet = std::make_unique<LogicalLinkControlPacket>(j["value"].template get<LogicalLinkControlPacket>());
            break;
        case PacketType::kMobileLinkEntityPacket:
            packet = std::make_unique<MobileLinkEntityPacket>(j["value"].template get<MobileLinkEntityPacket>());
            break;
        case PacketType::kCircuitModeControlEntityPacket:
            packet = std::make_unique<CircuitModeControlEntityPacket>(
                j["value"].template get<CircuitModeControlEntityPacket>());
            break;
        case PacketType::kMobileManagementPacket:
            packet = std::make_unique<MobileManagementPacket>(j["value"].template get<MobileManagementPacket>());
            break;
        case PacketType::kShortDataServicePacket:
            packet = std::make_unique<ShortDataServicePacket>(j["value"].template get<ShortDataServicePacket>());
            break;
        }
    }
};
} // namespace nlohmann
