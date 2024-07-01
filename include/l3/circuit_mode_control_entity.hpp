/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l3/circuit_mode_control_entity_packet.hpp"
#include "l3/circuit_mode_control_entity_packet_builder.hpp"
#include "l3/mobile_link_entity_packet.hpp"
#include "utils/packet_counter_metrics.hpp"

class CircuitModeControlEntity {
  public:
    CircuitModeControlEntity() = delete;
    explicit CircuitModeControlEntity(const std::shared_ptr<PrometheusExporter>& prometheus_exporter)
        : packet_builder_(prometheus_exporter) {
        if (prometheus_exporter) {
            metrics_ = std::make_unique<PacketCounterMetrics>(prometheus_exporter, "circuit_mode_control_entity");
        }
    };
    ~CircuitModeControlEntity() noexcept = default;

    auto process(const MobileLinkEntityPacket& packet) -> std::unique_ptr<CircuitModeControlEntityPacket>;

  private:
    CircuitModeControlEntityPacketBuilder packet_builder_;
    std::unique_ptr<PacketCounterMetrics> metrics_;
};