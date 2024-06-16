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

    /// The family of counters for all received upper mac packets
    prometheus::Family<prometheus::Counter>& packet_count_family_;
    /// The counters for all received c-plane signalling packets
    prometheus::Counter& c_plane_signalling_packet_count_;
    /// The counters for all received u-plane signalling packets
    prometheus::Counter& u_plane_signalling_packet_count_;
    /// The counters for all received u-plane traffic packets
    prometheus::Counter& u_plane_traffic_packet_count_;
    /// The counters for all received broadcast packets
    prometheus::Counter& broadcast_packet_count_;

    /// The family of counters for all received upper mac c-plane packets
    prometheus::Family<prometheus::Counter>& c_plane_packet_count_family_;
    /// The counter for all received upper mac c-plane MacResource
    prometheus::Counter& c_plane_packet_count_mac_resource_;
    /// The counter for all received upper mac c-plane MacResource for fragments
    prometheus::Counter& c_plane_packet_count_mac_resource_fragments_;
    /// The counter for all received upper mac c-plane MacFragmentDownlink
    prometheus::Counter& c_plane_packet_count_mac_fragment_downlink_;
    /// The counter for all received upper mac c-plane MacEndDownlink
    prometheus::Counter& c_plane_packet_count_mac_end_downlink_;
    /// The counter for all received upper mac c-plane MacDBlck
    prometheus::Counter& c_plane_packet_count_mac_d_blck_;
    /// The counter for all received upper mac c-plane MacAccess
    prometheus::Counter& c_plane_packet_count_mac_access_;
    /// The counter for all received upper mac c-plane MacEndHu
    prometheus::Counter& c_plane_packet_count_mac_end_hu_;
    /// The counter for all received upper mac c-plane MacData
    prometheus::Counter& c_plane_packet_count_mac_data_;
    /// The counter for all received upper mac c-plane MacData for fragments
    prometheus::Counter& c_plane_packet_count_mac_data_fragments_;
    /// The counter for all received upper mac c-plane MacFragmentUplink
    prometheus::Counter& c_plane_packet_count_mac_fragment_uplink_;
    /// The counter for all received upper mac c-plane MacEndUplink
    prometheus::Counter& c_plane_packet_count_mac_end_uplink_;
    /// The counter for all received upper mac c-plane MacUBlck
    prometheus::Counter& c_plane_packet_count_mac_u_blck_;

    // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)

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
        , packet_count_family_(prometheus_exporter_->upper_mac_packet_count())
        , c_plane_signalling_packet_count_(packet_count_family_.Add({{"packet_type", "C-Plane Signalling"}}))
        , u_plane_signalling_packet_count_(packet_count_family_.Add({{"packet_type", "U-Plane Signalling"}}))
        , u_plane_traffic_packet_count_(packet_count_family_.Add({{"packet_type", "U-Plane Traffic"}}))
        , broadcast_packet_count_(packet_count_family_.Add({{"packet_type", "Broadcast"}}))
        , c_plane_packet_count_family_(prometheus_exporter_->c_plane_packet_count())
        , c_plane_packet_count_mac_resource_(c_plane_packet_count_family_.Add({{"packet_type", "MacResource"}}))
        , c_plane_packet_count_mac_resource_fragments_(
              c_plane_packet_count_family_.Add({{"packet_type", "MacResource fragments"}}))
        , c_plane_packet_count_mac_fragment_downlink_(
              c_plane_packet_count_family_.Add({{"packet_type", "MacFragmentDownlink"}}))
        , c_plane_packet_count_mac_end_downlink_(c_plane_packet_count_family_.Add({{"packet_type", "MacEnd"}}))
        , c_plane_packet_count_mac_d_blck_(c_plane_packet_count_family_.Add({{"packet_type", "MacDBlck"}}))
        , c_plane_packet_count_mac_access_(c_plane_packet_count_family_.Add({{"packet_type", "MacAccess"}}))
        , c_plane_packet_count_mac_end_hu_(c_plane_packet_count_family_.Add({{"packet_type", "MacEndHu"}}))
        , c_plane_packet_count_mac_data_(c_plane_packet_count_family_.Add({{"packet_type", "MacData"}}))
        , c_plane_packet_count_mac_data_fragments_(
              c_plane_packet_count_family_.Add({{"packet_type", "MacData fragments"}}))
        , c_plane_packet_count_mac_fragment_uplink_(
              c_plane_packet_count_family_.Add({{"packet_type", "MacFragmentUplink"}}))
        , c_plane_packet_count_mac_end_uplink_(c_plane_packet_count_family_.Add({{"packet_type", "MacEndUplink"}}))
        , c_plane_packet_count_mac_u_blck_(c_plane_packet_count_family_.Add({{"packet_type", "MacUBlck"}})){};

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
        c_plane_signalling_packet_count_.Increment(static_cast<double>(packets.c_plane_signalling_packets_.size()));
        u_plane_signalling_packet_count_.Increment(static_cast<double>(packets.u_plane_signalling_packet_.size()));
        if (packets.u_plane_traffic_packet_) {
            u_plane_traffic_packet_count_.Increment();
        }
        if (packets.broadcast_packet_) {
            broadcast_packet_count_.Increment();
        }
    }
    /// This function is called for all c-plane packets in the upper mac
    /// \param packet the c-plane packet for which we want to increment the counters
    auto increment_c_plane_packet_counters(const UpperMacCPlaneSignallingPacket& packet) -> void {
        switch (packet.type_) {
        case MacPacketType::kMacResource:
            if (packet.is_downlink_fragment()) {
                c_plane_packet_count_mac_resource_fragments_.Increment();
            } else {
                c_plane_packet_count_mac_resource_.Increment();
            }
            break;
        case MacPacketType::kMacFragmentDownlink:
            c_plane_packet_count_mac_fragment_downlink_.Increment();
            break;
        case MacPacketType::kMacEndDownlink:
            c_plane_packet_count_mac_end_downlink_.Increment();
            break;
        case MacPacketType::kMacDBlck:
            c_plane_packet_count_mac_d_blck_.Increment();
        case MacPacketType::kMacBroadcast:
            throw std::runtime_error("C-Plane signalling may not be of type MacBroadcast");
        case MacPacketType::kMacAccess:
            c_plane_packet_count_mac_access_.Increment();
            break;
        case MacPacketType::kMacEndHu:
            c_plane_packet_count_mac_end_hu_.Increment();
            break;
        case MacPacketType::kMacData:
            if (packet.is_uplink_fragment()) {
                c_plane_packet_count_mac_data_fragments_.Increment();
            } else {
                c_plane_packet_count_mac_data_.Increment();
            }
            break;
        case MacPacketType::kMacFragmentUplink:
            c_plane_packet_count_mac_fragment_uplink_.Increment();
            break;
        case MacPacketType::kMacEndUplink:
            c_plane_packet_count_mac_end_uplink_.Increment();
        case MacPacketType::kMacUBlck:
            c_plane_packet_count_mac_u_blck_.Increment();
        case MacPacketType::kMacUSignal:
            throw std::runtime_error("C-Plane signalling may not be of type MacUSignal");
        }
    }
};