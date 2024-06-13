/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l2/upper_mac_packet.hpp"
#include <cassert>
#include <optional>
#include <stdexcept>
#include <vector>

/// hold the fragments of fragmented messages
struct UpperMacFragments {
    /// the start fragment
    std::optional<UpperMacCPlaneSignallingPacket> start_fragment_;
    /// the optional continuation fragments
    std::vector<UpperMacCPlaneSignallingPacket> continuation_fragments_;
    /// the end fragment
    std::optional<UpperMacCPlaneSignallingPacket> end_fragment_;
};

/// Class that provides the fragment reconstruction for uplink and downlink packets.
/// TODO: Uplink fragmentation may include reserved slots and is therefore harder to reconstruct. This is not handled
/// with this class.
/// TODO: Fragmentation over two slots of a stealing channel is also not handled.
class UpperMacFragmentation {
  private:
    /// the fragments for the downlink
    UpperMacFragments downlink_fragments_;
    /// the fragments for the uplink
    UpperMacFragments uplink_fragments_;

  public:
    UpperMacFragmentation() = default;

    /// Push a fragment for reconstruction.
    /// \param fragment the control plane signalling packet that is fragmented
    /// \return an optional reconstructed control plane signalling packet when reconstuction was successful
    auto push_fragment(const UpperMacCPlaneSignallingPacket& fragment)
        -> std::optional<UpperMacCPlaneSignallingPacket> {
        switch (fragment.type_) {
        case MacPacketType::kMacResource:
            assert(fragment.fragmentation_);
            downlink_fragments_ = UpperMacFragments{.start_fragment_ = fragment};
            break;
        case MacPacketType::kMacFragmentDownlink:
            if (downlink_fragments_.start_fragment_) {
                downlink_fragments_.continuation_fragments_.push_back(fragment);
            }
            break;
        case MacPacketType::kMacEndDownlink:
            if (downlink_fragments_.start_fragment_) {
                downlink_fragments_.end_fragment_ = fragment;
            }
            break;
        case MacPacketType::kMacDBlck:
            throw std::runtime_error("No fragmentation in MacDBlck");
        case MacPacketType::kMacBroadcast:
            throw std::runtime_error("No fragmentation in MacBroadcast");
        case MacPacketType::kMacAccess:
        case MacPacketType::kMacData:
            assert(fragment.fragmentation_);
            uplink_fragments_ = UpperMacFragments{.start_fragment_ = fragment};
            break;
        case MacPacketType::kMacFragmentUplink:
            if (uplink_fragments_.start_fragment_) {
                uplink_fragments_.continuation_fragments_.push_back(fragment);
            }
            break;
        case MacPacketType::kMacEndHu:
        case MacPacketType::kMacEndUplink:
            if (uplink_fragments_.start_fragment_) {
                uplink_fragments_.end_fragment_ = fragment;
            }
            break;
        case MacPacketType::kMacUBlck:
            throw std::runtime_error("No fragmentation in MacUBlck");
        case MacPacketType::kMacUSignal:
            throw std::runtime_error("No fragmentation in MacUSignal");
        }

        // forward and clear on MacEndDownlink
        if (downlink_fragments_.end_fragment_) {
            UpperMacCPlaneSignallingPacket packet = *downlink_fragments_.start_fragment_;

            for (const auto& fragment : downlink_fragments_.continuation_fragments_) {
                if (fragment.tm_sdu_) {
                    packet.tm_sdu_->append(*fragment.tm_sdu_);
                }
            }
            if (downlink_fragments_.end_fragment_->tm_sdu_) {
                packet.tm_sdu_->append(*downlink_fragments_.end_fragment_->tm_sdu_);
            }

            downlink_fragments_ = UpperMacFragments{};

            return packet;
        }

        // forward and clear on MacEndHu and MacEndUplink
        if (uplink_fragments_.end_fragment_) {
            UpperMacCPlaneSignallingPacket packet = *uplink_fragments_.start_fragment_;

            for (const auto& fragment : uplink_fragments_.continuation_fragments_) {
                if (fragment.tm_sdu_) {
                    packet.tm_sdu_->append(*fragment.tm_sdu_);
                }
            }
            if (uplink_fragments_.end_fragment_->tm_sdu_) {
                packet.tm_sdu_->append(*uplink_fragments_.end_fragment_->tm_sdu_);
            }

            uplink_fragments_ = UpperMacFragments{};

            return packet;
        }

        return std::nullopt;
    };
};