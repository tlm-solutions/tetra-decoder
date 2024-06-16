/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "prometheus.h"
#include <memory>

class LogicalLinkControlMetrics {
  private:
    /// The prometheus exporter
    std::shared_ptr<PrometheusExporter> prometheus_exporter_;

    // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)

    /// The family of counters for packet counts
    prometheus::Family<prometheus::Counter>& llc_received_packet_count_;

    /// Map from the name of the received LLV packet to the couter for it.
    std::map<std::string, prometheus::Counter*> lcc_received_packet_counter_map_;

    // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)

  public:
    LogicalLinkControlMetrics() = delete;
    explicit LogicalLinkControlMetrics(const std::shared_ptr<PrometheusExporter>& prometheus_exporter)
        : prometheus_exporter_(prometheus_exporter)
        , llc_received_packet_count_(prometheus_exporter_->logical_link_control_packet_count()){};

    /// This function is called for every LLC PDU
    /// \param type_name the type of the pdu described as a name
    auto increment(const std::string& type_name) -> void {
        if (!lcc_received_packet_counter_map_.count(type_name)) {
            lcc_received_packet_counter_map_[type_name] = &llc_received_packet_count_.Add({{"pdu_type", type_name}});
        }

        lcc_received_packet_counter_map_[type_name]->Increment();
    }
};