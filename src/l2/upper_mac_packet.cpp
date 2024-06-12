/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "l2/upper_mac_packet.hpp"

AccessCodeDefinition::AccessCodeDefinition(BitVector& data)
    : immediate_(data.take<4>())
    , waiting_time_(data.take<4>())
    , number_of_random_access_transmissions_on_up_link_(data.take<4>())
    , frame_length_factor_(data.take<1>())
    , timeslot_pointer_(data.take<4>())
    , minimum_pdu_priority_(data.take<3>()) {}

ExtendedServiceBroadcastSection1::ExtendedServiceBroadcastSection1(BitVector& data)
    : data_priority_supported_(data.take<1>())
    , extended_advanced_links_and_max_ublck_supported_(data.take<1>())
    , qos_negotiation_supported_(data.take<1>())
    , d8psk_service_(data.take<1>())
    , section2_sent_(data.take<1>())
    , section3_sent_(data.take<1>())
    , section4_sent_(data.take<1>()) {}

ExtendedServiceBroadcastSection2::ExtendedServiceBroadcastSection2(BitVector& data)
    : service_25Qam_(data.take<1>())
    , service_50Qam_(data.take<1>())
    , service_100Qam_(data.take<1>())
    , service_150Qam_(data.take<1>())
    , reserved_(data.take<3>()) {}

ExtendedServiceBroadcastSection3::ExtendedServiceBroadcastSection3(BitVector& data)
    : reserved_(data.take<7>()) {}

ExtendedServiceBroadcastSection4::ExtendedServiceBroadcastSection4(BitVector& data)
    : reserved_(data.take<7>()) {}

ExtendedServiceBroadcast::ExtendedServiceBroadcast(BitVector& data)
    : security_information_(data.take<8>())
    , sdstl_addressing_method_(data.take<2>())
    , gck_supported_(data.take<1>()) {
    auto section = data.take<2>();
    if (section == 0b00) {
        section1_ = ExtendedServiceBroadcastSection1(data);
    } else if (section == 0b01) {
        section2_ = ExtendedServiceBroadcastSection2(data);
    } else if (section == 0b10) {
        section3_ = ExtendedServiceBroadcastSection3(data);
    } else {
        section4_ = ExtendedServiceBroadcastSection4(data);
    }
}

AccessDefine::AccessDefine(BitVector& data)
    : common_or_assigned_control_channel_flag_(data.take<1>())
    , access_code_(data.take<2>())
    , access_code_definition_(data) {
    auto optional_field_flag = data.take<2>();
    if (optional_field_flag == 0b01) {
        subscriber_class_bitmap_ = data.take<16>();
    } else if (optional_field_flag == 0b10) {
        gssi_ = data.take<24>();
    }
    auto filler_bits = data.take<3>();
}

SystemInfo::SystemInfo(BitVector& data)
    : main_carrier_(data.take<12>())
    , frequency_band_(data.take<4>())
    , offset_(data.take<2>())
    , duplex_spacing_field_(data.take<3>())
    , reverse_operation_(data.take<1>())
    , number_secondary_control_channels_main_carrier_(data.take<2>())
    , ms_txpwr_max_cell_(data.take<3>())
    , rxlev_access_min_(data.take<4>())
    , access_parameter_(data.take<4>())
    , radio_downlink_timeout_(data.take<4>()) {
    auto hyper_frame_cipher_key_flag = data.take<1>();
    if (hyper_frame_cipher_key_flag == 0) {
        hyper_frame_number_ = data.take<16>();
    } else {
        common_cipher_key_identifier_or_static_cipher_key_version_number_ = data.take<16>();
    }
    auto optional_field_flag = data.take<2>();
    if (optional_field_flag == 0b00) {
        even_multi_frame_definition_for_ts_mode_ = data.take<20>();
    } else if (optional_field_flag == 0b01) {
        odd_multi_frame_definition_for_ts_mode_ = data.take<20>();
    } else if (optional_field_flag == 0b10) {
        defaults_for_access_code_a_ = AccessCodeDefinition(data);
    } else if (optional_field_flag == 0b11) {
        extended_service_broadcast_ = ExtendedServiceBroadcast(data);
    }

    // This element shall be present when the PDU is sent using π/8-D8PSK
    // modulation. This element shall not be present when the PDU is sent using
    // π/4-DQPSK modulation.
    // auto reserved = vec.take<28>();

    // Location area (14)
    location_area_ = data.take<14>();
    // Subscriber class (16)
    subscriber_class_ = data.take<16>();
    // BS service details (12)
    registration_ = data.take<1>();
    deregistration_ = data.take<1>();
    priority_cell_ = data.take<1>();
    minimum_mode_service_ = data.take<1>();
    migration_ = data.take<1>();
    system_wide_service_ = data.take<1>();
    tetra_voice_service_ = data.take<1>();
    circuit_mode_data_service_ = data.take<1>();
    auto reserved = data.take<1>();
    sndcp_service_ = data.take<1>();
    air_interface_encryption_service_ = data.take<1>();
    advanced_link_supported_ = data.take<1>();
}

auto SystemInfo::downlink_frequency() const noexcept -> int32_t {
    // downlink main carrier frequency = base frequency + (main carrier × 25 kHz)
    // + offset kHz.
    const int32_t duplex[4] = {0, 6250, -6250, 12500};
    return frequency_band_ * 100000000 + main_carrier_ * 25000 + duplex[offset_];
}

auto SystemInfo::uplink_frequency() const noexcept -> int32_t {
    static const int32_t kTetraDuplexSpacing[8][16] = {
        /* values are in kHz */
        [0] = {-1, 1600, 10000, 10000, 10000, 10000, 10000, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        [1] = {-1, 4500, -1, 36000, 7000, -1, -1, -1, 45000, 45000, -1, -1, -1, -1, -1, -1},
        [2] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        [3] = {-1, -1, -1, 8000, 8000, -1, -1, -1, 18000, 18000, -1, -1, -1, -1, -1, -1},
        [4] = {-1, -1, -1, 18000, 5000, -1, 30000, 30000, -1, 39000, -1, -1, -1, -1, -1, -1},
        [5] = {-1, -1, -1, -1, 9500, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        [6] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
        [7] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    };
    const auto duplex_spacing = kTetraDuplexSpacing[duplex_spacing_field_][frequency_band_];
    /* reserved for future standardization */
    if (duplex_spacing < 0) {
        return 0;
    }
    if (reverse_operation_) {
        return downlink_frequency() + duplex_spacing * 1000;
    }
    return downlink_frequency() - duplex_spacing * 1000;
}

ChannelAllocationElement::ChannelAllocationElement(BitVector& data)
    : allocation_type_(data.take<2>())
    , timeslot_assigned_(data.take<4>())
    , up_downlink_assigned_(data.take<2>())
    , clch_permission_(data.take<1>())
    , cell_change_flag_(data.take<1>())
    , carrier_number_(data.take<12>()) {

    auto extended_carrier_numbering_flag = data.take<1>();
    if (extended_carrier_numbering_flag == 0b1) {
        ExtendedCarrierNumbering extended_carrier_numbering{};
        extended_carrier_numbering.frequency_band_ = data.take<4>();
        extended_carrier_numbering.offset_ = data.take<2>();
        extended_carrier_numbering.duplex_spacing_ = data.take<3>();
        extended_carrier_numbering.reverse_operation_ = data.take<1>();

        extended_carrier_numbering_ = extended_carrier_numbering;
    }

    monitoring_pattern_ = data.take<2>();
    if (monitoring_pattern_ == 0b00) {
        frame18_monitoring_pattern_ = data.take<2>();
    }

    if (up_downlink_assigned_ == 0b00) {
        AugmentedChannelAllocation augmented_channel_allocation;

        augmented_channel_allocation.up_downlink_assigned_ = data.take<2>();
        augmented_channel_allocation.bandwidth_ = data.take<3>();
        augmented_channel_allocation.modulation_mode_ = data.take<3>();
        if (augmented_channel_allocation.modulation_mode_ == 0b010) {
            augmented_channel_allocation.maximum_uplink_qam_modulation_level_ = data.take<3>();
            auto reserved = data.take<3>();
        }
        augmented_channel_allocation.conforming_channel_status_ = data.take<3>();
        augmented_channel_allocation.bs_link_imbalance_ = data.take<4>();
        augmented_channel_allocation.bs_transmit_power_relative_to_main_carrier_ = data.take<5>();

        augmented_channel_allocation.napping_status_ = data.take<2>();
        if (augmented_channel_allocation.napping_status_ == 0b01) {
            augmented_channel_allocation.napping_information_ = data.take<11>();
        }
        auto reserved = data.take<4>();
        auto conditional_element_a_flag = data.take<1>();
        if (conditional_element_a_flag == 0b1) {
            augmented_channel_allocation.conditional_element_a_ = data.take<16>();
        }
        auto conditional_element_b_flag = data.take<1>();
        if (conditional_element_b_flag) {
            augmented_channel_allocation.conditional_element_b_ = data.take<16>();
        }
        augmented_channel_allocation.further_augmentation_flag_ = data.take<1>();

        augmented_channel_allocation_ = augmented_channel_allocation;
    }
}