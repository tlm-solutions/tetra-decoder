/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#pragma once

#include "l3/circuit_mode_control_entity_parser.hpp"
#include "l3/mobile_link_entity_packet.hpp"
#include "l3/mobile_management_parser.hpp"

class MobileLinkEntityParser : public PacketParser<LogicalLinkControlPacket, MobileLinkEntityPacket> {
  public:
    MobileLinkEntityParser() = delete;
    explicit MobileLinkEntityParser(const std::shared_ptr<PrometheusExporter>& prometheus_exporter)
        : PacketParser(prometheus_exporter, "mobile_link_entity")
        , cmce_(prometheus_exporter)
        , mm_(prometheus_exporter) {
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
    };

  private:
    auto packet_name(const MobileLinkEntityPacket& packet) -> std::string override {
        if (packet.mle_protocol_ == MobileLinkEntityProtocolDiscriminator::kMleProtocol) {
            auto pdu_type = packet.sdu_.look<3>(0);

            if (pdu_type == kExtendedPdu) {
                auto pdu_type = packet.sdu_.look<4>(3);
                return (packet.is_downlink() ? downlink_mle_pdu_extension_description_
                                             : uplink_mle_pdu_extension_description_)
                    .at(pdu_type);
            }

            return (packet.is_downlink() ? downlink_mle_pdu_description_ : uplink_mle_pdu_description_).at(pdu_type);
        }

        return to_string(packet.mle_protocol_);
    };

    auto forward(const MobileLinkEntityPacket& packet) -> std::unique_ptr<MobileLinkEntityPacket> override {
        // TODO: currently we only handle CMCE and MM
        switch (packet.mle_protocol_) {
        case MobileLinkEntityProtocolDiscriminator::kMmProtocol:
            return mm_.parse(packet);
        case MobileLinkEntityProtocolDiscriminator::kCmceProtocol:
            return cmce_.parse(packet);

        // Fall through for all other unimplemented packet types
        case MobileLinkEntityProtocolDiscriminator::kReserved0:
        case MobileLinkEntityProtocolDiscriminator::kReserved3:
        case MobileLinkEntityProtocolDiscriminator::kSndcpProtocol:
        case MobileLinkEntityProtocolDiscriminator::kMleProtocol:
        case MobileLinkEntityProtocolDiscriminator::kTetraManagementEntityProtocol:
        case MobileLinkEntityProtocolDiscriminator::kReservedForTesting:
            return std::make_unique<MobileLinkEntityPacket>(packet);
        }
    };

    CircuitModeControlEntityParser cmce_;
    MobileManagementParser mm_;

    static const auto kExtendedPdu = 7;

    std::array<std::string, 8> downlink_mle_pdu_description_;
    std::array<std::string, 16> downlink_mle_pdu_extension_description_;

    std::array<std::string, 8> uplink_mle_pdu_description_;
    std::array<std::string, 16> uplink_mle_pdu_extension_description_;
};