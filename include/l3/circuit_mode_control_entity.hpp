/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l3/short_data_service.hpp"
#include "reporter.hpp"
#include "utils/address.hpp"
#include "utils/bit_vector.hpp"
#include "utils/packet_counter_metrics.hpp"
#include <array>
#include <string>

class CircuitModeControlEntity {
  public:
    CircuitModeControlEntity() = delete;
    explicit CircuitModeControlEntity(const std::shared_ptr<PrometheusExporter>& prometheus_exporter,
                                      Reporter&& reporter, bool is_downlink)
        : sds_(ShortDataService(std::move(reporter)))
        , is_downlink_(is_downlink) {
        if (is_downlink) {
            cmce_pdu_description_ = {"D-ALERT",        "D-CALL-PROCEEDING",
                                     "D-CONNECT",      "D-CONNECT ACKNOWLEDGE",
                                     "D-DISCONNECT",   "D-INFO",
                                     "D-RELEASE",      "D-SETUP",
                                     "D-STATUS",       "D-TX CEASED",
                                     "D-TX CONTINUE",  "D-TX GRANTED",
                                     "D-TX WAIT",      "D-TX INTERRUPT",
                                     "D-CALL-RESTORE", "D-SDS-DATA",
                                     "D-FACILITY",     "D-Reserved17",
                                     "D-Reserved18",   "D-Reserved19",
                                     "D-Reserved20",   "D-Reserved21",
                                     "D-Reserved22",   "D-Reserved23",
                                     "D-Reserved24",   "D-Reserved25",
                                     "D-Reserved26",   "D-Reserved27",
                                     "D-Reserved28",   "D-Reserved29",
                                     "D-Reserved30",   "CMCE FUNCTION NOT SUPPORTED"};
        } else {
            cmce_pdu_description_ = {"U-ALERT",      "U-Reserved1",  "U-CONNECT",      "U-Reserved3",
                                     "U-DISCONNECT", "U-INFO",       "U-RELEASE",      "U-SETUP",
                                     "U-STATUS",     "U-TX CEASED",  "U-TX DEMAND",    "U-Reserved11",
                                     "U-Reserved12", "U-Reserved13", "U-CALL-RESTORE", "U-SDS-DATA",
                                     "U-FACILITY",   "U-Reserved17", "U-Reserved18",   "U-Reserved19",
                                     "U-Reserved20", "U-Reserved21", "U-Reserved22",   "U-Reserved23",
                                     "U-Reserved24", "U-Reserved25", "U-Reserved26",   "U-Reserved27",
                                     "U-Reserved28", "U-Reserved29", "U-Reserved30",   "CMCE FUNCTION NOT SUPPORTED"};
        }

        if (prometheus_exporter) {
            metrics_ = std::make_unique<PacketCounterMetrics>(prometheus_exporter, "Circuit Mode Control Entity");
        }
    };
    ~CircuitModeControlEntity() noexcept = default;

    void process(Address address, BitVector& vec);

  private:
    void process_d_sds_data(Address to_address, BitVector& vec);
    void process_u_sds_data(Address from_address, BitVector& vec);

    static const auto kSdsData = 15;

    ShortDataService sds_;
    bool is_downlink_;

    std::array<std::string, 32> cmce_pdu_description_;

    std::unique_ptr<PacketCounterMetrics> metrics_;
};