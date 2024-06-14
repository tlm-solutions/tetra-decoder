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
#include "l2/upper_mac_fragments.hpp"
#include "l2/upper_mac_packet_builder.hpp"
#include "l3/mobile_link_entity.hpp"
#include "prometheus.h"
#include "reporter.hpp"

/// The class to provide prometheus metrics to the upper mac.
/// 1. Received Slot are counted. Details about the number of Slot with CRC errors and decoding errors are saved.
/// 2. The type of upper mac packets are counted i.e., c-plane signalling, u-plane signalling, u-plane traffic and
/// broadcast
/// 3. The count and type of c-plane signalling packets, divided into non fragmented and fragmented pieces
class UpperMacPrometheusCounters {
  private:
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
    explicit UpperMacPrometheusCounters(std::shared_ptr<PrometheusExporter>& prometheus_exporter)
        : slot_received_count_family_(prometheus_exporter->upper_mac_total_slot_count())
        , signalling_channel_half_downlink_received_count_(
              slot_received_count_family_.Add({{"logical_channel", "SignallingChannelHalfDownlink"}}))
        , signalling_channel_half_uplink_received_count_(
              slot_received_count_family_.Add({{"logical_channel", "SignallingChannelHalfUplink"}}))
        , traffic_channel_received_count_(slot_received_count_family_.Add({{"logical_channel", "TrafficChannel"}}))
        , signalling_channel_full_received_count_(
              slot_received_count_family_.Add({{"logical_channel", "SignallingChannelFull"}}))
        , stealing_channel_received_count_(slot_received_count_family_.Add({{"logical_channel", "StealingChannel"}}))
        , slot_error_count_family_(prometheus_exporter->upper_mac_slot_error_count())
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
    /// \param logical_channel_data_and_crc the content of the slot
    auto increment(const LogicalChannelDataAndCrc& logical_channel_data_and_crc) -> void {
        switch (logical_channel_data_and_crc.channel) {
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

        if (!logical_channel_data_and_crc.crc_ok) {
            switch (logical_channel_data_and_crc.channel) {
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
};

class UpperMac {
  public:
    UpperMac() = delete;
    UpperMac(std::shared_ptr<PrometheusExporter>& prometheus_exporter, std::shared_ptr<Reporter> reporter,
             bool is_downlink)
        : reporter_(std::move(reporter))
        , mobile_link_entity_(std::make_shared<MobileLinkEntity>(reporter_, is_downlink))
        , logical_link_control_(std::make_unique<LogicalLinkControl>(reporter_, mobile_link_entity_)) {
        if (prometheus_exporter) {
            metrics_ = std::make_unique<UpperMacPrometheusCounters>(prometheus_exporter);
        }
    };
    ~UpperMac() noexcept = default;

    /// process the slots from the lower MAC
    /// \param slots the slots from the lower MAC
    auto process(Slots& slots) -> void {
        UpperMacPackets packets;

        // std::cout << slots;

        // increment the total count and crc error count metrics
        metrics_->increment(slots.get_first_slot().get_logical_channel_data_and_crc());
        if (slots.has_second_slot()) {
            metrics_->increment(slots.get_second_slot().get_logical_channel_data_and_crc());
        }

        try {
            packets = UpperMacPacketBuilder::parse_slots(slots);
            // if (packets.has_user_or_control_plane_data()) {
            //     std::cout << packets << std::endl;
            // }
        } catch (std::runtime_error& e) {
            std::cout << "Error decoding packets: " << e.what() << std::endl;
            return;
        }

        try {
            processPackets(std::move(packets));

        } catch (std::runtime_error& e) {
            std::cout << "Error decoding in upper mac: " << e.what() << std::endl;
        }
    }

    /// process Upper MAC packets and perform fragment reconstruction and pass it to the upper layers
    /// \param packets the packets that were parsed in the upper MAC layer
    auto processPackets(UpperMacPackets&& packets) -> void {
        for (const auto& packet : packets.c_plane_signalling_packets_) {
            // TODO: handle fragmentation over STCH
            if (packet.is_downlink_fragment() || packet.is_uplink_fragment()) {
                auto reconstructed_fragment = fragmentation_.push_fragment(packet);
                if (reconstructed_fragment) {
                    auto data = BitVector(*reconstructed_fragment->tm_sdu_);
                    logical_link_control_->process(reconstructed_fragment->address_, data);
                }
            } else if (packet.tm_sdu_) {
                auto data = BitVector(*packet.tm_sdu_);
                logical_link_control_->process(packet.address_, data);
            }
        }
    };

  private:
    std::shared_ptr<Reporter> reporter_;
    std::shared_ptr<MobileLinkEntity> mobile_link_entity_;
    std::unique_ptr<LogicalLinkControl> logical_link_control_;

    std::unique_ptr<UpperMacPrometheusCounters> metrics_;

    UpperMacFragmentation fragmentation_;
};