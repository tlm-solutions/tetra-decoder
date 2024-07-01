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
#include "utils/packet_counter_metrics.hpp"
#include <array>
#include <string>

class MobileManagement {
  public:
    MobileManagement() = delete;
    explicit MobileManagement(const std::shared_ptr<PrometheusExporter>& prometheus_exporter) {
        downlink_mm_pdu_description_ = {"D-OTAR",
                                        "D-AUTHENTICATION",
                                        "D-CK CHANGE DEMAND",
                                        "D-DISABLE",
                                        "D-ENABLE",
                                        "D-LOCATION UPDATE ACCEPT",
                                        "D-LOCATION UPDATE COMMAND",
                                        "D-LOCATION UPDATE REJECT",
                                        "D-Reserved8",
                                        "D-LOCATION UPDATE PROCEEDING",
                                        "D-LOCATION UPDATE PROCEEDING",
                                        "D-ATTACH/DETACH GROUP IDENTITY ACK",
                                        "D-MM STATUS",
                                        "D-Reserved13",
                                        "D-Reserved14",
                                        "D-MM PDU/FUNCTION NOT SUPPORTED"};
        uplink_mm_pdu_description_ = {"U-AUTHENTICATION",
                                      "U-ITSI DETACH",
                                      "U-LOCATION UPDATE DEMAND",
                                      "U-MM STATUS",
                                      "U-CK CHANGE RESULT",
                                      "U-OTAR",
                                      "U-INFORMATION PROVIDE",
                                      "U-ATTACH/DETACH GROUP IDENTITY",
                                      "U-ATTACH/DETACH GROUP IDENTITY ACK",
                                      "U-TEI PROVIDE",
                                      "U-Reserved10",
                                      "U-DISABLE STATUS",
                                      "U-Reserved12",
                                      "U-Reserved13",
                                      "U-Reserved14",
                                      "U-MM PDU/FUNCTION NOT SUPPORTED"};
        if (prometheus_exporter) {
            metrics_ = std::make_unique<PacketCounterMetrics>(prometheus_exporter, "mobile_management");
        }
    }
    ~MobileManagement() noexcept = default;

    auto process(const MobileLinkEntityPacket& packet) -> std::unique_ptr<MobileManagementPacket>;

  private:
    std::array<std::string, 16> downlink_mm_pdu_description_;
    std::array<std::string, 16> uplink_mm_pdu_description_;

    std::unique_ptr<PacketCounterMetrics> metrics_;
};
