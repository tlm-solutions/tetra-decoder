/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#pragma once

#include "l2/upper_mac_fragments.hpp"
#include "l2/upper_mac_packet_builder.hpp"
#include <l2/logical_link_control.hpp>
#include <l3/mobile_link_entity.hpp>
#include <reporter.hpp>

class UpperMac {
  public:
    UpperMac() = delete;
    UpperMac(std::shared_ptr<Reporter> reporter, bool is_downlink)
        : reporter_(std::move(reporter))
        , mobile_link_entity_(std::make_shared<MobileLinkEntity>(reporter_, is_downlink))
        , logical_link_control_(std::make_unique<LogicalLinkControl>(reporter_, mobile_link_entity_)){};
    ~UpperMac() noexcept = default;

    /// process Upper MAC packets and perform fragment reconstruction and pass it to the upper layers
    /// \param packets the packets that were parsed in the upper MAC layer
    auto process(UpperMacPackets&& packets) -> void {
        for (const auto& packet : packets.c_plane_signalling_packets_) {
            // TODO: handle fragmentation over STCH
            if (packet.is_downlink_fragment() || packet.is_uplink_fragment()) {
                auto reconstructed_fragment = fragmentation_.push_fragment(packet);
                if (reconstructed_fragment) {
                    auto data = BitVector(*reconstructed_fragment->tm_sdu_);
                    logical_link_control_->process(reconstructed_fragment->address_, data);
                }
            } else if (packet.tm_sdu_) {
                auto data = BitVector(*packet.tm_sdu_);
                logical_link_control_->process(packet.address_, data);
            }
        }
    };

  private:
    std::shared_ptr<Reporter> reporter_;
    std::shared_ptr<MobileLinkEntity> mobile_link_entity_;
    std::unique_ptr<LogicalLinkControl> logical_link_control_;

    UpperMacFragmentation fragmentation_;
};