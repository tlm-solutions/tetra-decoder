/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l3/mobile_management_packet.hpp"
#include "utils/packet_parser.hpp"

class MobileManagementParser : public PacketParser<MobileLinkEntityPacket, MobileManagementPacket> {
  public:
    MobileManagementParser() = delete;
    explicit MobileManagementParser(const std::shared_ptr<PrometheusExporter>& prometheus_exporter)
        : PacketParser(prometheus_exporter, "mobile_management") {
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
    };

  private:
    auto packet_name(const MobileManagementPacket& packet) -> std::string override {
        auto pdu_type = packet.sdu_.look<4>(0);
        return (packet.is_downlink() ? downlink_mm_pdu_description_ : uplink_mm_pdu_description_).at(pdu_type);
    }

    auto forward(const MobileManagementPacket& packet) -> std::unique_ptr<MobileManagementPacket> override {
        return std::make_unique<MobileManagementPacket>(packet);
    };

    std::array<std::string, 16> downlink_mm_pdu_description_;
    std::array<std::string, 16> uplink_mm_pdu_description_;
};