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
#include "l2/lower_mac_coding.hpp"
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
    /// \targ DataType with bool is selected we have bits, with int16_t we have symbols
    template <typename DataType>
    [[nodiscard]] auto process(std::vector<DataType> frame, BurstType burst_type) -> return_type {
        // Set to true if there was some decoding error in the lower MAC
        bool decode_error = false;

        // fmt::print("[Physical Channel] Decoding: {}\n", burst_type);

        // Once we received the Synchronization on the downlink, increment the time counter for every received
        // burst. We do not have any time handling for uplink processing.
        if (sync_ && is_downlink_burst(burst_type)) {
            sync_->time.increment();
            if (metrics_) {
                metrics_->set_time(sync_->time);
            }
        }

        if (burst_type == BurstType::SynchronizationBurst) {
            std::optional<BroadcastSynchronizationChannel> current_sync;

            // sb contains BSCH
            // ✅ done
            std::array<DataType, 120> sb_input{};
            for (auto i = 0; i < 120; i++) {
                sb_input[i] = frame[94 + i];
            };

            auto sb_bits = LowerMacCoding::viter_bi_decode_1614(
                viter_bi_codec_1614_, LowerMacCoding::depuncture23(LowerMacCoding::deinterleave(
                                          LowerMacCoding::descramble(sb_input, 0x0003), 11)));

            if (LowerMacCoding::check_crc_16_ccitt<76>(sb_bits)) {
                current_sync = BroadcastSynchronizationChannel(
                    burst_type, BitVector(std::vector(sb_bits.cbegin(), sb_bits.cbegin() + 60)));
            } else {
                decode_error |= true;
            }

            // Update the mismatching received number of bursts metrics
            if (current_sync && sync_ && metrics_) {
                metrics_->set_time(/*current_timestamp=*/current_sync->time, /*expected_timestamp=*/sync_->time);
            }
            sync_ = current_sync;
        }

        std::optional<Slots> slots;

        // We got a sync, continue with further processing of channels
        if (sync_) {
            slots = processChannels(frame, burst_type, *sync_);

            // check if we have crc decode errors in the lower mac
            decode_error |= slots->has_crc_error();
        }

        // Update the received burst type metrics
        if (metrics_) {
            metrics_->increment(burst_type, decode_error);
        }

        return slots;
    }

  private:
    // does the signal processing and then returns the slots containing the correct logical channels and their
    // associated data to be passed to the upper mac and further processed in a sequential order.
    [[nodiscard]] auto processChannels(const std::vector<bool>& frame, BurstType burst_type,
                                       const BroadcastSynchronizationChannel& bsc) -> Slots;

    const ViterbiCodec viter_bi_codec_1614_;

    std::unique_ptr<LowerMacMetrics> metrics_;

    /// The last received synchronization burst.
    /// This include the current scrambling code. Set by Synchronization Burst on downlink or injected from the side for
    /// uplink processing, as we decouple it from the downlink for data/control packets.
    std::optional<BroadcastSynchronizationChannel> sync_;
};