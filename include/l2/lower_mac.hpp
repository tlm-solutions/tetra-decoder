/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#pragma once

#include "l2/broadcast_synchronization_channel.hpp"
#include "l2/slot.hpp"
#include "l2/timebase_counter.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include <burst_type.hpp>
#include <l2/upper_mac.hpp>
#include <prometheus.h>
#include <reporter.hpp>
#include <utils/viter_bi_codec.hpp>

/// The class to provide prometheus metrics to the lower mac
class LowerMacPrometheusCounters {
  private:
    /// The family of counters for received bursts
    prometheus::Family<prometheus::Counter>& burst_received_count_family_;
    /// The counter for the received ControlUplinkBurst
    prometheus::Counter& control_uplink_burst_received_count_;
    /// The counter for the received NormalUplinkBurst
    prometheus::Counter& normal_uplink_burst_received_count_;
    /// The counter for the received NormalUplinkBurstSplit
    prometheus::Counter& normal_uplink_burst_split_received_count_;
    /// The counter for the received NormalDownlinkBurst
    prometheus::Counter& normal_downlink_burst_received_count_;
    /// The counter for the received NormalDownlinkBurstSplit
    prometheus::Counter& normal_downlink_burst_split_received_count_;
    /// The counter for the received SynchronizationBurst
    prometheus::Counter& synchronization_burst_received_count_;

    /// The family of counters for decoding errors on received bursts in the lower MAC
    prometheus::Family<prometheus::Counter>& burst_lower_mac_decode_error_count_family_;
    /// The counter for the received ControlUplinkBurst with decode errors
    prometheus::Counter& control_uplink_burst_lower_mac_decode_error_count_;
    /// The counter for the received NormalUplinkBurst with decode errors
    prometheus::Counter& normal_uplink_burst_lower_mac_decode_error_count_;
    /// The counter for the received NormalUplinkBurstSplit with decode errors
    prometheus::Counter& normal_uplink_burst_split_lower_mac_decode_error_count_;
    /// The counter for the received NormalDownlinkBurst with decode errors
    prometheus::Counter& normal_downlink_burst_lower_mac_decode_error_count_;
    /// The counter for the received NormalDownlinkBurstSplit with decode errors
    prometheus::Counter& normal_downlink_burst_split_lower_mac_decode_error_count_;
    /// The counter for the received SynchronizationBurst with decode errors
    prometheus::Counter& synchronization_burst_lower_mac_decode_error_count_;

    /// The family of counters for mismatched number of bursts in the downlink lower MAC
    prometheus::Family<prometheus::Counter>& burst_lower_mac_mismatch_count_family_;
    /// The counter for the skipped bursts (lost bursts) in the downlink lower MAC
    prometheus::Counter& lower_mac_burst_skipped_count_;
    /// The counter for the too many bursts in the downlink lower MAC
    prometheus::Counter& lower_mac_burst_too_many_count_;

    /// TODO: add gauge for the synchronization burst time

  public:
    LowerMacPrometheusCounters(std::shared_ptr<PrometheusExporter>& prometheus_exporter)
        : burst_received_count_family_(prometheus_exporter->burst_received_count())
        , control_uplink_burst_received_count_(burst_received_count_family_.Add({{"burst_type", "ControlUplinkBurst"}}))
        , normal_uplink_burst_received_count_(burst_received_count_family_.Add({{"burst_type", "NormalUplinkBurst"}}))
        , normal_uplink_burst_split_received_count_(
              burst_received_count_family_.Add({{"burst_type", "NormalUplinkBurstSplit"}}))
        , normal_downlink_burst_received_count_(
              burst_received_count_family_.Add({{"burst_type", "NormalDownlinkBurst"}}))
        , normal_downlink_burst_split_received_count_(
              burst_received_count_family_.Add({{"burst_type", "NormalDownlinkBurstSplit"}}))
        , synchronization_burst_received_count_(
              burst_received_count_family_.Add({{"burst_type", "SynchronizationBurst"}}))
        , burst_lower_mac_decode_error_count_family_(prometheus_exporter->burst_lower_mac_decode_error_count())
        , control_uplink_burst_lower_mac_decode_error_count_(
              burst_lower_mac_decode_error_count_family_.Add({{"burst_type", "ControlUplinkBurst"}}))
        , normal_uplink_burst_lower_mac_decode_error_count_(
              burst_lower_mac_decode_error_count_family_.Add({{"burst_type", "NormalUplinkBurst"}}))
        , normal_uplink_burst_split_lower_mac_decode_error_count_(
              burst_lower_mac_decode_error_count_family_.Add({{"burst_type", "NormalUplinkBurstSplit"}}))
        , normal_downlink_burst_lower_mac_decode_error_count_(
              burst_lower_mac_decode_error_count_family_.Add({{"burst_type", "NormalDownlinkBurst"}}))
        , normal_downlink_burst_split_lower_mac_decode_error_count_(
              burst_lower_mac_decode_error_count_family_.Add({{"burst_type", "NormalDownlinkBurstSplit"}}))
        , synchronization_burst_lower_mac_decode_error_count_(
              burst_lower_mac_decode_error_count_family_.Add({{"burst_type", "SynchronizationBurst"}}))
        , burst_lower_mac_mismatch_count_family_(prometheus_exporter->burst_lower_mac_mismatch_count())
        , lower_mac_burst_skipped_count_(burst_lower_mac_mismatch_count_family_.Add({{"mismatch_type", "Skipped"}}))
        , lower_mac_burst_too_many_count_(
              burst_lower_mac_mismatch_count_family_.Add({{"mismatch_type", "Too many"}})){};

    /// This function is called for every burst. It increments the counter associated to the burst type.
    /// \param burst_type the type of the burst for which to increment the counter
    /// \param decode_error true if there was an error decoding the packets on the lower mac (crc16)
    auto increment(BurstType burst_type, bool decode_error) -> void {
        switch (burst_type) {
        case BurstType::ControlUplinkBurst:
            control_uplink_burst_received_count_.Increment();
            break;
        case BurstType::NormalUplinkBurst:
            normal_uplink_burst_received_count_.Increment();
            break;
        case BurstType::NormalUplinkBurstSplit:
            normal_uplink_burst_split_received_count_.Increment();
            break;
        case BurstType::NormalDownlinkBurst:
            normal_downlink_burst_received_count_.Increment();
            break;
        case BurstType::NormalDownlinkBurstSplit:
            normal_downlink_burst_split_received_count_.Increment();
            break;
        case BurstType::SynchronizationBurst:
            synchronization_burst_received_count_.Increment();
            break;
        }

        if (decode_error) {
            switch (burst_type) {
            case BurstType::ControlUplinkBurst:
                control_uplink_burst_lower_mac_decode_error_count_.Increment();
                break;
            case BurstType::NormalUplinkBurst:
                normal_uplink_burst_lower_mac_decode_error_count_.Increment();
                break;
            case BurstType::NormalUplinkBurstSplit:
                normal_uplink_burst_split_lower_mac_decode_error_count_.Increment();
                break;
            case BurstType::NormalDownlinkBurst:
                normal_downlink_burst_lower_mac_decode_error_count_.Increment();
                break;
            case BurstType::NormalDownlinkBurstSplit:
                normal_downlink_burst_split_lower_mac_decode_error_count_.Increment();
                break;
            case BurstType::SynchronizationBurst:
                synchronization_burst_lower_mac_decode_error_count_.Increment();
                break;
            }
        }
    }

    /// This function is called for Synchronization Bursts. It increments the counters for the missmatched received
    /// burst counts.
    /// \param current_timestamp the current timestamp of the synchronization burst
    /// \param expected_timestamp the predicted timestamp of the synchronization burst
    auto increment(const TimebaseCounter current_timestamp, const TimebaseCounter expected_timestamp) {
        auto current_timestamp_count = current_timestamp.count();
        auto expected_timestamp_count = expected_timestamp.count();

        if (current_timestamp_count > expected_timestamp_count) {
            auto missed_bursts = current_timestamp_count - expected_timestamp_count;
            lower_mac_burst_skipped_count_.Increment(missed_bursts);
        } else if (expected_timestamp_count > current_timestamp_count) {
            auto too_many_bursts = expected_timestamp_count - current_timestamp_count;
            lower_mac_burst_too_many_count_.Increment(too_many_bursts);
        }
    }
};

class LowerMac {
  public:
    LowerMac() = delete;
    LowerMac(std::shared_ptr<Reporter> reporter, std::shared_ptr<PrometheusExporter>& prometheus_exporter,
             std::optional<uint32_t> scrambling_code = std::nullopt);
    ~LowerMac() = default;

    // does the signal processing and then returns the slots containing the correct logical channels and their
    // associated data to be passed to the upper mac and further processed in a sequential order.
    [[nodiscard]] auto processChannels(const std::vector<uint8_t>& frame, BurstType burst_type,
                                       const BroadcastSynchronizationChannel& bsc) -> Slots;

    /// handles the decoding of the synchronization bursts and once synchronized passes the data to the decoding of the
    /// channels. keeps track of the current network time
    [[nodiscard]] auto process(const std::vector<uint8_t>& frame, BurstType burst_type)
        -> std::vector<std::function<void()>>;

  private:
    std::shared_ptr<Reporter> reporter_;
    const ViterbiCodec viter_bi_codec_1614_;
    std::shared_ptr<UpperMac> upper_mac_;

    std::unique_ptr<LowerMacPrometheusCounters> metrics_;

    /// The last received synchronization burst.
    /// This include the current scrambling code. Set by Synchronization Burst on downlink or injected from the side for
    /// uplink processing, as we decouple it from the downlink for data/control packets.
    std::optional<BroadcastSynchronizationChannel> sync_;
};