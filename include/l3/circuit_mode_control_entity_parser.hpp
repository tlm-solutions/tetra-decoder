/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l3/circuit_mode_control_entity_packet.hpp"
#include "l3/short_data_service_parser.hpp"

class CircuitModeControlEntityParser : public PacketParser<MobileLinkEntityPacket, CircuitModeControlEntityPacket> {
  public:
    CircuitModeControlEntityParser() = delete;
    explicit CircuitModeControlEntityParser(const std::shared_ptr<PrometheusExporter>& prometheus_exporter)
        : PacketParser(prometheus_exporter, "circuit_mode_control_entity")
        , sds_(prometheus_exporter){};

  private:
    auto packet_name(const CircuitModeControlEntityPacket& packet) -> std::string override {
        return to_string(packet.packet_type_);
    };

    auto forward(const CircuitModeControlEntityPacket& packet)
        -> std::unique_ptr<CircuitModeControlEntityPacket> override {
        if (packet.sds_data_) {
            return sds_.parse(packet);
        }

        return std::make_unique<CircuitModeControlEntityPacket>(packet);
    };

    ShortDataServiceParser sds_;
};