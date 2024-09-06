/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l2/logical_channel.hpp"
#include "l2/slot.hpp"
#include "l2/upper_mac_packet.hpp"
#include "l2/upper_mac_packet_builder.hpp"
#include "prometheus.h"
#include "utils/packet_counter_metrics.hpp"
#include <memory>
#include <stdexcept>

/// The class to provide prometheus metrics to the upper mac.
/// 1. Received Slot are counted. Details about the number of Slot with CRC errors and decoding errors are saved.
/// 2. The type of upper mac packets are counted i.e., c-plane signalling, u-plane signalling, u-plane traffic and
/// broadcast
/// 3. The count and type of c-plane signalling packets, divided into non fragmented and fragmented pieces
class UpperMacMetrics {
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

    /// the class for the upper mac packet counters
    PacketCounterMetrics upper_mac_packet_metrics_;

    /// the class for the c-plane signalling packet counters
    PacketCounterMetrics c_plane_signalling_packet_metrics_;

  public:
    UpperMacMetrics() = delete;
    explicit UpperMacMetrics(const std::shared_ptr<PrometheusExporter>& prometheus_exporter)
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
              slot_error_count_family_.Add({{"logical_channel", "StealingChannel"}, {"error_type", "Decode Error"}}))
        , upper_mac_packet_metrics_(prometheus_exporter_, "upper_mac")
        , c_plane_signalling_packet_metrics_(prometheus_exporter_, "c_plane_signalling"){};

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

    /// This function is called for all decoded packets in the upper mac
    /// \param packets the datastructure that contains all the successfully decoded packets
    auto increment_packet_counters(const UpperMacPackets& packets) -> void {
        upper_mac_packet_metrics_.increment("C-Plane Signalling", packets.c_plane_signalling_packets_.size());
        upper_mac_packet_metrics_.increment("U-Plane Signalling", packets.u_plane_signalling_packet_.size());
        if (packets.u_plane_traffic_packet_) {
            upper_mac_packet_metrics_.increment("U-Plane Traffic");
        }
        if (packets.broadcast_packet_) {
            upper_mac_packet_metrics_.increment("Broadcast");
        }
    }

    /// This function is called for all c-plane packets in the upper mac
    /// \param packet the c-plane packet for which we want to increment the counters
    auto increment_c_plane_packet_counters(const UpperMacCPlaneSignallingPacket& packet) -> void {
        switch (packet.type_) {
        case MacPacketType::kMacResource:
            if (packet.is_downlink_fragment()) {
                c_plane_signalling_packet_metrics_.increment("MacResource fragments");
            } else {
                c_plane_signalling_packet_metrics_.increment("MacResource");
            }
            break;
        case MacPacketType::kMacFragmentDownlink:
            c_plane_signalling_packet_metrics_.increment("MacFragmentDownlink");
            break;
        case MacPacketType::kMacEndDownlink:
            c_plane_signalling_packet_metrics_.increment("MacEndDownlink");
            break;
        case MacPacketType::kMacDBlck:
            c_plane_signalling_packet_metrics_.increment("MacDBlck");
        case MacPacketType::kMacBroadcast:
            throw std::runtime_error("C-Plane signalling may not be of type MacBroadcast");
        case MacPacketType::kMacAccess:
            c_plane_signalling_packet_metrics_.increment("MacAccess");
            break;
        case MacPacketType::kMacEndHu:
            c_plane_signalling_packet_metrics_.increment("MacEndHu");
            break;
        case MacPacketType::kMacData:
            if (packet.is_uplink_fragment()) {
                c_plane_signalling_packet_metrics_.increment("MacData fragments");
            } else {
                c_plane_signalling_packet_metrics_.increment("MacData");
            }
            break;
        case MacPacketType::kMacFragmentUplink:
            c_plane_signalling_packet_metrics_.increment("MacFragmentUplink");
            break;
        case MacPacketType::kMacEndUplink:
            c_plane_signalling_packet_metrics_.increment("MacEndUplink");
            break;
        case MacPacketType::kMacUBlck:
            c_plane_signalling_packet_metrics_.increment("MacUBlck");
            break;
        case MacPacketType::kMacUSignal:
            throw std::runtime_error("C-Plane signalling may not be of type MacUSignal");
        }
    }
};