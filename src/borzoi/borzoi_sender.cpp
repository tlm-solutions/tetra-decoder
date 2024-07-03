/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "borzoi/borzoi_sender.hpp"
#include "borzoi/borzoi_converter.hpp"
#include "l3/circuit_mode_control_entity_packet.hpp"
#include "l3/mobile_link_entity_packet.hpp"
#include "l3/short_data_service_packet.hpp"
#include <cpr/body.h>
#include <cpr/cprtypes.h>
#include <cpr/payload.h>
#include <exception>
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
    if (auto* sds = dynamic_cast<ShortDataServicePacket*>(packet.get())) {
        nlohmann::json json;
        try {
            json = BorzoiConverter::to_json(sds);
            json["station"] = borzoi_uuid_;
            /// TODO: add json to post request
        } catch (std::exception& e) {
            std::cout << "Failed to convert packet to json. Error: " << e.what() << std::endl;
            return;
        }
        cpr::Response resp =
            cpr::Post(borzoi_url_sds_, cpr::Body{json.dump()}, cpr::Header{{"Content-Type", "application/json"}});

        if (resp.status_code != 200) {
            std::cout << "Failed to send packet to Borzoi: " << json.dump() << " Error: " << resp.status_code << " "
                      << resp.error.message << std::endl;
        }
    }
}

void BorzoiSender::send_failed_slots(const Slots& slots) {
    nlohmann::json json;
    try {
        json = BorzoiConverter::to_json(slots);
        json["station"] = borzoi_uuid_;
        /// TODO: add json to post request
    } catch (std::exception& e) {
        std::cout << "Failed to convert packet to json. Error: " << e.what() << std::endl;
        return;
    }
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
                    /// process the parsed packet
                    if (auto* llc = dynamic_cast<LogicalLinkControlPacket*>(arg.get())) {
                        if (llc->basic_link_information_ &&
                            (llc->basic_link_information_->basic_link_type_ == BasicLinkType::kBlAckWithoutFcs ||
                             llc->basic_link_information_->basic_link_type_ == BasicLinkType::kBlAckWithFcs)) {
                            return;
                        }
                        std::cout << *llc;
                        if (auto* mle = dynamic_cast<MobileLinkEntityPacket*>(llc)) {
                            std::cout << *mle;
                            if (auto* cmce = dynamic_cast<CircuitModeControlEntityPacket*>(llc)) {
                                std::cout << *cmce;
                                if (auto* sds = dynamic_cast<ShortDataServicePacket*>(llc)) {
                                    std::cout << *sds;
                                }
                            }
                            std::cout << std::endl;
                        }
                    }
                    send_packet(arg);
                } else if constexpr (std::is_same_v<T, Slots>) {
                    /// send out the slots which had an error while parsing
                    send_failed_slots(arg);
                }
            },
            *return_value);
    }
}