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
#include <thread>
#include <variant>

class BorzoiSender {
  public:
    BorzoiSender() = delete;

    /// This class sends the HTTPS Post requests to borzoi. https://github.com/tlm-solutions/borzoi
    /// \param queue the queue holds either the parsed packets (std::unique_ptr<LogicalLinkControlPacket>) or Slots that
    /// failed to decode
    /// \param termination_flag this flag is set when the sender should terminate after finishing all work
    BorzoiSender(ThreadSafeFifo<std::variant<std::unique_ptr<LogicalLinkControlPacket>, Slots>>& queue,
                 std::atomic_bool& termination_flag, unsigned borzoi_port);

    ~BorzoiSender();

  private:
    /// The thread function for continously process incomming parsed packets or failed slots.
    auto worker() -> void;

    /// The input queue
    ThreadSafeFifo<std::variant<std::unique_ptr<LogicalLinkControlPacket>, Slots>>& queue_;

    /// The flag that is set when terminating the program
    std::atomic_bool& termination_flag_;

    /// The worker thread
    std::thread worker_thread_;
};
