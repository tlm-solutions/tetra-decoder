/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l3/circuit_mode_control_entity_packet.hpp"
#include "l3/short_data_service.hpp"
#include <memory>

class CircuitModeControlEntityPacketBuilder {
  private:
    ShortDataService sds_;

  public:
    CircuitModeControlEntityPacketBuilder() = delete;

    explicit CircuitModeControlEntityPacketBuilder(const std::shared_ptr<PrometheusExporter>& prometheus_exporter)
        : sds_(prometheus_exporter){};

    [[nodiscard]] auto parse_mobile_link_entity(const MobileLinkEntityPacket& packet)
        -> std::unique_ptr<CircuitModeControlEntityPacket> {
        auto cmce_packet = CircuitModeControlEntityPacket(packet);
        if (cmce_packet.sds_data_) {
            return sds_.process(cmce_packet);
        }

        return std::make_unique<CircuitModeControlEntityPacket>(cmce_packet);
    };
};