/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l3/mobile_link_entity.hpp"
#include "utils/address.hpp"
#include "utils/bit_vector.hpp"
#include "utils/packet_counter_metrics.hpp"
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

class LogicalLinkControl {
  public:
    LogicalLinkControl() = delete;
    LogicalLinkControl(const std::shared_ptr<PrometheusExporter>& prometheus_exporter, Reporter&& reporter,
                       bool is_downlink)
        : mle_(prometheus_exporter, std::move(reporter), is_downlink) {
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
        if (prometheus_exporter) {
            metrics_ = std::make_unique<PacketCounterMetrics>(prometheus_exporter, "Logical Link Control");
        }
    };
    ~LogicalLinkControl() noexcept = default;

    void process(Address address, BitVector& vec);

    friend auto operator<<(std::ostream& stream, const LogicalLinkControl& llc) -> std::ostream&;

  private:
    static const auto kSupplementaryLlcPdu = 13;
    static const auto kLayer2SignallingPdu = 14;

    std::array<std::string, 16> llc_pdu_description_;
    std::array<std::string, 4> supplementary_llc_pdu_description_;
    std::array<std::string, 16> layer_2_signalling_pdu_description_;

    // Basic link (acknowledged service in connectionless mode) without Frame Check Sequence
    void process_bl_adata_without_fcs(Address address, BitVector& vec);
    // Basic link (acknowledged service in connectionless mode) without Frame Check Sequence
    void process_bl_data_without_fcs(Address address, BitVector& vec);
    void process_bl_udata_without_fcs(Address address, BitVector& vec);
    void process_bl_ack_without_fcs(Address address, BitVector& vec);

    // Basic link (acknowledged service in connectionless mode) with Frame Check Sequence
    void process_bl_adata_with_fcs(Address address, BitVector& vec);
    // Basic link (acknowledged service in connectionless mode) with Frame Check Sequence
    void process_bl_data_with_fcs(Address address, BitVector& vec);
    void process_bl_udata_with_fcs(Address address, BitVector& vec);
    void process_bl_ack_with_fcs(Address address, BitVector& vec);

    void process_supplementary_llc_pdu(Address address, BitVector& vec);
    void process_layer_2_signalling_pdu(Address address, BitVector& vec);

    static auto compute_fcs(std::vector<uint8_t> const& data) -> uint32_t;
    static auto check_fcs(BitVector& vec) -> bool;

    MobileLinkEntity mle_;

    std::unique_ptr<PacketCounterMetrics> metrics_;
};

auto operator<<(std::ostream& stream, const LogicalLinkControl& llc) -> std::ostream&;