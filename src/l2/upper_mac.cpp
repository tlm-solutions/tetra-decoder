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
#include "streaming_ordered_output_thread_pool_executor.hpp"
#include <optional>
#include <utility>
#include <variant>

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
    }
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
        // TODO: Implement a way to send a stop signal until termination
    }
}

auto UpperMac::process(const Slots& slots) -> void {
    const auto concreate_slots = slots.get_concreate_slots();

    /// Use this fragmentation reconstructor for the two stealing channels
    std::optional<UpperMacFragmentation> stealing_channel_fragmentation;
    for (const auto& slot : concreate_slots) {
        UpperMacPackets packets;

        // increment the total count and crc error count metrics
        if (metrics_) {
            metrics_->increment(slot);
        }

        try {
            packets = UpperMacPacketBuilder::parse_slot(slot);
            // if (packets.has_user_or_control_plane_data()) {
            //     std::cout << packets << std::endl;
            // }
        } catch (std::runtime_error& e) {
            if (metrics_) {
                metrics_->increment_decode_error(slot);
            }

            return;
        }

        try {
            processPackets(std::move(packets), stealing_channel_fragmentation);
        } catch (std::runtime_error& e) {
            if (metrics_) {
                metrics_->increment_decode_error(slot);
            }
        }
    }
}

auto UpperMac::processPackets(UpperMacPackets&& packets,
                              std::optional<UpperMacFragmentation>& stealling_channel_fragmentation) -> void {
    for (const auto& packet : packets.c_plane_signalling_packets_) {
        if (packet.is_downlink_fragment() || packet.is_uplink_fragment()) {
            /// populate the fragmenter for stealing channel
            if (packet.fragmentation_on_stealling_channel_ && !stealling_channel_fragmentation) {
                stealling_channel_fragmentation = UpperMacFragmentation{};
            }

            // select the correct fragment reconstructor
            auto& fragmentation = fragmentation_;
            if (stealling_channel_fragmentation) {
                fragmentation = *stealling_channel_fragmentation;
            }

            auto reconstructed_fragment = fragmentation.push_fragment(packet);
            if (reconstructed_fragment) {
                auto data = BitVector(*reconstructed_fragment->tm_sdu_);
                logical_link_control_->process(reconstructed_fragment->address_, data);
            }
        } else if (packet.tm_sdu_) {
            auto data = BitVector(*packet.tm_sdu_);
            logical_link_control_->process(packet.address_, data);
        }
    }
}