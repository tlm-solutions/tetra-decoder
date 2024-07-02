/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l2/logical_link_control_packet.hpp"
#include "l2/slot.hpp"
#include "thread_safe_fifo.hpp"
#include <atomic>
#include <cpr/cpr.h>
#include <thread>
#include <variant>

class BorzoiSender {
  public:
    BorzoiSender() = delete;

    /// This class sends the HTTP Post requests to borzoi. https://github.com/tlm-solutions/borzoi
    /// \param queue the queue holds either the parsed packets (std::unique_ptr<LogicalLinkControlPacket>) or Slots that
    /// failed to decode
    /// \param termination_flag this flag is set when the sender should terminate after finishing all work
    /// \param borzoi_url the URL of borzoi
    /// \param borzoi_uuid the station UUID of this instance of tetra-decoder sending to borzoi
    BorzoiSender(ThreadSafeFifo<std::variant<std::unique_ptr<LogicalLinkControlPacket>, Slots>>& queue,
                 std::atomic_bool& termination_flag, const std::string& borzoi_url, std::string borzoi_uuid);

    ~BorzoiSender();

  private:
    /// The thread function for continously process incomming parsed packets or failed slots.
    auto worker() -> void;

    void send_packet(const std::unique_ptr<LogicalLinkControlPacket>& packet);
    void send_failed_slots(const Slots& slots);

    /// The input queue
    ThreadSafeFifo<std::variant<std::unique_ptr<LogicalLinkControlPacket>, Slots>>& queue_;

    /// The flag that is set when terminating the program
    std::atomic_bool& termination_flag_;

    /// The urls of borzoi
    cpr::Url borzoi_url_sds_;
    cpr::Url borzoi_url_failed_slots_;

    /// The Station UUID of borzoi
    std::string borzoi_uuid_;

    /// The worker thread
    std::thread worker_thread_;
};
