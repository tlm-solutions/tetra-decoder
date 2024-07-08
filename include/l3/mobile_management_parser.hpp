/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l3/mobile_link_entity_packet.hpp"
#include "l3/mobile_management_packet.hpp"
#include "utils/packet_parser.hpp"

class MobileManagementParser : public PacketParser<MobileLinkEntityPacket, MobileManagementPacket> {
  public:
    MobileManagementParser() = delete;
    explicit MobileManagementParser(const std::shared_ptr<PrometheusExporter>& prometheus_exporter)
        : PacketParser(prometheus_exporter, "mobile_management"){};

  private:
    [[nodiscard]] auto packet_name(const MobileManagementPacket& packet) const -> std::string override {
        return to_string(packet.packet_type_);
    }

    auto forward(const MobileManagementPacket& packet) -> std::unique_ptr<MobileManagementPacket> override {
        return std::make_unique<MobileManagementPacket>(packet);
    };
};