/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#include "l2/lower_mac.hpp"
#include "l2/broadcast_synchronization_channel.hpp"
#include <cstdint>
#include <cstring>
#include <fmt/color.h>
#include <fmt/core.h>
#include <optional>

LowerMac::LowerMac(const std::shared_ptr<PrometheusExporter>& prometheus_exporter,
                   std::optional<uint32_t> scrambling_code) {
    // For decoupled uplink processing we need to inject a scrambling code. Inject it into the correct place that would
    // normally be filled by a Synchronization Burst
    if (scrambling_code.has_value()) {
        sync_ = BroadcastSynchronizationChannel();
        sync_->scrambling_code = *scrambling_code;
    }

    if (prometheus_exporter) {
        metrics_ = std::make_unique<LowerMacMetrics>(prometheus_exporter);
    }
}