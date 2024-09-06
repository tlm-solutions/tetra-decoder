/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "l2/upper_mac.hpp"
#include "l2/logical_link_control_parser.hpp"
#include "l2/lower_mac.hpp"
#include "l2/upper_mac_fragments.hpp"
#include "streaming_ordered_output_thread_pool_executor.hpp"
#include <utility>

#if defined(__linux__)
#include <pthread.h>
#endif

UpperMac::UpperMac(const std::shared_ptr<StreamingOrderedOutputThreadPoolExecutor<LowerMac::return_type>>& input_queue,
                   ThreadSafeFifo<std::variant<std::unique_ptr<LogicalLinkControlPacket>, Slots>>& output_queue,
                   std::atomic_bool& termination_flag, std::atomic_bool& output_termination_flag,
                   const std::shared_ptr<PrometheusExporter>& prometheus_exporter)
    : input_queue_(input_queue)
    , termination_flag_(termination_flag)
    , output_termination_flag_(output_termination_flag)
    , output_queue_(output_queue)
    , logical_link_control_(prometheus_exporter) {
    if (prometheus_exporter) {
        metrics_ = std::make_unique<UpperMacMetrics>(prometheus_exporter);
        fragmentation_metrics_continous_ =
            std::make_shared<UpperMacFragmentsPrometheusCounters>(prometheus_exporter, "Continous");
        fragmentation_metrics_stealing_channel_ =
            std::make_shared<UpperMacFragmentsPrometheusCounters>(prometheus_exporter, "Stealing Channel");
    }
    fragmentation_ = std::make_unique<UpperMacFragmentation>(fragmentation_metrics_continous_);
    worker_thread_ = std::thread(&UpperMac::worker, this);

#if defined(__linux__)
    auto handle = worker_thread_.native_handle();
    pthread_setname_np(handle, "UpperMacWorker");
#endif
}

UpperMac::~UpperMac() { worker_thread_.join(); }

void UpperMac::worker() {
    for (;;) {
        const auto return_value = input_queue_->get_or_null();

        if (!return_value) {
            if (termination_flag_.load() && input_queue_->empty()) {
                break;
            }

            continue;
        }

        auto slots = *return_value;
        if (slots) {
            this->process(*slots);
        }
    }

    // forward the termination to the next stage
    output_termination_flag_.store(true);
}

auto UpperMac::process(const Slots& slots) -> void {
    const auto concreate_slots = slots.get_concreate_slots();

    UpperMacPackets packets;
    for (const auto& slot : concreate_slots) {
        // increment the total count and crc error count metrics
        if (metrics_) {
            metrics_->increment(slot);
        }

        try {
            packets.merge(UpperMacPacketBuilder::parse_slot(slot));
            // if (packets.has_user_or_control_plane_data()) {
            //     std::cout << packets << std::endl;
            // }
        } catch (std::runtime_error& e) {
            if (metrics_) {
                metrics_->increment_decode_error(slot);
            }

            continue;
        }
    }

    /// This step takes care of adding the correct adresses for some uplink packets.
    packets.apply_uplink_null_pdu_information();

    try {
        processPackets(std::move(packets));
    } catch (std::runtime_error& e) {
        if (metrics_) {
            // if there was an error decoding the packets, report the error in the slots where the packets orginated
            // from
            for (const auto& slot : concreate_slots) {
                metrics_->increment_decode_error(slot);
            }
            // send the broken slot to borzoi
            output_queue_.push_back(slots);
        }
    }
}

auto UpperMac::processPackets(UpperMacPackets&& packets) -> void {
    // the fragmentation reconstructor for over two stealing channel in the same burst
    auto& fragmentation = *fragmentation_;
    auto stealling_channel_fragmentation =
        UpperMacFragmentation(fragmentation_metrics_stealing_channel_, /*continuation_fragments_allowed=*/false);

    std::vector<UpperMacCPlaneSignallingPacket> c_plane_packets;

    for (const auto& packet : packets.c_plane_signalling_packets_) {
        // increment the packets for the mac packet type
        if (metrics_) {
            metrics_->increment_c_plane_packet_counters(packet);
        }

        if (packet.is_downlink_fragment() || packet.is_uplink_fragment()) {
            /// populate the fragmenter for stealing channel
            if (packet.fragmentation_on_stealling_channel_) {
                fragmentation = stealling_channel_fragmentation;
            }

            auto reconstructed_fragment = fragmentation.push_fragment(packet);
            if (reconstructed_fragment) {
                c_plane_packets.emplace_back(std::move(*reconstructed_fragment));
            }
        } else if (packet.tm_sdu_) {
            c_plane_packets.emplace_back(packet);
        }
    }

    /// increment the reconstruction error counter if we could not complete the fragmentation over stealing channel
    if (!stealling_channel_fragmentation.is_in_start_state() && fragmentation_metrics_stealing_channel_) {
        fragmentation_metrics_stealing_channel_->increment_fragment_reconstruction_error();
    }

    /// increment the packet counter
    if (metrics_) {
        metrics_->increment_packet_counters(packets);
    }

    for (const auto& packet : c_plane_packets) {
        auto llc = logical_link_control_.parse(packet);

        output_queue_.push_back(std::move(llc));
    }
}