/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l2/logical_link_control_packet.hpp"
#include "l2/upper_mac_packet.hpp"
#include "l3/mobile_link_entity.hpp"
#include <memory>
#include <optional>

/// The packet that is parsed in the logical link control layer. Currently we only implement basic link.
class LogicalLinkControlPacketBuilder {
  private:
    MobileLinkEntity mle_;

  public:
    LogicalLinkControlPacketBuilder() = delete;

    explicit LogicalLinkControlPacketBuilder(const std::shared_ptr<PrometheusExporter>& prometheus_exporter,
                                             Reporter&& reporter, bool is_downlink)
        : mle_(prometheus_exporter, std::move(reporter), is_downlink){};

    [[nodiscard]] auto parse_c_plane_signalling(const UpperMacCPlaneSignallingPacket& packet)
        -> std::unique_ptr<UpperMacCPlaneSignallingPacket> {
        auto pdu_type = packet.tm_sdu_->look<4>(0);

        /// We only implemented packet parsing for Basic Link PDUs at this point in time
        if (pdu_type <= 0b1000) {
            auto llc_packet = LogicalLinkControlPacket(packet);

            /// Contains optional TL-SDU pass to MLE for further parsing.
            if (llc_packet.tl_sdu_.bits_left() > 0) {
                return mle_.process(llc_packet);
            }

            return std::make_unique<LogicalLinkControlPacket>(llc_packet);
        }

        return std::make_unique<UpperMacCPlaneSignallingPacket>(packet);
    };
};