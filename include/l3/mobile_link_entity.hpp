/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#pragma once

#include "l3/circuit_mode_control_entity.hpp"
#include "l3/mobile_management.hpp"
#include "prometheus.h"
#include "reporter.hpp"
#include "utils/bit_vector.hpp"
#include "utils/packet_counter_metrics.hpp"
#include <memory>

class MobileLinkEntity {
  public:
    MobileLinkEntity() = delete;
    MobileLinkEntity(const std::shared_ptr<PrometheusExporter>& prometheus_exporter, Reporter&& reporter,
                     bool is_downlink)
        : cmce_(CircuitModeControlEntity(prometheus_exporter, std::move(reporter), is_downlink))
        , mm_(MobileManagement(prometheus_exporter, is_downlink)) {
        protocol_discriminator_description_ = {"Reserved0",
                                               "MM protocol",
                                               "CMCE protocol",
                                               "Reserved3",
                                               "SNDCP protocol",
                                               "MLE protocol",
                                               "TETRA management entity protocol",
                                               "Reserved for testing"};

        if (is_downlink) {
            mle_pdu_description_ = {
                "D-NEW-CELL",    "D-PREPARE-FAIL", "D-NWRK-BROADCAST",   "D-NWRK-BROADCAST EXTENSION",
                "D-RESTORE-ACK", "D-RESTORE-FAIL", "D-CHANNEL RESPONSE", "Extended PDU"};
            mle_pdu_extension_description_ = {"D-NWRK-BROADCAST-DA", "D-NWRK-BROADCAST REMOVE",
                                              "D-Reserved2",         "D-Reserved3",
                                              "D-Reserved4",         "D-Reserved5",
                                              "D-Reserved6",         "D-Reserved7",
                                              "D-Reserved8",         "D-Reserved9",
                                              "D-Reserved10",        "D-Reserved11",
                                              "D-Reserved12",        "D-Reserved13",
                                              "D-Reserved14",        "D-Reserved15"};
        } else {
            mle_pdu_description_ = {"U-PREPARE", "U-PREPARE-DA", "U-IRREGULAR CHANNEL ADVICE", "U-CHANNEL CLASS ADVICE",
                                    "U-RESTORE", "Reserved",     "U-CHANNEL REQUEST",          "Extended PDU"};
            mle_pdu_extension_description_ = {"U-Reserved0",  "U-Reserved1",  "U-Reserved2",  "U-Reserved3",
                                              "U-Reserved4",  "U-Reserved5",  "U-Reserved6",  "U-Reserved7",
                                              "U-Reserved8",  "U-Reserved9",  "U-Reserved10", "U-Reserved11",
                                              "U-Reserved12", "U-Reserved13", "U-Reserved14", "U-Reserved15"};
        }

        if (prometheus_exporter) {
            metrics_ = std::make_unique<PacketCounterMetrics>(prometheus_exporter, "mobile_link_entity");
        }
    };
    ~MobileLinkEntity() noexcept = default;

    void service_user_pdu(Address address, BitVector& vec);

  private:
    void service_data_pdu(Address address, BitVector& vec);

    CircuitModeControlEntity cmce_;
    MobileManagement mm_;

    static const auto kMleProtocol = 5;
    static const auto kExtendedPdu = 7;

    std::array<std::string, 8> protocol_discriminator_description_;
    std::array<std::string, 8> mle_pdu_description_;
    std::array<std::string, 16> mle_pdu_extension_description_;

    std::unique_ptr<PacketCounterMetrics> metrics_;
};