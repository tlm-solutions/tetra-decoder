/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l2/logical_link_control_packet.hpp"

/// The protocol that is included in this MLE packet.
enum class MobileLinkEntityProtocolDiscriminator {
    kReserved0,
    kMmProtocol,
    kCmceProtocol,
    kReserved3,
    kSndcpProtocol,
    kMleProtocol,
    kTetraManagementEntityProtocol,
    kReservedForTesting,
};

constexpr auto to_string(MobileLinkEntityProtocolDiscriminator type) noexcept -> const char* {
    switch (type) {
    case MobileLinkEntityProtocolDiscriminator::kReserved0:
        return "Reserved0";
    case MobileLinkEntityProtocolDiscriminator::kMmProtocol:
        return "MM protocol";
    case MobileLinkEntityProtocolDiscriminator::kCmceProtocol:
        return "CMCE protocol";
    case MobileLinkEntityProtocolDiscriminator::kReserved3:
        return "Reserved3";
    case MobileLinkEntityProtocolDiscriminator::kSndcpProtocol:
        return "SNDCP protocol";
    case MobileLinkEntityProtocolDiscriminator::kMleProtocol:
        return "MLE protocol";
    case MobileLinkEntityProtocolDiscriminator::kTetraManagementEntityProtocol:
        return "TETRA management entity protocol";
    case MobileLinkEntityProtocolDiscriminator::kReservedForTesting:
        return "Reserved for testing";
    }
};

/// The packet that is parsed in the logical link control layer. Currently we only implement basic link.
struct MobileLinkEntityPacket : public LogicalLinkControlPacket {
    MobileLinkEntityProtocolDiscriminator mle_protocol_;
    BitVector sdu_;

    MobileLinkEntityPacket() = default;

    explicit MobileLinkEntityPacket(const LogicalLinkControlPacket& packet);

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(MobileLinkEntityPacket, mle_protocol_, sdu_)
};

auto operator<<(std::ostream& stream, const MobileLinkEntityPacket& mle) -> std::ostream&;