/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#pragma once

#include "l2/logical_link_control_packet.hpp"
#include "l3/mobile_link_entity_packet_builder.hpp"
#include "prometheus.h"
#include "utils/packet_counter_metrics.hpp"
#include <memory>

class MobileLinkEntity {
  public:
    MobileLinkEntity() = delete;
    MobileLinkEntity(const std::shared_ptr<PrometheusExporter>& prometheus_exporter)
        : packet_builder_(prometheus_exporter) {
        downlink_mle_pdu_description_ = {
            "D-NEW-CELL",    "D-PREPARE-FAIL", "D-NWRK-BROADCAST",   "D-NWRK-BROADCAST EXTENSION",
            "D-RESTORE-ACK", "D-RESTORE-FAIL", "D-CHANNEL RESPONSE", "Extended PDU"};
        downlink_mle_pdu_extension_description_ = {"D-NWRK-BROADCAST-DA", "D-NWRK-BROADCAST REMOVE",
                                                   "D-Reserved2",         "D-Reserved3",
                                                   "D-Reserved4",         "D-Reserved5",
                                                   "D-Reserved6",         "D-Reserved7",
                                                   "D-Reserved8",         "D-Reserved9",
                                                   "D-Reserved10",        "D-Reserved11",
                                                   "D-Reserved12",        "D-Reserved13",
                                                   "D-Reserved14",        "D-Reserved15"};
        uplink_mle_pdu_description_ = {
            "U-PREPARE", "U-PREPARE-DA", "U-IRREGULAR CHANNEL ADVICE", "U-CHANNEL CLASS ADVICE",
            "U-RESTORE", "Reserved",     "U-CHANNEL REQUEST",          "Extended PDU"};
        uplink_mle_pdu_extension_description_ = {"U-Reserved0",  "U-Reserved1",  "U-Reserved2",  "U-Reserved3",
                                                 "U-Reserved4",  "U-Reserved5",  "U-Reserved6",  "U-Reserved7",
                                                 "U-Reserved8",  "U-Reserved9",  "U-Reserved10", "U-Reserved11",
                                                 "U-Reserved12", "U-Reserved13", "U-Reserved14", "U-Reserved15"};

        if (prometheus_exporter) {
            metrics_ = std::make_unique<PacketCounterMetrics>(prometheus_exporter, "mobile_link_entity");
        }
    };
    ~MobileLinkEntity() noexcept = default;

    auto process(const LogicalLinkControlPacket& packet) -> std::unique_ptr<LogicalLinkControlPacket>;

  private:
    static const auto kMleProtocol = 5;
    static const auto kExtendedPdu = 7;

    std::array<std::string, 8> downlink_mle_pdu_description_;
    std::array<std::string, 16> downlink_mle_pdu_extension_description_;

    std::array<std::string, 8> uplink_mle_pdu_description_;
    std::array<std::string, 16> uplink_mle_pdu_extension_description_;

    MobileLinkEntityPacketBuilder packet_builder_;
    std::unique_ptr<PacketCounterMetrics> metrics_;
};