/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l3/mobile_link_entity_packet.hpp"
#include "utils/bit_vector.hpp"
#include "utils/type234_parser.hpp"
#include <bitset>
#include <optional>
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

enum class LocationUpdateType {
    kRoamingLocationUpdating,
    kMigrationLocationUpdating,
    kPeriodicLocationUpdating,
    kItsiAttach,
    kServiceRestorationRoamingLocationUpdating,
    kServiceRestorationMigratingLocationUpdating,
    kDemandLocationUpdating,
    kDisableMsUpdating,
};

constexpr auto to_string(LocationUpdateType type) -> const char* {
    switch (type) {
    case LocationUpdateType::kRoamingLocationUpdating:
        return "Roaming location updating";
    case LocationUpdateType::kMigrationLocationUpdating:
        return "Migrating location updating";
    case LocationUpdateType::kPeriodicLocationUpdating:
        return "Periodic location updating";
    case LocationUpdateType::kItsiAttach:
        return "ITSI attach";
    case LocationUpdateType::kServiceRestorationRoamingLocationUpdating:
        return "Service restoration roaming location updating";
    case LocationUpdateType::kServiceRestorationMigratingLocationUpdating:
        return "Service restoration migrating location updating";
    case LocationUpdateType::kDemandLocationUpdating:
        return "Demand location updating";
    case LocationUpdateType::kDisableMsUpdating:
        return "Disabled MS updating";
    }
};

enum class LocationUpdateAcceptType {
    kRoamingLocationUpdating,
    kTemporaryRegistration,
    kPeriodicLocationUpdating,
    kItsiAttach,
    kServiceRestorationRoamingLocationUpdating,
    kMigratingOrServiceRestorationMigratingLocationUpdating,
    kDemandLocationUpdating,
    kDisableMsUpdating,
};

constexpr auto to_string(LocationUpdateAcceptType type) -> const char* {
    switch (type) {
    case LocationUpdateAcceptType::kRoamingLocationUpdating:
        return "Roaming location updating";
    case LocationUpdateAcceptType::kTemporaryRegistration:
        return "Temporary registration";
    case LocationUpdateAcceptType::kPeriodicLocationUpdating:
        return "Periodic location updating";
    case LocationUpdateAcceptType::kItsiAttach:
        return "ITSI attach";
    case LocationUpdateAcceptType::kServiceRestorationRoamingLocationUpdating:
        return "Service restoration roaming location updating";
    case LocationUpdateAcceptType::kMigratingOrServiceRestorationMigratingLocationUpdating:
        return "Migrating or service restoration migrating location updating";
    case LocationUpdateAcceptType::kDemandLocationUpdating:
        return "Demand location updating";
    case LocationUpdateAcceptType::kDisableMsUpdating:
        return "Disabled MS updating";
    }
};

enum class MobileManagementDownlinkType34ElementIdentifiers {
    kReservedForFutureExtension,
    kDefaultGroupAttachLifetime,
    kNewRegisteredArea,
    kSecurityDownlink,
    kGroupReportResponse,
    kGroupIdentityLocationAccept,
    kDmMsAddress,
    kGroupIdentityDownlink,
    kReserved8,
    kReserved9,
    kAuthenticationDownlink,
    kReserved11,
    kGroupIdentitySecurityRelatedInformation,
    kCellTypeControl,
    kReserved14,
    kProprietary,
};

constexpr auto to_string(MobileManagementDownlinkType34ElementIdentifiers type) -> const char* {
    switch (type) {
    case MobileManagementDownlinkType34ElementIdentifiers::kReservedForFutureExtension:
        return "Reserved for future extension";
    case MobileManagementDownlinkType34ElementIdentifiers::kDefaultGroupAttachLifetime:
        return "Default group attachment lifetime";
    case MobileManagementDownlinkType34ElementIdentifiers::kNewRegisteredArea:
        return "New registered area";
    case MobileManagementDownlinkType34ElementIdentifiers::kSecurityDownlink:
        return "Security downlink, see ETSI EN 300 392-7";
    case MobileManagementDownlinkType34ElementIdentifiers::kGroupReportResponse:
        return "Group report response";
    case MobileManagementDownlinkType34ElementIdentifiers::kGroupIdentityLocationAccept:
        return "Group identity location accept";
    case MobileManagementDownlinkType34ElementIdentifiers::kDmMsAddress:
        return "DM-MS address, see ETSI EN 300 396-5";
    case MobileManagementDownlinkType34ElementIdentifiers::kGroupIdentityDownlink:
        return "Group identity downlink";
    case MobileManagementDownlinkType34ElementIdentifiers::kReserved8:
        return "Reserved 8 for any future specified Type 3/4 element";
    case MobileManagementDownlinkType34ElementIdentifiers::kReserved9:
        return "Reserved 9 for any future specified Type 3/4 element";
    case MobileManagementDownlinkType34ElementIdentifiers::kAuthenticationDownlink:
        return "Authentication downlink, see ETSI EN 300 392-7";
    case MobileManagementDownlinkType34ElementIdentifiers::kReserved11:
        return "Reserved 11 for any future specified Type 3/4 element";
    case MobileManagementDownlinkType34ElementIdentifiers::kGroupIdentitySecurityRelatedInformation:
        return "Group Identity Security Related Information, see ETSI EN 300 392-7";
    case MobileManagementDownlinkType34ElementIdentifiers::kCellTypeControl:
        return "Cell type control";
    case MobileManagementDownlinkType34ElementIdentifiers::kReserved14:
        return "Reserved 14 for any future specified Type 3/4 element";
    case MobileManagementDownlinkType34ElementIdentifiers::kProprietary:
        return "Proprietary";
    }
};

enum class GroupIdentityAcceptReject {
    kAllAttachmentDetachmentsAccepted,
    kAtLeastOneAttachmentDetachmentRejected,
};

constexpr auto to_string(GroupIdentityAcceptReject type) -> const char* {
    switch (type) {
    case GroupIdentityAcceptReject::kAllAttachmentDetachmentsAccepted:
        return "All attachment/detachments accepted";
    case GroupIdentityAcceptReject::kAtLeastOneAttachmentDetachmentRejected:
        return "At least one attachment/detachment rejected";
    }
};

struct MobileManagementDownlinkAttachDetachGroupIdentityAcknowledgement {
    GroupIdentityAcceptReject group_identity_accept_reject_;
    Type234Parser<MobileManagementDownlinkType34ElementIdentifiers>::Map optional_elements_;

    MobileManagementDownlinkAttachDetachGroupIdentityAcknowledgement() = delete;

    explicit MobileManagementDownlinkAttachDetachGroupIdentityAcknowledgement(BitVector&);
};

auto operator<<(std::ostream& stream, const MobileManagementDownlinkAttachDetachGroupIdentityAcknowledgement& packet)
    -> std::ostream&;

struct MobileManagementDownlinkLocationUpdateAccept {
    LocationUpdateAcceptType location_update_accept_type_;
    Address address_;
    std::optional<unsigned _BitInt(16)> subscriber_class_;
    std::optional<unsigned _BitInt(14)> energy_saving_information_;
    std::optional<unsigned _BitInt(4)> scch_information_;
    std::optional<unsigned _BitInt(2)> distribution_on_18th_frame_;

    Type234Parser<MobileManagementDownlinkType34ElementIdentifiers>::Map optional_elements_;

    MobileManagementDownlinkLocationUpdateAccept() = delete;

    explicit MobileManagementDownlinkLocationUpdateAccept(BitVector&);
};

auto operator<<(std::ostream& stream, const MobileManagementDownlinkLocationUpdateAccept& packet) -> std::ostream&;

struct MobileManagementPacket : public MobileLinkEntityPacket {
    MobileManagementPacketType packet_type_;
    std::optional<MobileManagementDownlinkLocationUpdateAccept> downlink_location_update_accept_;
    std::optional<MobileManagementDownlinkAttachDetachGroupIdentityAcknowledgement>
        downlink_attach_detach_group_identity_acknowledgement_;

    MobileManagementPacket() = delete;

    explicit MobileManagementPacket(const MobileLinkEntityPacket& packet);
};

auto operator<<(std::ostream& stream, const MobileManagementPacket& packet) -> std::ostream&;