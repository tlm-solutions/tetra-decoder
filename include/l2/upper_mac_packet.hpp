/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l2/logical_channel.hpp"
#include "utils/address_type.hpp"
#include "utils/bit_vector.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>

enum class MacPacketType {
    // downlink c-plane
    kMacResource,
    kMacFragmentDownlink,
    kMacEndDownlink,
    kMacDBlck,
    kMacBroadcast,

    // uplink c-plane (SCH/HU)
    kMacAccess,
    kMacEndHu,

    // uplink c-plane
    kMacData,
    kMacFragmentUplink,
    kMacEndUplink,
    kMacUBlck,

    // (uplink and downlink) u-plane signalling
    kMacUSignal,
};

constexpr auto to_string(MacPacketType type) noexcept -> const char* {
    switch (type) {
    case MacPacketType::kMacAccess:
        return "MacAccess";
    case MacPacketType::kMacResource:
        return "MacResource";
    case MacPacketType::kMacFragmentDownlink:
        return "MacFragmentDownlink";
    case MacPacketType::kMacEndDownlink:
        return "MacEndDownlink";
    case MacPacketType::kMacDBlck:
        return "MacDBlck";
    case MacPacketType::kMacEndHu:
        return "MacEndHu";
    case MacPacketType::kMacData:
        return "MacData";
    case MacPacketType::kMacFragmentUplink:
        return "MacFragmentUplink";
    case MacPacketType::kMacEndUplink:
        return "MacEndUplink";
    case MacPacketType::kMacUBlck:
        return "MacUBlck";
    case MacPacketType::kMacUSignal:
        return "MacUSignal";
    case MacPacketType::kMacBroadcast:
        return "MacBroadcast";
    }
};

struct AccessCodeDefinition {
    unsigned _BitInt(4) immediate_;
    unsigned _BitInt(4) waiting_time_;
    unsigned _BitInt(4) number_of_random_access_transmissions_on_up_link_;
    unsigned _BitInt(1) frame_length_factor_;
    unsigned _BitInt(4) timeslot_pointer_;
    unsigned _BitInt(3) minimum_pdu_priority_;

    AccessCodeDefinition() = delete;
    explicit AccessCodeDefinition(BitVector& data);

    friend auto operator<<(std::ostream& stream, const AccessCodeDefinition& element) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const AccessCodeDefinition& element) -> std::ostream&;

struct ExtendedServiceBroadcastSection1 {
    unsigned _BitInt(1) data_priority_supported_;
    unsigned _BitInt(1) extended_advanced_links_and_max_ublck_supported_;
    unsigned _BitInt(1) qos_negotiation_supported_;
    unsigned _BitInt(1) d8psk_service_;
    unsigned _BitInt(1) section2_sent_;
    unsigned _BitInt(1) section3_sent_;
    unsigned _BitInt(1) section4_sent_;

    ExtendedServiceBroadcastSection1() = delete;
    explicit ExtendedServiceBroadcastSection1(BitVector& data);

    friend auto operator<<(std::ostream& stream, const ExtendedServiceBroadcastSection1& element) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const ExtendedServiceBroadcastSection1& element) -> std::ostream&;

struct ExtendedServiceBroadcastSection2 {
    unsigned _BitInt(1) service_25Qam_;
    unsigned _BitInt(1) service_50Qam_;
    unsigned _BitInt(1) service_100Qam_;
    unsigned _BitInt(1) service_150Qam_;
    unsigned _BitInt(3) reserved_;

    ExtendedServiceBroadcastSection2() = delete;
    explicit ExtendedServiceBroadcastSection2(BitVector& data);

    friend auto operator<<(std::ostream& stream, const ExtendedServiceBroadcastSection2& element) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const ExtendedServiceBroadcastSection2& element) -> std::ostream&;

struct ExtendedServiceBroadcastSection3 {
    unsigned _BitInt(7) reserved_;

    ExtendedServiceBroadcastSection3() = delete;
    explicit ExtendedServiceBroadcastSection3(BitVector& data);

    friend auto operator<<(std::ostream& stream, const ExtendedServiceBroadcastSection3& element) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const ExtendedServiceBroadcastSection3& element) -> std::ostream&;

struct ExtendedServiceBroadcastSection4 {
    unsigned _BitInt(7) reserved_;

    ExtendedServiceBroadcastSection4() = delete;
    explicit ExtendedServiceBroadcastSection4(BitVector& data);

    friend auto operator<<(std::ostream& stream, const ExtendedServiceBroadcastSection4& element) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const ExtendedServiceBroadcastSection4& element) -> std::ostream&;

struct ExtendedServiceBroadcast {
    unsigned _BitInt(8) security_information_;
    unsigned _BitInt(2) sdstl_addressing_method_;
    unsigned _BitInt(1) gck_supported_;

    std::optional<ExtendedServiceBroadcastSection1> section1_;
    std::optional<ExtendedServiceBroadcastSection2> section2_;
    std::optional<ExtendedServiceBroadcastSection3> section3_;
    std::optional<ExtendedServiceBroadcastSection4> section4_;

    ExtendedServiceBroadcast() = delete;
    explicit ExtendedServiceBroadcast(BitVector& data);

    friend auto operator<<(std::ostream& stream, const ExtendedServiceBroadcast& element) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const ExtendedServiceBroadcast& element) -> std::ostream&;

struct SystemInfo {
    unsigned _BitInt(12) main_carrier_;
    unsigned _BitInt(4) frequency_band_;
    unsigned _BitInt(2) offset_;
    unsigned _BitInt(3) duplex_spacing_field_;
    unsigned _BitInt(1) reverse_operation_;
    unsigned _BitInt(2) number_secondary_control_channels_main_carrier_;
    unsigned _BitInt(3) ms_txpwr_max_cell_;
    unsigned _BitInt(4) rxlev_access_min_;
    unsigned _BitInt(4) access_parameter_;
    unsigned _BitInt(4) radio_downlink_timeout_;
    std::optional<unsigned _BitInt(16)> hyper_frame_number_;
    std::optional<unsigned _BitInt(16)> common_cipher_key_identifier_or_static_cipher_key_version_number_;
    std::optional<unsigned _BitInt(20)> even_multi_frame_definition_for_ts_mode_;
    std::optional<unsigned _BitInt(20)> odd_multi_frame_definition_for_ts_mode_;
    std::optional<AccessCodeDefinition> defaults_for_access_code_a_;
    std::optional<ExtendedServiceBroadcast> extended_service_broadcast_;

    unsigned _BitInt(14) location_area_;
    unsigned _BitInt(16) subscriber_class_;
    unsigned _BitInt(1) registration_;
    unsigned _BitInt(1) deregistration_;
    unsigned _BitInt(1) priority_cell_;
    unsigned _BitInt(1) minimum_mode_service_;
    unsigned _BitInt(1) migration_;
    unsigned _BitInt(1) system_wide_service_;
    unsigned _BitInt(1) tetra_voice_service_;
    unsigned _BitInt(1) circuit_mode_data_service_;
    unsigned _BitInt(1) sndcp_service_;
    unsigned _BitInt(1) air_interface_encryption_service_;
    unsigned _BitInt(1) advanced_link_supported_;

    SystemInfo() = delete;
    explicit SystemInfo(BitVector& data);

    // get the downlink frequency in Hz
    [[nodiscard]] auto downlink_frequency() const noexcept -> int32_t;

    // get the uplink frequency in Hz
    [[nodiscard]] auto uplink_frequency() const noexcept -> int32_t;

    friend auto operator<<(std::ostream& stream, const SystemInfo& element) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const SystemInfo& element) -> std::ostream&;

struct AccessDefine {
    unsigned _BitInt(1) common_or_assigned_control_channel_flag_;
    unsigned _BitInt(2) access_code_;
    AccessCodeDefinition access_code_definition_;
    std::optional<unsigned _BitInt(16)> subscriber_class_bitmap_;
    std::optional<unsigned _BitInt(24)> gssi_;

    AccessDefine() = delete;
    explicit AccessDefine(BitVector& data);

    friend auto operator<<(std::ostream& stream, const AccessDefine& element) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const AccessDefine& element) -> std::ostream&;

struct UpperMacBroadcastPacket {
    /// the type of the logical channel on which this packet is sent
    LogicalChannel logical_channel_;
    /// the type of the mac packet
    MacPacketType type_;

    std::optional<SystemInfo> sysinfo_;
    std::optional<AccessDefine> access_define_;

    friend auto operator<<(std::ostream& stream, const UpperMacBroadcastPacket& packet) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const UpperMacBroadcastPacket& packet) -> std::ostream&;

/// reconstruct the number of bits from the length indication field
struct LengthIndication {
    // Y1 and Z1 octets, for a PDU sent in a subslot (i.e. MAC-ACCESS or MAC-END-HU).
    // Y2 and Z2 octets, for a PDU sent in a slot (i.e. MAC-DATA, MAC-RESOURCE or MAC-END).
    // These values are only valid for π/4-DQPSK. For values in other modulation schemes look at:
    // "Table 21.98: Value of Y1, Z1, Y2 and Z2 in TMA-SAP MAC PDUs"
    // The constants below are not in unit of octets, but in bits!
    static const std::size_t Y1 = 8;
    static const std::size_t Y2 = 8;
    static const std::size_t Z1 = 8;
    static const std::size_t Z2 = 8;

    static auto from_mac_access(unsigned _BitInt(5) length_indication) -> std::size_t {
        if (length_indication < 0b01111) {
            return length_indication * Y1;
        }
        return (14 * Y1 + (length_indication - 14) * Z1);
    };

    static auto from_mac_end_hu(unsigned _BitInt(4) length_indication) -> std::size_t {
        return length_indication * Z1;
    };

    static auto from_mac_data(unsigned _BitInt(6) length_indication) -> std::size_t {
        if (length_indication < 0b010011) {
            return length_indication * Y2;
        }
        return (18 * Y2 + (length_indication - 18) * Z2);
    };

    static auto from_mac_end_uplink(unsigned _BitInt(6) length_indication) -> std::size_t {
        if (length_indication < 0b000111) {
            return length_indication * Y2;
        }
        return (6 * Y2 + (length_indication - 6) * Z2);
    };

    static auto from_mac_resource(unsigned _BitInt(6) length_indication) -> std::size_t {
        if (length_indication < 0b010011) {
            return length_indication * Y2;
        }
        return (18 * Y2 + (length_indication - 18) * Z2);
    };

    static auto from_mac_end_downlink(unsigned _BitInt(6) length_indication) -> std::size_t {
        if (length_indication < 0b010011) {
            return length_indication * Y2;
        }
        return (18 * Y2 + (length_indication - 18) * Z2);
    };
};

struct ExtendedCarrierNumbering {
    unsigned _BitInt(4) frequency_band_;
    unsigned _BitInt(2) offset_;
    unsigned _BitInt(3) duplex_spacing_;
    unsigned _BitInt(1) reverse_operation_;

    friend auto operator<<(std::ostream& stream, const ExtendedCarrierNumbering& element) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const ExtendedCarrierNumbering& element) -> std::ostream&;

struct AugmentedChannelAllocation {
    unsigned _BitInt(2) up_downlink_assigned_;
    unsigned _BitInt(3) bandwidth_;

    unsigned _BitInt(3) modulation_mode_;
    std::optional<unsigned _BitInt(3)> maximum_uplink_qam_modulation_level_;

    unsigned _BitInt(3) conforming_channel_status_;
    unsigned _BitInt(4) bs_link_imbalance_;
    unsigned _BitInt(5) bs_transmit_power_relative_to_main_carrier_;

    unsigned _BitInt(2) napping_status_;
    std::optional<unsigned _BitInt(11)> napping_information_;

    std::optional<unsigned _BitInt(16)> conditional_element_a_;
    std::optional<unsigned _BitInt(16)> conditional_element_b_;
    unsigned _BitInt(1) further_augmentation_flag_;

    friend auto operator<<(std::ostream& stream, const AugmentedChannelAllocation& element) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const AugmentedChannelAllocation& element) -> std::ostream&;

struct ChannelAllocationElement {
    unsigned _BitInt(2) allocation_type_;
    unsigned _BitInt(4) timeslot_assigned_;
    unsigned _BitInt(2) up_downlink_assigned_;
    unsigned _BitInt(1) clch_permission_;
    unsigned _BitInt(1) cell_change_flag_;

    unsigned _BitInt(12) carrier_number_;
    std::optional<ExtendedCarrierNumbering> extended_carrier_numbering_;

    unsigned _BitInt(2) monitoring_pattern_;
    std::optional<unsigned _BitInt(2)> frame18_monitoring_pattern_;

    std::optional<AugmentedChannelAllocation> augmented_channel_allocation_;

    ChannelAllocationElement() = delete;
    explicit ChannelAllocationElement(BitVector& data);

    friend auto operator<<(std::ostream& stream, const ChannelAllocationElement& element) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const ChannelAllocationElement& element) -> std::ostream&;

struct UpperMacCPlaneSignallingPacket {
    /// the type of the logical channel on which this packet is sent
    LogicalChannel logical_channel_;
    /// the type of the mac packet
    MacPacketType type_;

    bool encrypted_ = false;

    AddressType address_;
    bool fragmentation_ = false;

    bool fragmentation_on_stealling_channel_ = false;

    std::optional<unsigned _BitInt(4)> reservation_requirement_;

    /// the tm_sdu that is passed to the LLC
    std::optional<BitVector> tm_sdu_;

    // Uplink
    std::optional<unsigned _BitInt(2)> encryption_mode_;

    // Downlink
    std::optional<bool> immediate_napping_permission_flag_;
    std::optional<unsigned _BitInt(8)> basic_slot_granting_element_;
    std::optional<unsigned _BitInt(1)> position_of_grant_;
    std::optional<ChannelAllocationElement> channel_allocation_element_;
    std::optional<unsigned _BitInt(1)> random_access_flag_;
    std::optional<unsigned _BitInt(4)> power_control_element_;

    /// check if this packet is a null pdu
    [[nodiscard]] auto is_null_pdu() const -> bool {
        return type_ == MacPacketType::kMacResource && address_ == AddressType{};
    };

    friend auto operator<<(std::ostream& stream, const UpperMacCPlaneSignallingPacket& packet) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const UpperMacCPlaneSignallingPacket& packet) -> std::ostream&;

struct UpperMacUPlaneSignallingPacket {
    /// the type of the logical channel on which this packet is sent
    LogicalChannel logical_channel_;
    /// the type of the mac packet
    MacPacketType type_;
    /// the tm_sdu that is passed to the LLC
    BitVector tm_sdu;

    /// all the other relevant fields...

    friend auto operator<<(std::ostream& stream, const UpperMacUPlaneSignallingPacket& packet) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const UpperMacUPlaneSignallingPacket& packet) -> std::ostream&;

struct UpperMacUPlaneTrafficPacket {
    /// the type of the logical channel on which this packet is sent
    LogicalChannel logical_channel_;
    /// the traffic in the packet
    BitVector data;

    /// all the other relevant fields...

    friend auto operator<<(std::ostream& stream, const UpperMacUPlaneTrafficPacket& packet) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const UpperMacUPlaneTrafficPacket& packet) -> std::ostream&;