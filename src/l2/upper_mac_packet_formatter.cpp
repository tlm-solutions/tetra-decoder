/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "l2/upper_mac_packet.hpp"
#include <bitset>
#include <ostream>

auto operator<<(std::ostream& stream, const AccessCodeDefinition& element) -> std::ostream& {
    stream << "    Default definition for access code A information element:" << std::endl;
    stream << "      Immediate: 0b" << std::bitset<4>(element.immediate_) << std::endl;
    stream << "      Waiting time: 0b" << std::bitset<4>(element.waiting_time_) << std::endl;
    stream << "      Number of random access transmissions on uplink: 0b"
           << std::bitset<4>(element.number_of_random_access_transmissions_on_up_link_) << std::endl;
    stream << "      Frame-length factor: 0b" << std::bitset<1>(element.frame_length_factor_) << std::endl;
    stream << "      Timeslot pointer: 0b" << std::bitset<4>(element.timeslot_pointer_) << std::endl;
    stream << "      Minimum PDU priority: 0b" << std::bitset<3>(element.minimum_pdu_priority_) << std::endl;
    return stream;
}

auto operator<<(std::ostream& stream, const ExtendedServiceBroadcastSection1& element) -> std::ostream& {
    stream << "      Section1:" << std::endl;
    stream << "        Data priority supported: 0b" << std::bitset<1>(element.data_priority_supported_) << std::endl;
    stream << "        Extended advanced links and MAC-U-BLCK supported: 0b"
           << std::bitset<1>(element.extended_advanced_links_and_max_ublck_supported_) << std::endl;
    stream << "        QoS negotiation supported: 0b" << std::bitset<1>(element.qos_negotiation_supported_)
           << std::endl;
    stream << "        D8PSK service: 0b" << std::bitset<1>(element.d8psk_service_) << std::endl;
    stream << "        section 2 sent: 0b" << std::bitset<1>(element.section2_sent_) << std::endl;
    stream << "        section 3 sent: 0b" << std::bitset<1>(element.section3_sent_) << std::endl;
    stream << "        section 4 sent: 0b" << std::bitset<1>(element.section4_sent_) << std::endl;
    return stream;
}

auto operator<<(std::ostream& stream, const ExtendedServiceBroadcastSection2& element) -> std::ostream& {
    stream << "      Section2:" << std::endl;
    stream << "        25 kHz QAM service: 0b" << std::bitset<1>(element.service_25Qam_) << std::endl;
    stream << "        50 kHz QAM service: 0b" << std::bitset<1>(element.service_50Qam_) << std::endl;
    stream << "        100 kHz QAM service: 0b" << std::bitset<1>(element.service_100Qam_) << std::endl;
    stream << "        150 kHz QAM service: 0b" << std::bitset<1>(element.service_150Qam_) << std::endl;
    stream << "        reserved: " << std::bitset<3>(element.reserved_) << std::endl;
    return stream;
}

auto operator<<(std::ostream& stream, const ExtendedServiceBroadcastSection3& element) -> std::ostream& {
    stream << "      Section3:" << std::endl;
    stream << "        reserved: " << std::bitset<7>(element.reserved_) << std::endl;
    return stream;
}

auto operator<<(std::ostream& stream, const ExtendedServiceBroadcastSection4& element) -> std::ostream& {
    stream << "      Section4:" << std::endl;
    stream << "        reserved: " << std::bitset<7>(element.reserved_) << std::endl;
    return stream;
}

auto operator<<(std::ostream& stream, const ExtendedServiceBroadcast& element) -> std::ostream& {
    stream << "    Extended services broadcast:" << std::endl;
    stream << "      Security information: 0b" << std::bitset<8>(element.security_information_) << std::endl;
    stream << "      SDS-TL addressing method: 0b" << std::bitset<2>(element.sdstl_addressing_method_) << std::endl;
    stream << "      GCK supported: 0b" << std::bitset<1>(element.gck_supported_) << std::endl;
    if (element.section1_) {
        stream << *element.section1_;
    }
    if (element.section2_) {
        stream << *element.section2_;
    }
    if (element.section3_) {
        stream << *element.section3_;
    }
    if (element.section4_) {
        stream << *element.section4_;
    }
    return stream;
}

auto operator<<(std::ostream& stream, const SystemInfo& element) -> std::ostream& {
    stream << "    [SYSINFO]" << std::endl;
    stream << "    DL " << std::to_string(element.downlink_frequency()) << "Hz UL "
           << std::to_string(element.uplink_frequency()) << "Hz" << std::endl;
    stream << "    Number of common secondary control channels in use on CA main "
              "carrier: ";
    switch (element.number_secondary_control_channels_main_carrier_) {
    case 0b00:
        stream << "None";
        break;
    case 0b01:
        stream << "Timeslot 2 of main carrier";
        break;
    case 0b10:
        stream << "Timeslots 2 and 3 of main carrier";
        break;
    case 0b11:
        stream << "Timeslots 2, 3 and 4 of main carrier";
        break;
    }
    stream << std::endl;
    stream << "    MS_TXPWR_MAX_CELL: ";
    switch (element.ms_txpwr_max_cell_) {
    case 0b000:
        stream << "Reserved";
        break;
    default:
        stream << std::to_string(10 + 5 * element.ms_txpwr_max_cell_) << " dBm";
        break;
    }
    stream << std::endl;
    stream << "    RXLEV_ACCESS_MIN: " << std::to_string(-125 + 5 * element.rxlev_access_min_) << " dBm" << std::endl;
    stream << "    ACCESS_PARAMETER: " << std::to_string(-53 + 2 * element.access_parameter_) << " dBm" << std::endl;
    stream << "    RADIO_DOWNLINK_TIMEOUT: ";
    switch (element.radio_downlink_timeout_) {
    case 0b0000:
        stream << "Disable radio downlink counter";
        break;
    default:
        stream << std::to_string(144 * element.radio_downlink_timeout_) << " timeslots";
        break;
    }
    stream << std::endl;
    if (element.hyper_frame_number_) {
        stream << "    Cyclic count of hyperframes: " << std::bitset<16>(*element.hyper_frame_number_) << std::endl;
    }
    if (element.common_cipher_key_identifier_or_static_cipher_key_version_number_) {
        stream << "    Common cipher key identifier: "
               << std::bitset<16>(*element.common_cipher_key_identifier_or_static_cipher_key_version_number_)
               << std::endl;
    }
    if (element.even_multi_frame_definition_for_ts_mode_) {
        stream << "    Bit map of common frames for TS mode (even multiframes): 0b"
               << std::bitset<20>(*element.even_multi_frame_definition_for_ts_mode_) << std::endl;
    }
    if (element.odd_multi_frame_definition_for_ts_mode_) {
        stream << "    Bit map of common frames for TS mode (odd multiframes): 0b"
               << std::bitset<20>(*element.odd_multi_frame_definition_for_ts_mode_) << std::endl;
    }
    if (element.defaults_for_access_code_a_) {
        stream << *element.defaults_for_access_code_a_;
    }
    if (element.extended_service_broadcast_) {
        stream << *element.extended_service_broadcast_;
    }

    stream << "    Location Area (LA): " << static_cast<unsigned>(element.location_area_) << std::endl;
    stream << "    Subscriber Class 1..16 allowed: 0b" << std::bitset<16>(element.subscriber_class_) << std::endl;
    stream << "    "
           << (element.registration_ ? "Registration mandatory on this cell" : "Registration not required on this cell")
           << std::endl;
    stream << "    "
           << (element.deregistration_ ? "De-registration requested on this cell"
                                       : "De-registration not required on this cell")
           << std::endl;
    stream << "    " << (element.priority_cell_ ? "Cell is a priority cell" : "Cell is not a priority cell")
           << std::endl;
    stream << "    " << (element.minimum_mode_service_ ? "Cell never uses minimum mode" : "Cell may use minimum mode")
           << std::endl;
    stream << "    "
           << (element.migration_ ? "Migration is supported by this cell" : "Migration is not supported by this cell")
           << std::endl;
    stream << "    "
           << (element.system_wide_service_ ? "Normal mode (system wide services supported)"
                                            : "System wide services temporarily not supported")
           << std::endl;
    stream << "    "
           << (element.tetra_voice_service_ ? "TETRA voice service is supported on this cell"
                                            : "TETRA voice service is not supported on this cell")
           << std::endl;
    stream << "    "
           << (element.circuit_mode_data_service_ ? "Circuit mode data service is supported on this cell"
                                                  : "Circuit mode data service is not supported on this cell")
           << std::endl;
    stream << "    "
           << (element.sndcp_service_ ? "SNDCP service is available on this cell"
                                      : "SNDCP service is not available on this cell")
           << std::endl;
    stream << "    "
           << (element.air_interface_encryption_service_ ? "Air interface encryption is available on this cell"
                                                         : "Air interface encryption is not available on this cell")
           << std::endl;
    stream << "    "
           << (element.advanced_link_supported_ ? "Advanced link is supported on this cell"
                                                : "Advanced link is not supported on this cell")
           << std::endl;
    return stream;
}

auto operator<<(std::ostream& stream, const AccessDefine& element) -> std::ostream& {
    stream << "    [SYSINFO]" << std::endl;
    stream << "    "
           << (element.common_or_assigned_control_channel_flag_ ? "ACCESS-DEFINE applies to common channel"
                                                                : "ACCESS-DEFINE applies to assigned channel")
           << std::endl;
    stream << "    Access code ";
    switch (element.access_code_) {
    case 0b00:
        stream << "A";
        break;
    case 0b01:
        stream << "B";
        break;
    case 0b10:
        stream << "C";
        break;
    case 0b11:
        stream << "D";
        break;
    }
    stream << std::endl;
    stream << element.access_code_definition_;
    if (element.subscriber_class_bitmap_) {
        stream << "    Subscriber class bit map: " << std::bitset<16>(*element.subscriber_class_bitmap_) << std::endl;
    }
    if (element.gssi_) {
        stream << "    GSSI: " << std::bitset<24>(*element.gssi_) << std::endl;
    }
    return stream;
}

auto operator<<(std::ostream& stream, const UpperMacBroadcastPacket& packet) -> std::ostream& {
    stream << "  [Broadcast]" << std::endl;
    stream << "  [LogicalChannel] " << to_string(packet.logical_channel_) << std::endl;
    stream << "  [PacketType] " << to_string(packet.type_) << std::endl;
    if (packet.sysinfo_) {
        stream << *packet.sysinfo_;
    }
    if (packet.access_define_) {
        stream << *packet.access_define_;
    }
    return stream;
}

auto operator<<(std::ostream& stream, const ExtendedCarrierNumbering& element) -> std::ostream& {
    stream << "    frequency band: " << std::bitset<4>(element.frequency_band_) << std::endl;
    stream << "    offset: " << std::bitset<2>(element.offset_) << std::endl;
    stream << "    duplex spacing: " << std::bitset<3>(element.duplex_spacing_) << std::endl;
    stream << "    reverse operation: " << std::bitset<1>(element.reverse_operation_);
    return stream;
}

auto operator<<(std::ostream& stream, const AugmentedChannelAllocation& element) -> std::ostream& {
    stream << "    up/downlink assigned: " << std::bitset<2>(element.up_downlink_assigned_) << std::endl;
    stream << "    bandwidth: " << std::bitset<3>(element.bandwidth_) << std::endl;
    stream << "    modulation mode: " << std::bitset<3>(element.modulation_mode_) << std::endl;
    if (element.maximum_uplink_qam_modulation_level_) {
        stream << "    maximum qam modulation level: " << std::bitset<3>(*element.maximum_uplink_qam_modulation_level_)
               << std::endl;
    }
    stream << "    conforming channel status: " << std::bitset<3>(element.conforming_channel_status_) << std::endl;
    stream << "    bs link imbalance: " << std::bitset<4>(element.bs_link_imbalance_) << std::endl;
    stream << "    bs power relative to main carrier: "
           << std::bitset<5>(element.bs_transmit_power_relative_to_main_carrier_) << std::endl;
    stream << "    napping status: " << std::bitset<2>(element.napping_status_) << std::endl;
    if (element.napping_information_) {
        stream << "    napping information: " << std::bitset<11>(*element.napping_information_) << std::endl;
    }
    if (element.conditional_element_a_) {
        stream << "    conditional element a: " << std::bitset<16>(*element.conditional_element_a_) << std::endl;
    }
    if (element.conditional_element_b_) {
        stream << "    conditional element b: " << std::bitset<16>(*element.conditional_element_b_) << std::endl;
    }
    stream << "    further augmentation flag: " << std::bitset<1>(element.further_augmentation_flag_);
    return stream;
}

auto operator<<(std::ostream& stream, const ChannelAllocationElement& element) -> std::ostream& {
    stream << "    allocation type: " << std::bitset<2>(element.allocation_type_) << std::endl;
    stream << "    timeslot assigned: " << std::bitset<4>(element.timeslot_assigned_) << std::endl;
    stream << "    up/downlink assigned: " << std::bitset<2>(element.up_downlink_assigned_) << std::endl;
    stream << "    clch permissions: " << std::bitset<1>(element.clch_permission_) << std::endl;
    stream << "    cell change flag: " << std::bitset<1>(element.cell_change_flag_) << std::endl;
    stream << "    carrier number: " << std::bitset<12>(element.carrier_number_) << std::endl;
    if (element.extended_carrier_numbering_) {
        stream << *element.extended_carrier_numbering_ << std::endl;
    }
    stream << "    monitoring pattern: " << std::bitset<2>(element.monitoring_pattern_) << std::endl;
    if (element.frame18_monitoring_pattern_) {
        stream << "    frame18 monitoring pattern: " << std::bitset<2>(*element.frame18_monitoring_pattern_)
               << std::endl;
    }
    if (element.augmented_channel_allocation_) {
        stream << *element.augmented_channel_allocation_ << std::endl;
    }

    return stream;
}

auto operator<<(std::ostream& stream, const UpperMacCPlaneSignallingPacket& packet) -> std::ostream& {
    stream << "  [CPlaneSignalling]" << std::endl;
    stream << "  [LogicalChannel] " << to_string(packet.logical_channel_) << std::endl;
    stream << "  [PacketType] " << to_string(packet.type_) << std::endl;
    stream << "    encrypted: " << (packet.encrypted_ ? "true" : "false") << std::endl;
    if (packet.encryption_mode_) {
        stream << "    encryption mode: " << std::bitset<2>(*packet.encryption_mode_) << std::endl;
    }
    stream << "    address: " << packet.address_ << std::endl;
    stream << "    fragmentation: " << (packet.fragmentation_ ? "true" : "false") << std::endl;
    stream << "    fragmentation on stealing channel: "
           << (packet.fragmentation_on_stealling_channel_ ? "true" : "false") << std::endl;
    if (packet.reservation_requirement_) {
        stream << "    reservation requirement: " << std::bitset<4>(*packet.reservation_requirement_) << std::endl;
    }
    if (packet.immediate_napping_permission_flag_) {
        stream << "    immediate napping permission flag: "
               << (*packet.immediate_napping_permission_flag_ ? "true" : "false") << std::endl;
    }
    if (packet.basic_slot_granting_element_) {
        stream << "    basic slot granting element: " << std::bitset<8>(*packet.basic_slot_granting_element_)
               << std::endl;
    }
    if (packet.position_of_grant_) {
        stream << "    position of grant: " << std::bitset<1>(*packet.position_of_grant_) << std::endl;
    }
    if (packet.channel_allocation_element_) {
        stream << *packet.channel_allocation_element_;
    }
    if (packet.random_access_flag_) {
        stream << "    random access flag: " << std::bitset<1>(*packet.random_access_flag_) << std::endl;
    }
    if (packet.power_control_element_) {
        stream << "    power control element: " << std::bitset<4>(*packet.power_control_element_) << std::endl;
    }
    if (packet.tm_sdu_) {
        stream << "    " << *packet.tm_sdu_ << std::endl;
    }
    return stream;
}

auto operator<<(std::ostream& stream, const UpperMacUPlaneSignallingPacket& packet) -> std::ostream& {
    stream << "  [UPlaneSignalling]" << std::endl;
    stream << "  [LogicalChannel] " << to_string(packet.logical_channel_) << std::endl;
    stream << "  [PacketType] " << to_string(packet.type_) << std::endl;
    stream << "    " << packet.tm_sdu_ << std::endl;
    return stream;
}

auto operator<<(std::ostream& stream, const UpperMacUPlaneTrafficPacket& packet) -> std::ostream& {
    stream << "  [UPlaneTraffic]" << std::endl;
    stream << "  [LogicalChannel] " << to_string(packet.logical_channel_) << std::endl;
    stream << "    " << packet.data_ << std::endl;
    return stream;
}