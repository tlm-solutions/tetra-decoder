/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#pragma once

#include "burst_type.hpp"
#include "l2/broadcast_synchronization_channel.hpp"
#include "l2/lower_mac_metrics.hpp"
#include "l2/slot.hpp"
#include "prometheus.h"
#include "utils/viter_bi_codec.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

class LowerMac {
  public:
    using return_type = std::optional<Slots>;

    LowerMac() = delete;
    explicit LowerMac(const std::shared_ptr<PrometheusExporter>& prometheus_exporter,
                      std::optional<uint32_t> scrambling_code = std::nullopt);
    ~LowerMac() = default;

    /// handles the decoding of the synchronization bursts and once synchronized passes the data to the decoding of the
    /// channels. keeps track of the current network time
    [[nodiscard]] auto process(std::vector<uint8_t> frame, BurstType burst_type) -> return_type;

  private:
    // does the signal processing and then returns the slots containing the correct logical channels and their
    // associated data to be passed to the upper mac and further processed in a sequential order.
    [[nodiscard]] auto processChannels(const std::vector<uint8_t>& frame, BurstType burst_type,
                                       const BroadcastSynchronizationChannel& bsc) -> Slots;

    const ViterbiCodec viter_bi_codec_1614_;

    std::unique_ptr<LowerMacMetrics> metrics_;

    /// The last received synchronization burst.
    /// This include the current scrambling code. Set by Synchronization Burst on downlink or injected from the side for
    /// uplink processing, as we decouple it from the downlink for data/control packets.
    std::optional<BroadcastSynchronizationChannel> sync_;
};