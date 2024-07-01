/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l3/mobile_link_entity.hpp"
#include "utils/packet_parser.hpp"

class LogicalLinkControlParser : public PacketParser<UpperMacCPlaneSignallingPacket, LogicalLinkControlPacket> {
  public:
    LogicalLinkControlParser() = delete;
    explicit LogicalLinkControlParser(const std::shared_ptr<PrometheusExporter>& prometheus_exporter)
        : PacketParser(prometheus_exporter, "logical_link_control")
        , mle_(prometheus_exporter) {
        llc_pdu_description_ = {"BL-ADATA without FCS",
                                "BL-DATA without FCS",
                                "BL-UDATA without FCS",
                                "BL-ACK without FCS",
                                "BL-ADATA with FCS",
                                "BL-DATA with FCS",
                                "BL-UDATA with FCS",
                                "BL-ACK with FCS",
                                "AL-SETUP",
                                "AL-DATA/AL-DATA-AR/AL-FINAL/AL-FINAL-AR",
                                "AL-UDATA/AL-UFINAL",
                                "AL-ACK/AL-RNR",
                                "AL-RECONNECT",
                                "Supplementary LLC PDU",
                                "Layer 2 signalling PDU",
                                "AL-DISC"};
        supplementary_llc_pdu_description_ = {"AL-X-DATA/AL-X-DATA-AR/AL-X-FINAL/AL-X-FINAL-AR",
                                              "AL-X-UDATA/AL-X-UFINAL", "AL-X-ACK/AL-X-RNR",
                                              "ReservedSupplementaryLlcPdu"};
        layer_2_signalling_pdu_description_ = {"L2-DATA-PRIORITY",
                                               "L2-SCHEDULE-SYNC",
                                               "L2-LINK-FEEDBACK-CONTROL",
                                               "L2-LINK-FEEDBACK-INFO",
                                               "L2-LINK-FEEDBACK-INFO-AND-RESIDUAL-DATA-PRIORITY",
                                               "ReservedLayer2SignallingPdu5",
                                               "ReservedLayer2SignallingPdu6",
                                               "ReservedLayer2SignallingPdu7",
                                               "ReservedLayer2SignallingPdu8",
                                               "ReservedLayer2SignallingPdu9",
                                               "ReservedLayer2SignallingPdu10",
                                               "ReservedLayer2SignallingPdu11",
                                               "ReservedLayer2SignallingPdu12",
                                               "ReservedLayer2SignallingPdu13",
                                               "ReservedLayer2SignallingPdu14",
                                               "ReservedLayer2SignallingPdu15"};
    };

  private:
    auto packet_name(const LogicalLinkControlPacket& packet) -> std::string override {
        auto pdu_type = packet.tm_sdu_->look<4>(0);

        if (pdu_type == kSupplementaryLlcPdu) {
            auto pdu_type = packet.tm_sdu_->look<2>(4);
            return supplementary_llc_pdu_description_.at(pdu_type);
        }

        if (pdu_type == kLayer2SignallingPdu) {
            auto pdu_type = packet.tm_sdu_->look<4>(4);
            return layer_2_signalling_pdu_description_.at(pdu_type);
        }

        return llc_pdu_description_.at(pdu_type);
    };

    auto forward(const LogicalLinkControlPacket& packet) -> std::unique_ptr<LogicalLinkControlPacket> override {
        if (packet.basic_link_information_ && packet.tl_sdu_.bits_left() > 0) {
            return mle_.parse(packet);
        }
        return std::make_unique<LogicalLinkControlPacket>(packet);
    };

    MobileLinkEntityParser mle_;

    static const auto kSupplementaryLlcPdu = 13;
    static const auto kLayer2SignallingPdu = 14;

    std::array<std::string, 16> llc_pdu_description_;
    std::array<std::string, 4> supplementary_llc_pdu_description_;
    std::array<std::string, 16> layer_2_signalling_pdu_description_;
};