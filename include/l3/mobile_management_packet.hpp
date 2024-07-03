/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l3/mobile_link_entity_packet.hpp"
#include <variant>

enum class MobileManagementDownlinkPacketType {
    kDOtar,
    kDAuthentication,
    kDCkChangeDemand,
    kDDisable,
    kDEnable,
    kDLocationUpdateAccept,
    kDLocationUpdateCommand,
    kDLocationUpdateReject,
    kDReserved8,
    kDLocationUpdateProceeding,
    kDAttachDetachGroupIdentity,
    kDAttachDetachGroupIdentityAck,
    kDMmStatus,
    kDReserved13,
    kDReserved14,
    kDMmPduFunctionNotSupported,
};

constexpr auto to_string(MobileManagementDownlinkPacketType type) -> const char* {
    switch (type) {
    case MobileManagementDownlinkPacketType::kDOtar:
        return "D-OTAR";
    case MobileManagementDownlinkPacketType::kDAuthentication:
        return "D-AUTHENTICATION";
    case MobileManagementDownlinkPacketType::kDCkChangeDemand:
        return "D-CK CHANGE DEMAND";
    case MobileManagementDownlinkPacketType::kDDisable:
        return "D-DISABLE";
    case MobileManagementDownlinkPacketType::kDEnable:
        return "D-ENABLE";
    case MobileManagementDownlinkPacketType::kDLocationUpdateAccept:
        return "D-LOCATION UPDATE ACCEPT";
    case MobileManagementDownlinkPacketType::kDLocationUpdateCommand:
        return "D-LOCATION UPDATE COMMAND";
    case MobileManagementDownlinkPacketType::kDLocationUpdateReject:
        return "D-LOCATION UPDATE REJECT";
    case MobileManagementDownlinkPacketType::kDReserved8:
        return "D-Reserved8";
    case MobileManagementDownlinkPacketType::kDLocationUpdateProceeding:
        return "D-LOCATION UPDATE PROCEEDING";
    case MobileManagementDownlinkPacketType::kDAttachDetachGroupIdentity:
        return "D-ATTACH/DETACH GROUP IDENTITY";
    case MobileManagementDownlinkPacketType::kDAttachDetachGroupIdentityAck:
        return "D-ATTACH/DETACH GROUP IDENTITY ACK";
    case MobileManagementDownlinkPacketType::kDMmStatus:
        return "D-MM STATUS";
    case MobileManagementDownlinkPacketType::kDReserved13:
        return "D-Reserved13";
    case MobileManagementDownlinkPacketType::kDReserved14:
        return "D-Reserved14";
    case MobileManagementDownlinkPacketType::kDMmPduFunctionNotSupported:
        return "D-MM PDU/FUNCTION NOT SUPPORTED";
    }
};

enum class MobileManagementUplinkPacketType {
    kUAuthentication,
    kUItsiDetach,
    kULocationUpdateDemand,
    kUMmStatus,
    kUCkChangeResult,
    kUOtar,
    kUInformationProvide,
    kUAttachDetachGroupIdentity,
    kUAttachDetachGroupIdentityAck,
    kUTeiProvide,
    kUReserved10,
    kUDisableStatus,
    kUReserved12,
    kUReserved13,
    kUReserved14,
    kUMmPduFunctionNotSupported,
};

constexpr auto to_string(MobileManagementUplinkPacketType type) -> const char* {
    switch (type) {
    case MobileManagementUplinkPacketType::kUAuthentication:
        return "U-AUTHENTICATION";
    case MobileManagementUplinkPacketType::kUItsiDetach:
        return "U-ITSI DETACH";
    case MobileManagementUplinkPacketType::kULocationUpdateDemand:
        return "U-LOCATION UPDATE DEMAND";
    case MobileManagementUplinkPacketType::kUMmStatus:
        return "U-MM STATUS";
    case MobileManagementUplinkPacketType::kUCkChangeResult:
        return "U-CK CHANGE RESULT";
    case MobileManagementUplinkPacketType::kUOtar:
        return "U-OTAR";
    case MobileManagementUplinkPacketType::kUInformationProvide:
        return "U-INFORMATION PROVIDE";
    case MobileManagementUplinkPacketType::kUAttachDetachGroupIdentity:
        return "U-ATTACH/DETACH GROUP IDENTITY";
    case MobileManagementUplinkPacketType::kUAttachDetachGroupIdentityAck:
        return "U-ATTACH/DETACH GROUP IDENTITY ACK";
    case MobileManagementUplinkPacketType::kUTeiProvide:
        return "U-TEI PROVIDE";
    case MobileManagementUplinkPacketType::kUReserved10:
        return "U-Reserved10";
    case MobileManagementUplinkPacketType::kUDisableStatus:
        return "U-DISABLE STATUS";
    case MobileManagementUplinkPacketType::kUReserved12:
        return "U-Reserved12";
    case MobileManagementUplinkPacketType::kUReserved13:
        return "U-Reserved13";
    case MobileManagementUplinkPacketType::kUReserved14:
        return "U-Reserved14";
    case MobileManagementUplinkPacketType::kUMmPduFunctionNotSupported:
        return "U-MM PDU/FUNCTION NOT SUPPORTED";
    }
};

using MobileManagementPacketType = std::variant<MobileManagementDownlinkPacketType, MobileManagementUplinkPacketType>;

constexpr auto to_string(MobileManagementPacketType type) -> const char* {
    return std::visit([](auto&& arg) { return to_string(arg); }, type);
}

struct MobileManagementPacket : public MobileLinkEntityPacket {
    MobileManagementPacketType packet_type_;

    MobileManagementPacket() = delete;

    explicit MobileManagementPacket(const MobileLinkEntityPacket& packet);
};