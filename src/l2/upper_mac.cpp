/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "l2/upper_mac.hpp"
#include "l2/lower_mac.hpp"
#include "l2/upper_mac_fragments.hpp"
#include "l2/upper_mac_packet.hpp"
#include "streaming_ordered_output_thread_pool_executor.hpp"
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#if defined(__linux__)
#include <pthread.h>
#endif

UpperMac::UpperMac(const std::shared_ptr<StreamingOrderedOutputThreadPoolExecutor<LowerMac::return_type>>& input_queue,
                   const std::shared_ptr<PrometheusExporter>& prometheus_exporter,
                   const std::shared_ptr<Reporter>& reporter, bool is_downlink)
    : input_queue_(input_queue)
    , logical_link_control_(
          std::make_unique<LogicalLinkControl>(reporter, std::make_shared<MobileLinkEntity>(reporter, is_downlink))) {
    if (prometheus_exporter) {
        metrics_ = std::make_unique<UpperMacPrometheusCounters>(prometheus_exporter);
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
        const auto return_value = input_queue_->get();

        if (std::holds_alternative<TerminationToken>(return_value)) {
            return;
        }

        const auto slots = std::get<LowerMac::return_type>(return_value);
        if (slots) {
            this->process(*slots);
        }
    }
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

    try {
        processPackets(std::move(packets));
    } catch (std::runtime_error& e) {
        // TODO: increment other metrics
    }
}

auto UpperMac::processPackets(UpperMacPackets&& packets) -> void {
    // the fragmentation reconstructor for over two stealing channel in the same burst
    auto& fragmentation = *fragmentation_;
    auto stealling_channel_fragmentation =
        UpperMacFragmentation(fragmentation_metrics_stealing_channel_, /*continuation_fragments_allowed=*/false);

    std::vector<UpperMacCPlaneSignallingPacket> c_plane_packets;

    for (const auto& packet : packets.c_plane_signalling_packets_) {
        if (packet.is_downlink_fragment() || packet.is_uplink_fragment()) {
            /// populate the fragmenter for stealing channel
            if (packet.fragmentation_on_stealling_channel_) {
                fragmentation = stealling_channel_fragmentation;
            }

            auto reconstructed_fragment = fragmentation.push_fragment(packet);
            if (reconstructed_fragment) {
                c_plane_packets.emplace_back(std::move(reconstructed_fragment));
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
        auto data = BitVector(*packet.tm_sdu_);
        logical_link_control_->process(packet.address_, data);
    }
}