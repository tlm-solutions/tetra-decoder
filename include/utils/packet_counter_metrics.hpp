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

class PacketCounterMetrics {
  private:
    /// The prometheus exporter
    std::shared_ptr<PrometheusExporter> prometheus_exporter_;

    // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)

    /// The family of counters for packet counts
    prometheus::Family<prometheus::Counter>& received_packet_count_;

    /// Map from the name of the received packet to the counter for it.
    std::map<std::string, prometheus::Counter*> received_packet_counter_map_;

    // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)

  public:
    PacketCounterMetrics() = delete;
    explicit PacketCounterMetrics(const std::shared_ptr<PrometheusExporter>& prometheus_exporter,
                                  const std::string& protocol)
        : prometheus_exporter_(prometheus_exporter)
        , received_packet_count_(prometheus_exporter_->packet_count(protocol)){};

    /// This function is called for every packet_type
    /// \param packet_type the type of the packet described as a name
    auto increment(const std::string& packet_type) -> void {
        if (!received_packet_counter_map_.count(packet_type)) {
            received_packet_counter_map_[packet_type] = &received_packet_count_.Add({{"packet_type", packet_type}});
        }

        received_packet_counter_map_[packet_type]->Increment();
    };

    /// This function is called for every packet_type
    /// \param packet_type the type of the packet described as a name
    /// \param increment how much should the counter be incremented
    auto increment(const std::string& packet_type, unsigned increment) -> void {
        if (!received_packet_counter_map_.count(packet_type)) {
            received_packet_counter_map_[packet_type] = &received_packet_count_.Add({{"packet_type", packet_type}});
        }

        received_packet_counter_map_[packet_type]->Increment(increment);
    }
};