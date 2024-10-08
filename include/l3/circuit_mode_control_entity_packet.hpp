/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l3/mobile_link_entity_packet.hpp"
#include "nlohmann/std_variant.hpp" // IWYU pragma: keep
#include "utils/address.hpp"
#include "utils/bit_vector.hpp"
#include "utils/type234_parser.hpp"
#include <nlohmann/json.hpp>
#include <optional>
#include <variant>

enum class CircuitModeControlEntityDownlinkPacketType {
    kDAlert,
    kDCallProceeding,
    kDConnect,
    kDConnectAcknowledge,
    kDDisconnect,
    kDInfo,
    kDRelease,
    kDSetup,
    kDStatus,
    kDTxCeased,
    kDTxContinue,
    kDTxGranted,
    kDTxWait,
    kDTxInterrupt,
    kDCallRestore,
    kDSdsData,
    kDFacility,
    kDReservered17,
    kDReservered18,
    kDReservered19,
    kDReservered20,
    kDReservered21,
    kDReservered22,
    kDReservered23,
    kDReservered24,
    kDReservered25,
    kDReservered26,
    kDReservered27,
    kDReservered28,
    kDReservered29,
    kDReservered30,
    kCmceFunctionNotSupported,
};

constexpr auto to_string(CircuitModeControlEntityDownlinkPacketType type) -> const char* {
    switch (type) {
    case CircuitModeControlEntityDownlinkPacketType::kDAlert:
        return "D-ALERT";
    case CircuitModeControlEntityDownlinkPacketType::kDCallProceeding:
        return "D-CALL-PROCEEDING";
    case CircuitModeControlEntityDownlinkPacketType::kDConnect:
        return "D-CONNECT";
    case CircuitModeControlEntityDownlinkPacketType::kDConnectAcknowledge:
        return "D-CONNECT ACKNOWLEDGE";
    case CircuitModeControlEntityDownlinkPacketType::kDDisconnect:
        return "D-DISCONNECT";
    case CircuitModeControlEntityDownlinkPacketType::kDInfo:
        return "D-INFO";
    case CircuitModeControlEntityDownlinkPacketType::kDRelease:
        return "D-RELEASE";
    case CircuitModeControlEntityDownlinkPacketType::kDSetup:
        return "D-SETUP";
    case CircuitModeControlEntityDownlinkPacketType::kDStatus:
        return "D-STATUS";
    case CircuitModeControlEntityDownlinkPacketType::kDTxCeased:
        return "D-TX CEASED";
    case CircuitModeControlEntityDownlinkPacketType::kDTxContinue:
        return "D-TX CONTINUE";
    case CircuitModeControlEntityDownlinkPacketType::kDTxGranted:
        return "D-TX GRANTED";
    case CircuitModeControlEntityDownlinkPacketType::kDTxWait:
        return "D-TX WAIT";
    case CircuitModeControlEntityDownlinkPacketType::kDTxInterrupt:
        return "D-TX INTERRUPT";
    case CircuitModeControlEntityDownlinkPacketType::kDCallRestore:
        return "D-CALL-RESTORE";
    case CircuitModeControlEntityDownlinkPacketType::kDSdsData:
        return "D-SDS-DATA";
    case CircuitModeControlEntityDownlinkPacketType::kDFacility:
        return "D-FACILITY";
    case CircuitModeControlEntityDownlinkPacketType::kDReservered17:
        return "D-Reserved17";
    case CircuitModeControlEntityDownlinkPacketType::kDReservered18:
        return "D-Reserved18";
    case CircuitModeControlEntityDownlinkPacketType::kDReservered19:
        return "D-Reserved19";
    case CircuitModeControlEntityDownlinkPacketType::kDReservered20:
        return "D-Reserved20";
    case CircuitModeControlEntityDownlinkPacketType::kDReservered21:
        return "D-Reserved21";
    case CircuitModeControlEntityDownlinkPacketType::kDReservered22:
        return "D-Reserved22";
    case CircuitModeControlEntityDownlinkPacketType::kDReservered23:
        return "D-Reserved23";
    case CircuitModeControlEntityDownlinkPacketType::kDReservered24:
        return "D-Reserved24";
    case CircuitModeControlEntityDownlinkPacketType::kDReservered25:
        return "D-Reserved25";
    case CircuitModeControlEntityDownlinkPacketType::kDReservered26:
        return "D-Reserved26";
    case CircuitModeControlEntityDownlinkPacketType::kDReservered27:
        return "D-Reserved27";
    case CircuitModeControlEntityDownlinkPacketType::kDReservered28:
        return "D-Reserved28";
    case CircuitModeControlEntityDownlinkPacketType::kDReservered29:
        return "D-Reserved29";
    case CircuitModeControlEntityDownlinkPacketType::kDReservered30:
        return "D-Reserved30";
    case CircuitModeControlEntityDownlinkPacketType::kCmceFunctionNotSupported:
        return "CMCE FUNCTION NOT SUPPORTED";
    }
};

enum class CircuitModeControlEntityUplinkPacketType {
    kUAlert,
    kUReserved1,
    kUConnect,
    kUReserved3,
    kUDisconnect,
    kUInfo,
    kURelease,
    kUSetup,
    kUStatus,
    kUTxCeased,
    kUTxDemand,
    kUReserved11,
    kUReserved12,
    kUReserved13,
    kUCallRestore,
    kUSdsData,
    kUFacility,
    kUReserved17,
    kUReserved18,
    kUReserved19,
    kUReserved20,
    kUReserved21,
    kUReserved22,
    kUReserved23,
    kUReserved24,
    kUReserved25,
    kUReserved26,
    kUReserved27,
    kUReserved28,
    kUReserved29,
    kUReserved30,
    kCmceFunctionNotSupported,
};

constexpr auto to_string(CircuitModeControlEntityUplinkPacketType type) -> const char* {
    switch (type) {
    case CircuitModeControlEntityUplinkPacketType::kUAlert:
        return "U-ALERT";
    case CircuitModeControlEntityUplinkPacketType::kUReserved1:
        return "U-Reserved1";
    case CircuitModeControlEntityUplinkPacketType::kUConnect:
        return "U-CONNECT";
    case CircuitModeControlEntityUplinkPacketType::kUReserved3:
        return "U-Reserved3";
    case CircuitModeControlEntityUplinkPacketType::kUDisconnect:
        return "U-DISCONNECT";
    case CircuitModeControlEntityUplinkPacketType::kUInfo:
        return "U-INFO";
    case CircuitModeControlEntityUplinkPacketType::kURelease:
        return "U-RELEASE";
    case CircuitModeControlEntityUplinkPacketType::kUSetup:
        return "U-SETUP";
    case CircuitModeControlEntityUplinkPacketType::kUStatus:
        return "U-STATUS";
    case CircuitModeControlEntityUplinkPacketType::kUTxCeased:
        return "U-TX CEASED";
    case CircuitModeControlEntityUplinkPacketType::kUTxDemand:
        return "U-TX DEMAND";
    case CircuitModeControlEntityUplinkPacketType::kUReserved11:
        return "U-Reserved11";
    case CircuitModeControlEntityUplinkPacketType::kUReserved12:
        return "U-Reserved12";
    case CircuitModeControlEntityUplinkPacketType::kUReserved13:
        return "U-Reserved13";
    case CircuitModeControlEntityUplinkPacketType::kUCallRestore:
        return "U-CALL-RESTORE";
    case CircuitModeControlEntityUplinkPacketType::kUSdsData:
        return "U-SDS-DATA";
    case CircuitModeControlEntityUplinkPacketType::kUFacility:
        return "U-FACILITY";
    case CircuitModeControlEntityUplinkPacketType::kUReserved17:
        return "U-Reserved17";
    case CircuitModeControlEntityUplinkPacketType::kUReserved18:
        return "U-Reserved18";
    case CircuitModeControlEntityUplinkPacketType::kUReserved19:
        return "U-Reserved19";
    case CircuitModeControlEntityUplinkPacketType::kUReserved20:
        return "U-Reserved20";
    case CircuitModeControlEntityUplinkPacketType::kUReserved21:
        return "U-Reserved21";
    case CircuitModeControlEntityUplinkPacketType::kUReserved22:
        return "U-Reserved22";
    case CircuitModeControlEntityUplinkPacketType::kUReserved23:
        return "U-Reserved23";
    case CircuitModeControlEntityUplinkPacketType::kUReserved24:
        return "U-Reserved24";
    case CircuitModeControlEntityUplinkPacketType::kUReserved25:
        return "U-Reserved25";
    case CircuitModeControlEntityUplinkPacketType::kUReserved26:
        return "U-Reserved26";
    case CircuitModeControlEntityUplinkPacketType::kUReserved27:
        return "U-Reserved27";
    case CircuitModeControlEntityUplinkPacketType::kUReserved28:
        return "U-Reserved28";
    case CircuitModeControlEntityUplinkPacketType::kUReserved29:
        return "U-Reserved29";
    case CircuitModeControlEntityUplinkPacketType::kUReserved30:
        return "U-Reserved30";
    case CircuitModeControlEntityUplinkPacketType::kCmceFunctionNotSupported:
        return "CMCE FUNCTION NOT SUPPORTED";
    }
}

using CircuitModeControlEntityPacketType =
    std::variant<CircuitModeControlEntityDownlinkPacketType, CircuitModeControlEntityUplinkPacketType>;

constexpr auto to_string(CircuitModeControlEntityPacketType type) -> const char* {
    return std::visit([](auto&& arg) { return to_string(arg); }, type);
}

enum class CircuitModeControlEntityType3ElementIdentifiers {
    kReserved,
    kDTMF,
    kExternalSubsriberNumber,
    kFacility,
    kPollResponseAddresses,
    kTemporaryAddress,
    kDmMsAddress,
    kReservedForAnyFutureSpecifiedType3Element7,
    kReservedForAnyFutureSpecifiedType3Element8,
    kReservedForAnyFutureSpecifiedType3Element9,
    kReservedForAnyFutureSpecifiedType3Element10,
    kReservedForAnyFutureSpecifiedType3Element11,
    kReservedForAnyFutureSpecifiedType3Element12,
    kReservedForAnyFutureSpecifiedType3Element13,
    kReservedForAnyFutureSpecifiedType3Element14,
    kProprietary,
};

constexpr auto to_string(CircuitModeControlEntityType3ElementIdentifiers type) -> const char* {
    switch (type) {
    case CircuitModeControlEntityType3ElementIdentifiers::kReserved:
        return "Reserved";
    case CircuitModeControlEntityType3ElementIdentifiers::kDTMF:
        return "DTMF";
    case CircuitModeControlEntityType3ElementIdentifiers::kExternalSubsriberNumber:
        return "External subscriber number";
    case CircuitModeControlEntityType3ElementIdentifiers::kFacility:
        return "Facility";
    case CircuitModeControlEntityType3ElementIdentifiers::kPollResponseAddresses:
        return "Poll response addresses";
    case CircuitModeControlEntityType3ElementIdentifiers::kTemporaryAddress:
        return "Temporary address";
    case CircuitModeControlEntityType3ElementIdentifiers::kDmMsAddress:
        return "DM-MS address";
    case CircuitModeControlEntityType3ElementIdentifiers::kReservedForAnyFutureSpecifiedType3Element7:
        return "Reserved for any future specified Type 3 element 7";
    case CircuitModeControlEntityType3ElementIdentifiers::kReservedForAnyFutureSpecifiedType3Element8:
        return "Reserved for any future specified Type 3 element 8";
    case CircuitModeControlEntityType3ElementIdentifiers::kReservedForAnyFutureSpecifiedType3Element9:
        return "Reserved for any future specified Type 3 element 9";
    case CircuitModeControlEntityType3ElementIdentifiers::kReservedForAnyFutureSpecifiedType3Element10:
        return "Reserved for any future specified Type 3 element 10";
    case CircuitModeControlEntityType3ElementIdentifiers::kReservedForAnyFutureSpecifiedType3Element11:
        return "Reserved for any future specified Type 3 element 11";
    case CircuitModeControlEntityType3ElementIdentifiers::kReservedForAnyFutureSpecifiedType3Element12:
        return "Reserved for any future specified Type 3 element 12";
    case CircuitModeControlEntityType3ElementIdentifiers::kReservedForAnyFutureSpecifiedType3Element13:
        return "Reserved for any future specified Type 3 element 13";
    case CircuitModeControlEntityType3ElementIdentifiers::kReservedForAnyFutureSpecifiedType3Element14:
        return "Reserved for any future specified Type 3 element 14";
    case CircuitModeControlEntityType3ElementIdentifiers::kProprietary:
        return "Proprietary";
    }
};

struct SdsData {
    /// the area selection that is present in the uplink sds
    std::optional<unsigned _BitInt(4)> area_selection_;
    /// the from or to address of the sds cmce packet
    Address address_;
    /// the sds data that is included int he cmce packet
    BitVector data_;
    // This map may contain the "External subscriber number" and "DM-MS address"
    Type234Parser<CircuitModeControlEntityType3ElementIdentifiers>::Map optional_elements_;

    SdsData() = default;

    static auto from_d_sds_data(BitVector& data) -> SdsData;

    static auto from_u_sds_data(BitVector& data) -> SdsData;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SdsData, area_selection_, address_, data_, optional_elements_)
};

auto operator<<(std::ostream& stream, const SdsData& sds) -> std::ostream&;

struct CircuitModeControlEntityPacket : public MobileLinkEntityPacket {
    CircuitModeControlEntityPacketType packet_type_;

    std::optional<SdsData> sds_data_;

    CircuitModeControlEntityPacket() = default;

    explicit CircuitModeControlEntityPacket(const MobileLinkEntityPacket& packet);

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CircuitModeControlEntityPacket, burst_type_, logical_channel_, type_, encrypted_,
                                   address_, fragmentation_, fragmentation_on_stealling_channel_,
                                   reservation_requirement_, tm_sdu_, encryption_mode_,
                                   immediate_napping_permission_flag_, basic_slot_granting_element_, position_of_grant_,
                                   channel_allocation_element_, random_access_flag_, power_control_element_,
                                   basic_link_information_, tl_sdu_, mle_protocol_, sdu_, packet_type_, sds_data_)
};

auto operator<<(std::ostream& stream, const CircuitModeControlEntityPacket& cmce) -> std::ostream&;