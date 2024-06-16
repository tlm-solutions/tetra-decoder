/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#pragma once

#include "l2/logical_link_control.hpp"
#include "l2/lower_mac.hpp"
#include "l2/slot.hpp"
#include "l2/upper_mac_fragments.hpp"
#include "l2/upper_mac_metrics.hpp"
#include "l2/upper_mac_packet_builder.hpp"
#include "prometheus.h"
#include "reporter.hpp"
#include "streaming_ordered_output_thread_pool_executor.hpp"
#include <memory>
#include <thread>

class UpperMac {
  public:
    UpperMac() = delete;
    ///
    /// \param queue the input queue from the lower mac
    /// \param prometheus_exporter the reference to the prometheus exporter that is used for the metrics in the upper
    /// mac
    /// \param is_downlink true if this channel is on the downlink
    UpperMac(const std::shared_ptr<StreamingOrderedOutputThreadPoolExecutor<LowerMac::return_type>>& input_queue,
             const std::shared_ptr<PrometheusExporter>& prometheus_exporter, const std::shared_ptr<Reporter>& reporter,
             bool is_downlink);
    ~UpperMac();

  private:
    /// The thread function for continously process incomming packets for the lower MAC and passing it into the upper
    /// layers.
    auto worker() -> void;

    /// process the slots from the lower MAC
    /// \param slots the slots from the lower MAC
    auto process(const Slots& slots) -> void;

    /// process Upper MAC packets and perform fragment reconstruction and pass it to the upper layers
    /// \param packets the packets that were parsed in the upper MAC layer
    auto processPackets(UpperMacPackets&& packets) -> void;

    /// The input queue
    std::shared_ptr<StreamingOrderedOutputThreadPoolExecutor<LowerMac::return_type>> input_queue_;

    /// The prometheus metrics
    std::unique_ptr<UpperMacMetrics> metrics_;

    /// The prometheus metrics for the fragmentation
    std::shared_ptr<UpperMacFragmentsPrometheusCounters> fragmentation_metrics_continous_;
    std::shared_ptr<UpperMacFragmentsPrometheusCounters> fragmentation_metrics_stealing_channel_;

    std::unique_ptr<LogicalLinkControl> logical_link_control_;

    std::unique_ptr<UpperMacFragmentation> fragmentation_;

    /// The worker thread
    std::thread worker_thread_;
};