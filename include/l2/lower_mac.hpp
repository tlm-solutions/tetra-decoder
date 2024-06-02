/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <burst_type.hpp>
#include <l2/upper_mac.hpp>
#include <prometheus.h>
#include <reporter.hpp>
#include <utils/viter_bi_codec.hpp>

/// The class to provide prometheus metrics to the lower mac
class LowerMacPrometheusCounters {
  private:
    /// The family of counters
    prometheus::Family<prometheus::Counter>& family_;
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

  public:
    LowerMacPrometheusCounters(std::shared_ptr<PrometheusExporter>& prometheus_exporter)
        : family_(prometheus_exporter->burst_received_count())
        , control_uplink_burst_received_count_(family_.Add({{"burst_type", "ControlUplinkBurst"}}))
        , normal_uplink_burst_received_count_(family_.Add({{"burst_type", "NormalUplinkBurst"}}))
        , normal_uplink_burst_split_received_count_(family_.Add({{"burst_type", "NormalUplinkBurstSplit"}}))
        , normal_downlink_burst_received_count_(family_.Add({{"burst_type", "NormalDownlinkBurst"}}))
        , normal_downlink_burst_split_received_count_(family_.Add({{"burst_type", "NormalDownlinkBurstSplit"}}))
        , synchronization_burst_received_count_(family_.Add({{"burst_type", "SynchronizationBurst"}})){};

    /// This function is called for every burst. It increments the counter associated to the burst type.
    /// \param burst_type the type of the burst for which to increment the counter
    auto increment(BurstType burst_type) -> void {
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
    }
};

class LowerMac {
  public:
    LowerMac() = delete;
    LowerMac(std::shared_ptr<Reporter> reporter, std::shared_ptr<PrometheusExporter>& prometheus_exporter);
    ~LowerMac() = default;

    // does the signal processing and then returns a list of function that need to be executed for data to be passed to
    // upper mac sequentially.
    // TODO: this is currently only done for uplink bursts
    // Downlink burst get processed and passed to upper mac directly
    [[nodiscard]] auto process(const std::vector<uint8_t>& frame, BurstType burst_type)
        -> std::vector<std::function<void()>>;
    void set_scrambling_code(unsigned int scrambling_code) { upper_mac_->set_scrambling_code(scrambling_code); };

  private:
    std::shared_ptr<Reporter> reporter_{};
    std::shared_ptr<ViterbiCodec> viter_bi_codec_1614_{};
    std::shared_ptr<UpperMac> upper_mac_{};

    std::unique_ptr<LowerMacPrometheusCounters> metrics_;

    static auto descramble(const uint8_t* const data, uint8_t* const res, const std::size_t len,
                           const uint32_t scramblingCode) noexcept -> void;
    static auto deinterleave(const uint8_t* const data, uint8_t* const res, const std::size_t K,
                             const std::size_t a) noexcept -> void;
    [[nodiscard]] static auto depuncture23(const uint8_t* const data, const uint32_t len) noexcept
        -> std::vector<int16_t>;
    static auto reed_muller_3014_decode(const uint8_t* const data, uint8_t* const res) noexcept -> void;
    [[nodiscard]] static auto check_crc_16_ccitt(const uint8_t* const data, const std::size_t len) noexcept -> bool;

    [[nodiscard]] auto viter_bi_decode_1614(const std::vector<int16_t>& data) const noexcept -> std::vector<uint8_t>;
};