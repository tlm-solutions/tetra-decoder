/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "borzoi_sender.hpp"
#include "l3/circuit_mode_control_entity_packet.hpp"
#include "l3/mobile_link_entity_packet.hpp"
#include "l3/short_data_service_packet.hpp"

#if defined(__linux__)
#include <pthread.h>
#endif

BorzoiSender::BorzoiSender(ThreadSafeFifo<std::variant<std::unique_ptr<LogicalLinkControlPacket>, Slots>>& queue,
                           std::atomic_bool& termination_flag, unsigned borzoi_port)
    : queue_(queue)
    , termination_flag_(termination_flag) {
    worker_thread_ = std::thread(&BorzoiSender::worker, this);

#if defined(__linux__)
    auto handle = worker_thread_.native_handle();
    pthread_setname_np(handle, "BorzoiSender");
#endif
}

BorzoiSender::~BorzoiSender() { worker_thread_.join(); }

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
            [](auto&& arg) {
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
                } else if constexpr (std::is_same_v<T, Slots>) {
                    /// send out the slots which had an error while parsing
                }
            },
            *return_value);
    }
}