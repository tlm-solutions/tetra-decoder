/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l2/logical_link_control_packet.hpp"
#include "l3/circuit_mode_control_entity.hpp"
#include "l3/mobile_link_entity_packet.hpp"
#include "l3/mobile_management.hpp"
#include <memory>

/// The packet that is parsed in the logical link control layer. Currently we only implement basic link.
class MobileLinkEntityPacketBuilder {
  private:
    CircuitModeControlEntity cmce_;
    MobileManagement mm_;

  public:
    MobileLinkEntityPacketBuilder() = delete;

    explicit MobileLinkEntityPacketBuilder(const std::shared_ptr<PrometheusExporter>& prometheus_exporter)
        : cmce_(prometheus_exporter)
        , mm_(prometheus_exporter){};

    [[nodiscard]] auto parse_logical_link_control(const LogicalLinkControlPacket& packet)
        -> std::unique_ptr<MobileLinkEntityPacket> {
        auto mle_packet = MobileLinkEntityPacket(packet);

        // TODO: currently we only handle CMCE and MM
        switch (mle_packet.mle_protocol_) {
        case MobileLinkEntityProtocolDiscriminator::kMmProtocol:
            return mm_.process(mle_packet);
        case MobileLinkEntityProtocolDiscriminator::kCmceProtocol:
            return cmce_.process(mle_packet);

        // Fall through for all other unimplemented packet types
        case MobileLinkEntityProtocolDiscriminator::kReserved0:
        case MobileLinkEntityProtocolDiscriminator::kReserved3:
        case MobileLinkEntityProtocolDiscriminator::kSndcpProtocol:
        case MobileLinkEntityProtocolDiscriminator::kMleProtocol:
        case MobileLinkEntityProtocolDiscriminator::kTetraManagementEntityProtocol:
        case MobileLinkEntityProtocolDiscriminator::kReservedForTesting:
            return std::make_unique<MobileLinkEntityPacket>(mle_packet);
        }
    };
};