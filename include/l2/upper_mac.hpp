/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#pragma once

#include "l2/logical_channel.hpp"
#include "l2/logical_link_control.hpp"
#include "l2/lower_mac.hpp"
#include "l2/slot.hpp"
#include "l2/upper_mac_fragments.hpp"
#include "l2/upper_mac_packet_builder.hpp"
#include "prometheus.h"
#include "reporter.hpp"
#include "streaming_ordered_output_thread_pool_executor.hpp"
#include <thread>

/// The class to provide prometheus metrics to the upper mac.
/// 1. Received Slot are counted. Details about the number of Slot with CRC errors and decoding errors are saved.
/// 2. The type of upper mac packets are counted i.e., c-plane signalling, u-plane signalling, u-plane traffic and
/// broadcast
/// 3. The count and type of c-plane signalling packets, divided into non fragmented and fragmented pieces
class UpperMacPrometheusCounters {
  private:
    /// The prometheus exporter
    std::shared_ptr<PrometheusExporter> prometheus_exporter_;

    // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)

    /// The family of counters for received slot
    prometheus::Family<prometheus::Counter>& slot_received_count_family_;
    /// The counter for received SignallingChannelHalfDownlink
    prometheus::Counter& signalling_channel_half_downlink_received_count_;
    /// The counter for received SignallingChannelHalfUplink
    prometheus::Counter& signalling_channel_half_uplink_received_count_;
    /// The counter for received TrafficChannel
    prometheus::Counter& traffic_channel_received_count_;
    /// The counter for received SignallingChannelFull
    prometheus::Counter& signalling_channel_full_received_count_;
    /// The counter for received StealingChannel
    prometheus::Counter& stealing_channel_received_count_;

    /// The family of counters for received slot with errors
    prometheus::Family<prometheus::Counter>& slot_error_count_family_;

    /// The counter for received SignallingChannelHalfDownlink with CRC errors
    prometheus::Counter& signalling_channel_half_downlink_received_count_crc_error_;
    /// The counter for received SignallingChannelHalfUplink with CRC errors
    prometheus::Counter& signalling_channel_half_uplink_received_count_crc_error_;
    /// The counter for received SignallingChannelFull with CRC errors
    prometheus::Counter& signalling_channel_full_received_count_crc_error_;
    /// The counter for received StealingChannel with CRC errors
    prometheus::Counter& stealing_channel_received_count_crc_error_;

    /// The counter for received SignallingChannelHalfDownlink with decoding errors
    prometheus::Counter& signalling_channel_half_downlink_received_count_decoding_error_;
    /// The counter for received SignallingChannelHalfUplink with decoding errors
    prometheus::Counter& signalling_channel_half_uplink_received_count_decoding_error_;
    /// The counter for received SignallingChannelFull with decoding errors
    prometheus::Counter& signalling_channel_full_received_count_decoding_error_;
    /// The counter for received StealingChannel with decoding errors
    prometheus::Counter& stealing_channel_received_count_decoding_error_;

    // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)

  public:
    UpperMacPrometheusCounters() = delete;
    explicit UpperMacPrometheusCounters(const std::shared_ptr<PrometheusExporter>& prometheus_exporter)
        : prometheus_exporter_(prometheus_exporter)
        , slot_received_count_family_(prometheus_exporter_->upper_mac_total_slot_count())
        , signalling_channel_half_downlink_received_count_(
              slot_received_count_family_.Add({{"logical_channel", "SignallingChannelHalfDownlink"}}))
        , signalling_channel_half_uplink_received_count_(
              slot_received_count_family_.Add({{"logical_channel", "SignallingChannelHalfUplink"}}))
        , traffic_channel_received_count_(slot_received_count_family_.Add({{"logical_channel", "TrafficChannel"}}))
        , signalling_channel_full_received_count_(
              slot_received_count_family_.Add({{"logical_channel", "SignallingChannelFull"}}))
        , stealing_channel_received_count_(slot_received_count_family_.Add({{"logical_channel", "StealingChannel"}}))
        , slot_error_count_family_(prometheus_exporter_->upper_mac_slot_error_count())
        , signalling_channel_half_downlink_received_count_crc_error_(slot_error_count_family_.Add(
              {{"logical_channel", "SignallingChannelHalfDownlink"}, {"error_type", "CRC Error"}}))
        , signalling_channel_half_uplink_received_count_crc_error_(slot_error_count_family_.Add(
              {{"logical_channel", "SignallingChannelHalfUplink"}, {"error_type", "CRC Error"}}))
        , signalling_channel_full_received_count_crc_error_(
              slot_error_count_family_.Add({{"logical_channel", "SignallingChannelFull"}, {"error_type", "CRC Error"}}))
        , stealing_channel_received_count_crc_error_(
              slot_error_count_family_.Add({{"logical_channel", "StealingChannel"}, {"error_type", "CRC Error"}}))
        , signalling_channel_half_downlink_received_count_decoding_error_(slot_error_count_family_.Add(
              {{"logical_channel", "SignallingChannelHalfDownlink"}, {"error_type", "Decode Error"}}))
        , signalling_channel_half_uplink_received_count_decoding_error_(slot_error_count_family_.Add(
              {{"logical_channel", "SignallingChannelHalfUplink"}, {"error_type", "Decode Error"}}))
        , signalling_channel_full_received_count_decoding_error_(slot_error_count_family_.Add(
              {{"logical_channel", "SignallingChannelFull"}, {"error_type", "Decode Error"}}))
        , stealing_channel_received_count_decoding_error_(
              slot_error_count_family_.Add({{"logical_channel", "StealingChannel"}, {"error_type", "Decode Error"}})){};

    /// This function is called for every slot once it is passed up from the lower MAC
    /// \param slot the content of the slot
    auto increment(const ConcreateSlot& slot) -> void {
        switch (slot.logical_channel_data_and_crc.channel) {
        case LogicalChannel::kSignallingChannelHalfDownlink:
            signalling_channel_half_downlink_received_count_.Increment();
            break;
        case LogicalChannel::kSignallingChannelHalfUplink:
            signalling_channel_half_uplink_received_count_.Increment();
            break;
        case LogicalChannel::kTrafficChannel:
            traffic_channel_received_count_.Increment();
            break;
        case LogicalChannel::kSignallingChannelFull:
            signalling_channel_full_received_count_.Increment();
            break;
        case LogicalChannel::kStealingChannel:
            stealing_channel_received_count_.Increment();
            break;
        }

        if (!slot.logical_channel_data_and_crc.crc_ok) {
            switch (slot.logical_channel_data_and_crc.channel) {
            case LogicalChannel::kSignallingChannelHalfDownlink:
                signalling_channel_half_downlink_received_count_crc_error_.Increment();
                break;
            case LogicalChannel::kSignallingChannelHalfUplink:
                signalling_channel_half_uplink_received_count_crc_error_.Increment();
                break;
            case LogicalChannel::kTrafficChannel:
                break;
            case LogicalChannel::kSignallingChannelFull:
                signalling_channel_full_received_count_crc_error_.Increment();
                break;
            case LogicalChannel::kStealingChannel:
                stealing_channel_received_count_crc_error_.Increment();
                break;
            }
        }
    }

    /// This function is called for every slot once if there were decode errors in upper mac layers
    /// \param slot the content of the slot
    auto increment_decode_error(const ConcreateSlot& slot) -> void {
        switch (slot.logical_channel_data_and_crc.channel) {
        case LogicalChannel::kSignallingChannelHalfDownlink:
            signalling_channel_half_downlink_received_count_decoding_error_.Increment();
            break;
        case LogicalChannel::kSignallingChannelHalfUplink:
            signalling_channel_half_uplink_received_count_decoding_error_.Increment();
            break;
        case LogicalChannel::kTrafficChannel:
            break;
        case LogicalChannel::kSignallingChannelFull:
            signalling_channel_full_received_count_decoding_error_.Increment();
            break;
        case LogicalChannel::kStealingChannel:
            stealing_channel_received_count_decoding_error_.Increment();
            break;
        }
    }
};

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
    std::unique_ptr<UpperMacPrometheusCounters> metrics_;

    std::unique_ptr<LogicalLinkControl> logical_link_control_;

    UpperMacFragmentation fragmentation_;

    /// The worker thread
    std::thread worker_thread_;
};