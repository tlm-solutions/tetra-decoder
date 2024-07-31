/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "borzoi/borzoi_sender.hpp"
#include "borzoi/borzoi_packets.hpp"
#include "nlohmann/borzoi_send_tetra_packet.hpp"                 // IWYU pragma: keep
#include "nlohmann/borzoi_send_tetra_slots.hpp"                  // IWYU pragma: keep
#include "utils/ostream_std_unique_ptr_logical_link_control.hpp" // IWYU pragma: keep
#include <cpr/body.h>
#include <cpr/cprtypes.h>
#include <cpr/payload.h>
#include <utility>

#if defined(__linux__)
#include <pthread.h>
#endif

BorzoiSender::BorzoiSender(ThreadSafeFifo<std::variant<std::unique_ptr<LogicalLinkControlPacket>, Slots>>& queue,
                           std::atomic_bool& termination_flag, const std::string& borzoi_url, std::string borzoi_uuid)
    : queue_(queue)
    , termination_flag_(termination_flag)
    , borzoi_url_sds_(borzoi_url + "/tetra")
    , borzoi_url_failed_slots_(borzoi_url + "/tetra/failed_slots")
    , borzoi_uuid_(std::move(borzoi_uuid)) {
    worker_thread_ = std::thread(&BorzoiSender::worker, this);

#if defined(__linux__)
    auto handle = worker_thread_.native_handle();
    pthread_setname_np(handle, "BorzoiSender");
#endif
}

BorzoiSender::~BorzoiSender() { worker_thread_.join(); }

void BorzoiSender::send_packet(const std::unique_ptr<LogicalLinkControlPacket>& packet) {
    nlohmann::json json = BorzoiSendTetraPacket(packet, borzoi_uuid_);

    cpr::Response resp =
        cpr::Post(borzoi_url_sds_, cpr::Body{json.dump()}, cpr::Header{{"Content-Type", "application/json"}});

    if (resp.status_code != 200) {
        std::cout << "Failed to send packet to Borzoi: " << json.dump() << " Error: " << resp.status_code << " "
                  << resp.error.message << std::endl;
    }
}

void BorzoiSender::send_failed_slots(const Slots& slots) {
    nlohmann::json json = BorzoiSendTetraSlots(slots, borzoi_uuid_);

    cpr::Response resp =
        cpr::Post(borzoi_url_failed_slots_, cpr::Body{json.dump()}, cpr::Header{{"Content-Type", "application/json"}});

    if (resp.status_code != 200) {
        std::cout << "Failed to send packet to Borzoi: " << json.dump() << " Error: " << resp.status_code << " "
                  << resp.error.message << std::endl;
    }
}

void BorzoiSender::worker() {
    for (;;) {
        const auto return_value = queue_.get_or_null();

        if (!return_value) {
            if (termination_flag_.load() && queue_.empty()) {
                break;
            }

            continue;
        }

        std::visit(
            [this](const auto& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, std::unique_ptr<LogicalLinkControlPacket>>) {
                    send_packet(arg);

                    // Do not log acknowledgements
                    if (arg->basic_link_information_ &&
                        (arg->basic_link_information_->basic_link_type_ == BasicLinkType::kBlAckWithoutFcs ||
                         arg->basic_link_information_->basic_link_type_ == BasicLinkType::kBlAckWithFcs)) {
                        return;
                    }
                    std::cout << arg << std::endl;
                } else if constexpr (std::is_same_v<T, Slots>) {
                    /// send out the slots which had an error while parsing
                    send_failed_slots(arg);
                }
            },
            *return_value);
    }
}