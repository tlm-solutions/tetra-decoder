/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "prometheus.h"
#include "utils/packet_counter_metrics.hpp"
#include <cstddef>
#include <memory>

/// This is the template that is used to keep an consistent interface in the protocol parsing.
/// 1. The input of type Input is passed into the constructor of type Output.
/// 2. The metrics are incremented in the increment_metrics function.
/// 3. The packet is forwared to the next parsing stage or returned if this is not necessary.
template <typename Input, typename Output> class PacketParser {
  private:
    /// The metrics for this parser
    std::unique_ptr<PacketCounterMetrics> metrics_;

  public:
    PacketParser() = delete;
    virtual ~PacketParser() = default;

    explicit PacketParser(const std::shared_ptr<PrometheusExporter>& prometheus_exporter,
                          const std::string& packet_parser_name) {
        if (prometheus_exporter) {
            metrics_ = std::make_unique<PacketCounterMetrics>(prometheus_exporter, packet_parser_name);
        }
    };

    /// Parse the input packet, increment the metrics and return the parsed packet passed through all the layers defined
    /// in the forward function.
    virtual auto parse(const Input& input) -> std::unique_ptr<Output> {
        /// Parse this layer
        auto packet = Output(input);
        /// Increment the metrics
        if (metrics_) {
            metrics_->increment(packet_name(packet));
        }
        /// Forward it to further parsing steps
        return forward(packet);
    };

  protected:
    /// This function needs to be implemented for each parsing layer. It should return the correct packet name.
    virtual auto packet_name(const Output&) -> std::string = 0;

    /// This function take the currently parsed packet and should forward it to the next parsing stage or return a
    /// unique pointer to it.
    virtual auto forward(const Output&) -> std::unique_ptr<Output> = 0;
};