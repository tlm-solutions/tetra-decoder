#include <bitset>
#include <cassert>

#include <l2/upper_mac.hpp>
#include <utils/bit_vector.hpp>

void UpperMac::process_SCH_HD(const BurstType burst_type, BitVector& vec) {
    assert(is_downlink_burst(burst_type));

    std::cout << "[Channel] SCH_HD" << std::endl;
    process_signalling_channel(burst_type, vec, true, false);
}

void UpperMac::process_SCH_HU(const BurstType burst_type, BitVector& vec) {
    assert(vec.bits_left() == 92);
    assert(is_uplink_burst(burst_type));

    try {
        fragmentation_start_burst();
        last_address_type_ = AddressType();

        std::cout << "[Channel] SCH_HU" << std::endl;

        auto pduType = vec.take<1>();
        auto fill_bit_indication = vec.take<1>();

        remove_fill_bits_ = true;
        if (fill_bit_indication == 0b1) {
            remove_fill_bits(vec);
        }

        if (pduType == 0b0) {
            process_mac_access(vec);
        } else if (pduType == 0b1) {
            process_mac_end_hu(vec);
        }
        // TODO: one mac may contain multiple mac headers...
        // compare with process_signalling_channel

        fragmentation_end_burst();
    } catch (std::exception& e) {
        std::cout << "Error with decoding: " << e.what() << std::endl;
    }
}

void UpperMac::process_SCH_F(const BurstType burst_type, BitVector& vec) {
    std::cout << "[Channel] SCH_F" << std::endl;
    process_signalling_channel(burst_type, vec, false, false);
}

void UpperMac::process_STCH(const BurstType burst_type, BitVector& vec) {
    std::cout << "[Channel] STCH" << std::endl;

    second_slot_stolen_ = false;

    process_signalling_channel(burst_type, vec, true, true);
}

void UpperMac::process_signalling_channel(const BurstType burst_type, BitVector& vec, bool isHalfChannel,
                                          bool isStolenChannel) {
    remove_fill_bits_ = true;
    try {
        fragmentation_start_burst();
        process_signalling_channel_internal(burst_type, vec, isHalfChannel, isStolenChannel);
        fragmentation_end_burst();
    } catch (std::exception& e) {
        std::cout << "Error with decoding: " << e.what() << std::endl;
    }
}

void UpperMac::process_signalling_channel_internal(const BurstType burst_type, BitVector& vec, bool isHalfChannel,
                                                   bool isStolenChannel) {
    auto pduType = vec.take<2>();

    if (pduType == 0b00) {
        // MAC-RESOURCE (downlink) or MAC-DATA (uplink)
        // TMA-SAP
        if (is_downlink_burst(burst_type)) {
            process_mac_resource(vec);
        } else {
            process_mac_data(vec);
        }
    } else if (pduType == 0b01) {
        // MAC-END or MAC-FRAG
        // TMA-SAP
        auto subtype = vec.take<1>();
        if (subtype == 0b0) {
            if (is_downlink_burst(burst_type)) {
                process_mac_frag_downlink(vec);
            } else {
                process_mac_frag_uplink(vec);
            }
        } else if (subtype == 0b1) {
            if (is_downlink_burst(burst_type)) {
                process_mac_end_downlink(vec);
            } else {
                process_mac_end_uplink(vec);
            }
        }
    } else if (pduType == 0b10) {
        // Broadcast
        // TMB-SAP
        // ✅ done
        if (is_uplink_burst(burst_type)) {
            throw std::runtime_error("Uplink Burst Type may not send broadcast packets");
        }
        process_broadcast(vec);
    } else if (pduType == 0b11) {
        // Supplementary MAC PDU (not on STCH, SCH/HD or SCH-P8/HD)
        // MAC-U-SIGNAL (only on STCH)
        // TMA-SAP or TMD-SAP
        if (isHalfChannel && isStolenChannel) {
            process_mac_usignal(vec);
        } else if ((!isHalfChannel) && (!isStolenChannel)) {
            process_supplementary_mac_pdu(burst_type, vec);
        } else {
            // Reserved
            // TODO: print unimplemented error
        }
    }

    // TODO: one mac may contain multiple mac headers. proccess all the others
    // layers first and then continue with the next
    if (vec.bits_left() > 0) {
        std::cout << "BitsLeft(): " << std::to_string(vec.bits_left()) << " " << vec << std::endl;
        if (!vec.is_mac_padding()) {
            process_signalling_channel_internal(burst_type, vec, isHalfChannel, isStolenChannel);
        }
    }
}

void UpperMac::process_broadcast(BitVector& vec) {
    auto broadcastType = vec.take<2>();
    switch (broadcastType) {
    case 0b00:
        // SYSINFO PDU
        process_system_info_pdu(vec);
        break;
    case 0b01:
        // ACCESS-DEFINE PDU
        process_access_define_pdu(vec);
        break;
    case 0b10:
        // SYSINFO-DA
        // only QAM
        process_system_info_da(vec);
        break;
    case 0b11:
        // Reserved
        // TODO: print unimplemented error
        break;
    }
}

void UpperMac::process_supplementary_mac_pdu(const BurstType burst_type, BitVector& vec) {
    auto subtype = vec.take<1>();

    switch (subtype) {
    case 0b0:
        if (is_downlink_burst(burst_type)) {
            process_mac_d_blck(vec);
        } else {
            process_mac_u_blck(vec);
        }
        break;
    case 0b1:
        // Reserved
        // TODO: print unimplemented error
        break;
    }
}

void UpperMac::process_system_info_pdu(BitVector& vec) {
    auto mainCarrier = vec.take<12>();
    auto frequency_band = vec.take<4>();
    auto offset = vec.take<2>();
    auto duplexSpacing = vec.take<3>();
    auto reverseOperation = vec.take<1>();
    number_secondary_control_channels_main_carrier_ = vec.take<2>();
    ms_txpwr_max_cell_ = vec.take<3>();
    rxlev_access_min_ = vec.take<4>();
    access_parameter_ = vec.take<4>();
    radio_downlink_timeout_ = vec.take<4>();
    hyper_frame_cipher_key_flag_ = vec.take<1>();
    if (hyper_frame_cipher_key_flag_ == 0) {
        hyper_frame_number_ = vec.take<16>();
    } else {
        common_cipher_key_identifier_or_static_cipher_key_version_number_ = vec.take<16>();
    }
    auto optionalFieldFlag = vec.take<2>();
    if (optionalFieldFlag == 0b00) {
        *even_multi_frame_definition_for_ts_mode_ = vec.take<20>();
    } else if (optionalFieldFlag == 0b01) {
        *odd_multi_frame_definition_for_ts_mode_ = vec.take<20>();
    } else if (optionalFieldFlag == 0b10) {
        defaults_for_access_code_a_.immediate_ = vec.take<4>();
        defaults_for_access_code_a_.waiting_time_ = vec.take<4>();
        defaults_for_access_code_a_.number_of_random_access_transmissions_on_up_link_ = vec.take<4>();
        defaults_for_access_code_a_.frame_length_factor_ = vec.take<1>();
        defaults_for_access_code_a_.timeslot_pointer_ = vec.take<4>();
        defaults_for_access_code_a_.minimum_pdu_priority_ = vec.take<3>();
        defaults_for_access_code_a_.system_info_ = true;
    } else if (optionalFieldFlag == 0b11) {
        extended_service_broadcast_.security_information_ = vec.take<8>();
        extended_service_broadcast_.sdstl_addressing_method_ = vec.take<2>();
        extended_service_broadcast_.gck_supported_ = vec.take<1>();
        auto section = vec.take<2>();
        if (section == 0b00) {
            extended_service_broadcast_.data_priority_supported_ = vec.take<1>();
            extended_service_broadcast_.extended_advanced_links_and_max_ublck_supported_ = vec.take<1>();
            extended_service_broadcast_.qos_negotiation_supported_ = vec.take<1>();
            extended_service_broadcast_.d8psk_service_ = vec.take<1>();
            auto _sectionInformation = vec.take<3>();
            extended_service_broadcast_.system_info_section_1_ = true;
        } else if (section == 0b01) {
            extended_service_broadcast_.service_25Qam_ = vec.take<1>();
            extended_service_broadcast_.service_50Qam_ = vec.take<1>();
            extended_service_broadcast_.service_100Qam_ = vec.take<1>();
            extended_service_broadcast_.service_150Qam_ = vec.take<1>();
            auto _reserved = vec.take<3>();
            extended_service_broadcast_.system_info_section_2_ = true;
        } else {
            // TODO: Section 2 and 3 are reserved
            auto _reserved = vec.take<7>();
        }
        extended_service_broadcast_.system_info_ = true;
    }

    // downlink main carrier frequency = base frequency + (main carrier × 25 kHz)
    // + offset kHz.
    const int32_t duplex[4] = {0, 6250, -6250, 12500};
    downlink_frequency_ = frequency_band * 100000000 + mainCarrier * 25000 + duplex[offset];

    static const int32_t tetra_duplex_spacing[8][16] = {
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
    uint32_t duplex_spacing = tetra_duplex_spacing[duplexSpacing][frequency_band];
    /* reserved for future standardization */
    if (duplex_spacing < 0) {
        uplink_frequency_ = 0;
    } else if (reverseOperation) {
        uplink_frequency_ = static_cast<int32_t>(downlink_frequency_) + duplex_spacing * 1000;
    } else {
        uplink_frequency_ = static_cast<int32_t>(downlink_frequency_) - duplex_spacing * 1000;
    }

    // This element shall be present when the PDU is sent using π/8-D8PSK
    // modulation. This element shall not be present when the PDU is sent using
    // π/4-DQPSK modulation.
    // auto reserved = vec.take<28>();

    mobile_link_entity_->service_DMle_system_info(vec);

    system_info_received_ = true;
}

void UpperMac::process_access_define_pdu(BitVector& vec) {
    auto _ = vec.take<23>();
    auto optional_field_flag = vec.take<2>();
    if (optional_field_flag == 0b01) {
        auto _subscriber_class_bitmap = vec.take<16>();
    } else if (optional_field_flag == 0b10) {
        auto _gssi = vec.take<24>();
    }
    auto _filter_bits = vec.take<3>();
}

void UpperMac::process_mac_usignal(BitVector& vec) {
    second_slot_stolen_ = (vec.take<1>() == 1);

    // TODO: TM-SDU
    auto bits_left = vec.bits_left();
    auto tm_sdu = vec.take_vector(bits_left);
    std::cout << "MAC U-SIGNAL" << std::endl;
    std::cout << "  Second subslot is stolen: " << (second_slot_stolen_ ? "true" : "false") << std::endl;
    std::cout << "  TM-SDU: size = " << std::to_string(tm_sdu.bits_left()) << ": " << tm_sdu << std::endl;
}

void UpperMac::process_mac_d_blck(BitVector& vec) {
    auto fill_bit_indication = vec.take<1>();
    auto encrypted = vec.take<2>();
    auto event_label = vec.take<10>();
    auto address = AddressType();
    address.set_event_label(event_label);
    auto immediate_napping_permission_flag = vec.take<1>();
    auto slot_granting_flag = vec.take<1>();
    auto basic_slot_granting_element = 0;
    if (slot_granting_flag == 0b1) {
        basic_slot_granting_element = vec.take<8>();
    }

    if (fill_bit_indication == 0b1) {
        remove_fill_bits(vec);
    }

    auto bits_left = vec.bits_left();
    auto tm_sdu = vec.take_vector(bits_left);
    std::cout << "MAC D-BLCK" << std::endl;
    std::cout << "  TM-SDU: size = " << std::to_string(tm_sdu.bits_left()) << ": " << tm_sdu << std::endl;
    std::cout << "  Address: " << address << std::endl;

    logical_link_control_->process(address, tm_sdu);
}

void UpperMac::process_mac_u_blck(BitVector& vec) {
    auto fill_bit_indication = vec.take<1>();
    auto encrypted = vec.take<1>();
    auto event_label = vec.take<10>();
    auto address = AddressType();
    address.set_event_label(event_label);
    auto reservation_requirement = vec.take<4>();

    if (fill_bit_indication == 0b1) {
        remove_fill_bits(vec);
    }

    auto bits_left = vec.bits_left();
    auto tm_sdu = vec.take_vector(bits_left);
    std::cout << "MAC U-BLCK" << std::endl;
    std::cout << "  TM-SDU: size = " << std::to_string(tm_sdu.bits_left()) << ": " << tm_sdu << std::endl;
    std::cout << "  encrypted: 0b" << std::bitset<1>(encrypted) << std::endl;
    std::cout << "  reservation_requirement: 0b" << std::bitset<4>(reservation_requirement) << std::endl;
    std::cout << "  Address: " << address << std::endl;

    logical_link_control_->process(address, tm_sdu);
}

void UpperMac::process_mac_frag_downlink(BitVector& vec) {
    auto fill_bit_indication = vec.take<1>();

    if (fill_bit_indication == 0b1) {
        remove_fill_bits(vec);
    }

    auto bits_left = vec.bits_left();
    auto tm_sdu = vec.take_vector(bits_left);
    std::cout << "MAC FRAG" << std::endl;
    std::cout << "  TM-SDU: size = " << std::to_string(tm_sdu.bits_left()) << ": " << tm_sdu << std::endl;

    fragmentation_push_tm_sdu_frag(tm_sdu);
}

void UpperMac::process_mac_frag_uplink(BitVector& vec) {
    auto fill_bit_indication = vec.take<1>();

    if (fill_bit_indication == 0b1) {
        remove_fill_bits(vec);
    }

    auto bits_left = vec.bits_left();
    auto tm_sdu = vec.take_vector(bits_left);
    std::cout << "MAC FRAG" << std::endl;
    std::cout << "  TM-SDU: size = " << std::to_string(tm_sdu.bits_left()) << ": " << tm_sdu << std::endl;

    fragmentation_push_tm_sdu_frag(tm_sdu);
}

void UpperMac::process_mac_end_downlink(BitVector& vec) {
    second_slot_stolen_ = false;

    auto pre_processing_bit_count = vec.bits_left() + 3;

    auto fill_bit_indication = vec.take<1>();
    auto position_of_grant = vec.take<1>();
    auto length_indictaion = vec.take<6>();
    // The immediate napping permission flag shall be present when the PDU is sent
    // using π/8-D8PSK or QAM modulation. It shall not be present when the PDU is
    // sent using π/4-DQPSK modulation. auto immediateNapping = vec.take<1>();
    auto slot_granting_flag = vec.take<1>();
    // The multiple slot granting flag shall be present when the slot granting
    // flag is set to 1 and the PDU is sent using QAM modulation. It shall not be
    // present when the slot granting flag is set to 0 or the PDU is sent using
    // π/4-DQPSK or π/8-D8PSK modulation auto multipleSlotGranting = vec.take<1>();
    // The basic slot granting element shall be present when the slot granting
    // flag is set to 1 and either the PDU is sent using π/4-DQPSK or π/8-D8PSK
    // modulation, or the PDU is sent using QAM modulation and the multiple slot
    // granting flag is set to 0.
    auto basic_slot_granting_element = 0;
    if (slot_granting_flag == 0b1) {
        basic_slot_granting_element = vec.take<8>();
    }
    auto channel_allocation_flag = vec.take<1>();
    if (channel_allocation_flag == 0b1) {
        auto channel_allocation1 = vec.take<22>();
        auto extended_carrier_numbering_flag = vec.take<1>();
        if (extended_carrier_numbering_flag == 0b1) {
            auto channel_allocation2 = vec.take<9>();
        }
        auto monitoring_pattern = vec.take<2>();
        if (monitoring_pattern == 0b00) {
            auto frame18_monitoring_pattern = vec.take<2>();
            auto up_downlink_assigned_for_augmented_channel_allocation = vec.take<2>();
            auto bandwidth_of_allocated_channel = vec.take<3>();
            auto modulation_mode_of_allocated_channel = vec.take<3>();
            if (modulation_mode_of_allocated_channel == 0b010) {
                auto maximum_uplink_qam_modulation_level = vec.take<3>();
                auto reserved = vec.take<3>();
            }
            auto channel_allocation3 = vec.take<12>();
            auto napping_status = vec.take<2>();
            if (napping_status == 0b01) {
                auto napping_information = vec.take<11>();
            }
            auto reserved = vec.take<4>();

            // taking optional element a
            auto conditional_element_a_flag = vec.take<1>();
            if (conditional_element_a_flag == 0b1) {
                auto conditional_element_a = vec.take<16>();
            }

            // taking optional element b
            auto conditional_element_b_flag = vec.take<1>();
            if (conditional_element_b_flag == 0b1) {
                auto conditional_element_b = vec.take<16>();
            }
            auto further_augmentation_flag = vec.take<1>();
        }
    }

    auto mac_header_length = pre_processing_bit_count - vec.bits_left();
    auto bits_left = length_indictaion * 8 - mac_header_length;

    if (fill_bit_indication == 0b1) {
        remove_fill_bits(vec);
    }

    auto tm_sdu = vec.take_vector(bits_left);
    std::cout << "MAC END" << std::endl;
    std::cout << "  length_indictaion: 0b" << std::bitset<6>(length_indictaion) << std::endl;
    std::cout << "  TM-SDU: size = " << std::to_string(bits_left) << ": " << tm_sdu << std::endl;
    std::cout << "  mac_header_length = " << std::to_string(mac_header_length) << std::endl;

    fragmentation_push_tm_sdu_end(tm_sdu);
}

void UpperMac::process_mac_end_uplink(BitVector& vec) {
    auto pre_processing_bit_count = vec.bits_left() + 3;

    auto fill_bit_indication = vec.take<1>();
    auto length_indictaion = vec.take<6>();

    auto mac_header_length = pre_processing_bit_count - vec.bits_left();
    auto bits_left = length_indictaion * 8 - mac_header_length;

    if (fill_bit_indication == 0b1) {
        remove_fill_bits(vec);
    }

    auto tm_sdu = vec.take_vector(bits_left);
    std::cout << "MAC END" << std::endl;
    std::cout << "  length_indictaion: 0b" << std::bitset<6>(length_indictaion) << std::endl;
    std::cout << "  TM-SDU: size = " << std::to_string(bits_left) << ": " << tm_sdu << std::endl;
    std::cout << "  mac_header_length = " << std::to_string(mac_header_length) << std::endl;

    fragmentation_push_tm_sdu_end(tm_sdu);
}

void UpperMac::process_mac_resource(BitVector& vec) {
    std::cout << "MAC RESOURCE" << std::endl;

    auto preprocessing_bit_count = vec.bits_left() + 2;

    auto fill_bit_indication = vec.take<1>();
    auto position_of_grant = vec.take<1>();
    auto encryption_mode = vec.take<2>();
    auto random_access_flag = vec.take<1>();
    auto length_indictaion = vec.take<6>();
    if (length_indictaion == 0b111110 || length_indictaion == 0b111111) {
        second_slot_stolen_ = true;
    }
    std::cout << "  Second subslot is stolen: " << (second_slot_stolen_ ? "true" : "false") << std::endl;
    std::cout << "  length_indictaion: 0b" << std::bitset<6>(length_indictaion) << std::endl;
    auto address_type = vec.take<3>();
    auto address = AddressType();
    std::cout << "  address_type: 0b" << std::bitset<3>(address_type) << std::endl;
    if (address_type == 0b000) {
        remove_fill_bits(vec);

        // std::cout << "  NULL PDU" << std::endl;
        // std::cout << "  fill_bit_indication: 0b" <<
        // std::bitset<1>(fill_bit_indication)
        //           << std::endl;
        // std::cout << "  encryption_mode: 0b" << std::bitset<2>(encryption_mode)
        //           << std::endl;
        // std::cout << "  length_indictaion: 0b" << std::bitset<5>(length_indictaion)
        //           << std::endl;
        return;
    } else if (address_type == 0b001) {
        address.set_ssi(vec.take<24>());
    } else if (address_type == 0b010) {
        address.set_event_label(vec.take<10>());
    } else if (address_type == 0b011) {
        address.set_ussi(vec.take<24>());
    } else if (address_type == 0b100) {
        address.set_smi(vec.take<24>());
    } else if (address_type == 0b101) {
        address.set_ssi(vec.take<24>());
        address.set_event_label(vec.take<10>());
    } else if (address_type == 0b110) {
        address.set_ssi(vec.take<24>());
        address.set_usage_marker(vec.take<6>());
    } else if (address_type == 0b111) {
        address.set_smi(vec.take<24>());
        address.set_event_label(vec.take<10>());
    }
    // The immediate napping permission flag shall be present when the PDU is sent
    // using π/8-D8PSK or QAM modulation. It shall not be present when the PDU is
    // sent using π/4-DQPSK modulation. auto immediateNappingPermisionFlag =
    // vec.take<1>();
    auto power_control_flag = vec.take<1>();
    if (power_control_flag == 0b1) {
        auto power_control_element = vec.take<4>();
    }
    auto slot_granting_flag = vec.take<1>();
    // The multiple slot granting flag shall be present when the slot granting
    // flag is set to 1 and the PDU is sent using QAM modulation. It shall not be
    // present when the slot granting flag is set to 0 or the PDU is sent using
    // π/4-DQPSK or π/8-D8PSK modulation. auto multipleSlotGrantingFlag =
    // vec.take<1>(); The basic slot granting element shall be present when the slot
    // granting flag is set to 1 and either the PDU is sent using π/4-DQPSK or
    // π/8-D8PSK modulation, or the PDU is sent using QAM modulation and the
    // multiple slot granting flag is set to 0.
    if (slot_granting_flag == 0b1) {
        auto basic_slot_granting_element = vec.take<8>();
    }
    auto channel_allocation_flag = vec.take<1>();
    std::cout << "  channel_allocation_flag = 0b" << std::bitset<1>(channel_allocation_flag) << std::endl;
    if (channel_allocation_flag == 0b1) {
        auto allocation_type = vec.take<2>();
        auto time_slot_assigned = vec.take<4>();
        auto up_downlink_assigned = vec.take<2>();
        auto clch_permission = vec.take<1>();
        auto cell_change_flag = vec.take<1>();
        auto carrier_number = vec.take<12>();
        auto extended_carrier_numbering_flag = vec.take<1>();
        if (extended_carrier_numbering_flag == 0b1) {
            auto channel_allocation_2 = vec.take<10>();
        }
        auto monitoring_pattern = vec.take<2>();
        if (monitoring_pattern == 0b00) {
            auto frame18_monitoring_pattern = vec.take<2>();
        }
        if (up_downlink_assigned == 0b00) {
            auto up_downlink_assigned_for_augmented_channel_allocation = vec.take<2>();
            auto bandwidth_of_allocated_channel = vec.take<3>();
            auto modulation_mode_of_allocated_channel = vec.take<3>();
            if (modulation_mode_of_allocated_channel == 0b010) {
                auto maximumUplinkQamModulationLevel = vec.take<3>();
            } else {
                auto reserved = vec.take<3>();
            }
            auto channel_allocation3 = vec.take<12>();
            auto napping_status = vec.take<2>();
            if (napping_status == 0b01) {
                auto napping_information = vec.take<11>();
            }
            auto reserved = vec.take<4>();
            auto conditional_element_a_flag = vec.take<1>();
            if (conditional_element_a_flag == 0b1) {
                auto conditional_element_a = vec.take<16>();
            }
            auto conditional_element_b_flag = vec.take<1>();
            if (conditional_element_b_flag == 0b1) {
                auto conditional_element_b = vec.take<16>();
            }
            auto further_augmentation_flag = vec.take<1>();
        }
    }

    auto mac_header_length = preprocessing_bit_count - vec.bits_left();
    auto bits_left = length_indictaion * 8 - mac_header_length;

    // std::cout << "MAC RESOURCE" << std::endl;
    // std::cout << "  encryption_mode: 0b" << std::bitset<2>(encryption_mode)
    //           << std::endl;
    // std::cout << "  fill_bit_indication: 0b" << std::bitset<1>(fill_bit_indication)
    //           << std::endl;
    // std::cout << "  length_indictaion: 0b" << std::bitset<5>(length_indictaion)
    //           << std::endl;
    // std::cout << "  channel_allocation_flag: 0b"
    //           << std::bitset<1>(channel_allocation_flag) << std::endl;

    // if (fill_bit_indication == 0b1 && (bits_left != 133) && (address_type !=
    // 0b001)) { if (fill_bit_indication == 0b1) {
    //   remove_fill_bits(vec);
    // }

    // XXX: check this
    if (length_indictaion == 0b111111 || length_indictaion == 0b111110) {
        bits_left = vec.bits_left();
    }

    auto tm_sdu = vec.take_vector(bits_left);
    std::cout << "  TM-SDU: size = " << std::to_string(bits_left) << ": " << tm_sdu << std::endl;
    std::cout << "  mac_header_length = " << std::to_string(mac_header_length) << std::endl;
    std::cout << "  fill_bit_indication: 0b" << std::bitset<1>(fill_bit_indication) << std::endl;
    std::cout << "  Address: " << address << std::endl;

    if (length_indictaion != 0b111111) {
        // no fragmentation
        logical_link_control_->process(address, tm_sdu);
    } else {
        fragmentation_push_tm_sdu_start(address, tm_sdu);
    }
    // fragmentation_push_tm_sdu_start(address, tm_sdu);
}

void UpperMac::process_mac_data(BitVector& vec) {
    auto preprocessing_bit_count = vec.bits_left() + 2;

    auto fill_bit_indication = vec.take<1>();
    auto encrypted_flag = vec.take<1>();
    auto address_type = vec.take<2>();

    std::cout << "MAC DATA" << std::endl;
    std::cout << "  encrypted: 0b" << std::bitset<1>(encrypted_flag) << std::endl;

    auto address = AddressType();
    if (address_type == 0b00) {
        address.set_ssi(vec.take<24>());
    } else if (address_type == 0b01) {
        address.set_event_label(vec.take<10>());
    } else if (address_type == 0b11) {
        address.set_ussi(vec.take<24>());
    } else if (address_type == 0b11) {
        address.set_smi(vec.take<24>());
    }

    auto length_indication_or_capacity_request = vec.take<1>();
    auto fragmentation = false;
    uint64_t length_indication{};
    if (length_indication_or_capacity_request == 0b0) {
        length_indication = vec.take<6>();
        std::cout << "  length indication: 0b" << std::bitset<6>(length_indication) << std::endl;
        if (length_indication == 0b111110) {
            std::cout << "  Second half slot stolen on STCH" << std::endl;
            second_slot_stolen_ = true;
        } else if (length_indication == 0b111111) {
            std::cout << "  Second half slot stolen on STCH" << std::endl;
            std::cout << "  Start of fragmentation on STCH" << std::endl;
            second_slot_stolen_ = true;
            fragmentation = true;
        }
    } else {
        auto fragmentation_flag = vec.take<1>();
        if (fragmentation_flag == 0b1) {
            fragmentation = true;
        }
        auto reservation_requirement = vec.take<4>();
        auto reserved = vec.take<1>();

        if (reservation_requirement == 0b0000) {
            last_address_type_end_hu_ = address;
        }

        std::cout << "  fragmentation flag: 0b" << std::bitset<1>(fragmentation_flag) << std::endl;
        std::cout << "  reservation requirement: 0b" << std::bitset<4>(reservation_requirement) << std::endl;
        std::cout << "  reserved: 0b" << std::bitset<1>(reserved) << std::endl;
    }

    uint64_t bits_left = vec.bits_left();

    auto null_pdu = false;
    if (length_indication_or_capacity_request == 0b0) {
        auto mac_header_length = preprocessing_bit_count - vec.bits_left();
        bits_left = length_indication * 8 - mac_header_length;
        if (length_indication == 0) {
            bits_left = 0;
            null_pdu = true;
        }
    }

    auto tm_sdu = vec.take_vector(bits_left);
    std::cout << "  TM-SDU: size = " << std::to_string(tm_sdu.bits_left()) << ": " << tm_sdu << std::endl;
    std::cout << "  Address: " << address << std::endl;

    if (fragmentation) {
        fragmentation_push_tm_sdu_start(address, tm_sdu);
    } else if (null_pdu) {
        // only write address to last_address_type_
        last_address_type_ = address;
    } else {
        logical_link_control_->process(address, tm_sdu);
    }
}

void UpperMac::process_mac_access(BitVector& vec) {
    auto preprocessing_bit_count = vec.bits_left() + 2;

    std::cout << "MAC-ACCESS" << std::endl;
    auto encrypted_flag = vec.take<1>();
    auto address_type = vec.take<2>();

    std::cout << "  encrypted: 0b" << std::bitset<1>(encrypted_flag) << std::endl;

    auto address = AddressType();
    if (address_type == 0b00) {
        address.set_ssi(vec.take<24>());
    } else if (address_type == 0b01) {
        address.set_event_label(vec.take<10>());
    } else if (address_type == 0b11) {
        address.set_ussi(vec.take<24>());
    } else if (address_type == 0b11) {
        address.set_smi(vec.take<24>());
    }

    auto optional_field_flag = vec.take<1>();
    uint64_t length_indication{};
    uint64_t length_indication_or_capacity_request{};
    auto fragmentation = false;

    if (optional_field_flag == 0b1) {
        length_indication_or_capacity_request = vec.take<1>();
        if (length_indication_or_capacity_request == 0b0) {
            length_indication = vec.take<5>();
            std::cout << "  length indication: 0b" << std::bitset<5>(length_indication) << std::endl;
        } else {
            auto fragmentation_flag = vec.take<1>();
            if (fragmentation_flag == 0b1) {
                fragmentation = true;
            }
            auto reservation_requirement = vec.take<4>();

            if (reservation_requirement == 0b0000) {
                last_address_type_end_hu_ = address;
            }

            std::cout << "  fragmentation flag: 0b" << std::bitset<1>(fragmentation_flag) << std::endl;
            std::cout << "  reservation requirement: 0b" << std::bitset<4>(reservation_requirement) << std::endl;
        }
    }

    uint64_t bits_left = vec.bits_left();

    if (optional_field_flag == 0b1 && length_indication_or_capacity_request == 0b0) {
        auto mac_header_length = preprocessing_bit_count - vec.bits_left();
        bits_left = length_indication * 8 - mac_header_length;
    }

    auto tm_sdu = vec.take_vector(bits_left);
    std::cout << "  TM-SDU: size = " << std::to_string(tm_sdu.bits_left()) << ": " << tm_sdu << std::endl;
    std::cout << "  Address: " << address << std::endl;

    if (!fragmentation) {
        logical_link_control_->process(address, tm_sdu);
    } else {
        fragmentation_push_tm_sdu_start(address, tm_sdu);
    }
}

void UpperMac::process_mac_end_hu(BitVector& vec) {
    auto preprocessing_bit_count = vec.bits_left() + 2;
    std::cout << "MAC-END-HU" << std::endl;

    auto length_indictaion_or_capacity_request = vec.take<1>();
    uint64_t length_indictaion;
    if (length_indictaion_or_capacity_request == 0b0) {
        // TODO: parse this
        length_indictaion = vec.take<4>();
        std::cout << "  length indication: 0b" << std::bitset<4>(length_indictaion) << std::endl;
    } else {
        auto reservation_requirement = vec.take<4>();
        std::cout << "  reservation requirement: 0b" << std::bitset<4>(reservation_requirement) << std::endl;
    }

    uint64_t bits_left = vec.bits_left();

    if (length_indictaion_or_capacity_request == 0b0) {
        auto mac_header_length = preprocessing_bit_count - vec.bits_left();
        bits_left = length_indictaion * 8 - mac_header_length;
    }

    auto tm_sdu = vec.take_vector(bits_left);
    std::cout << "  TM-SDU: size = " << std::to_string(tm_sdu.bits_left()) << ": " << tm_sdu << std::endl;
    // XXX: implement combination of uplink and downlink
    std::cout << "  Last fragment sent on reserved subslot. Cannot process! Taking the last MAC-ACCESS or MAC-DATA as "
                 "the first half."
              << std::endl;

    // XXX: This just takes the last plausible MAC-DATA or MAC-ACCESS as start of fragmentation. this will be buggy, but
    // at least a start to avoid having to deal with the combination of uplink and downlink processing
    fragmentation_push_tm_sdu_end_hu(tm_sdu);
}

void UpperMac::remove_fill_bits(BitVector& vec) {
    if (remove_fill_bits_) {
        while (vec.take_last<1>() == 0b0)
            ;
    }
    remove_fill_bits_ = false;
}

auto operator<<(std::ostream& stream, const UpperMac& upperMac) -> std::ostream& {
    if (upperMac.system_info_received_) {
        stream << "SYSINFO:" << std::endl;
        stream << "  DL " << std::to_string(upperMac.downlink_frequency_) << "Hz UL "
               << std::to_string(upperMac.uplink_frequency_) << "Hz" << std::endl;
        stream << "  Number of common secondary control channels in use on CA main "
                  "carrier: ";
        switch (upperMac.number_secondary_control_channels_main_carrier_) {
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
        stream << "  MS_TXPWR_MAX_CELL: ";
        switch (upperMac.ms_txpwr_max_cell_) {
        case 0b000:
            stream << "Reserved";
            break;
        default:
            stream << std::to_string(10 + 5 * upperMac.ms_txpwr_max_cell_) << " dBm";
            break;
        }
        stream << std::endl;
        stream << "  RXLEV_ACCESS_MIN: " << std::to_string(-125 + 5 * upperMac.rxlev_access_min_) << " dBm"
               << std::endl;
        stream << "  ACCESS_PARAMETER: " << std::to_string(-53 + 2 * upperMac.access_parameter_) << " dBm" << std::endl;
        stream << "  RADIO_DOWNLINK_TIMEOUT: ";
        switch (upperMac.radio_downlink_timeout_) {
        case 0b0000:
            stream << "Disable radio downlink counter";
            break;
        default:
            stream << std::to_string(144 * upperMac.radio_downlink_timeout_) << " timeslots";
            break;
        }
        stream << std::endl;
        if (upperMac.hyper_frame_cipher_key_flag_) {
            stream << "  Common cipher key identifier: " << std::to_string(upperMac.hyper_frame_cipher_key_flag_)
                   << std::endl;
        } else {
            stream << "  Cyclic count of hyperframes: " << std::to_string(upperMac.hyper_frame_number_) << std::endl;
        }

        if (upperMac.even_multi_frame_definition_for_ts_mode_.has_value()) {
            stream << "  Bit map of common frames for TS mode (even multiframes): 0b"
                   << std::bitset<20>(*upperMac.even_multi_frame_definition_for_ts_mode_) << std::endl;
        }
        if (upperMac.odd_multi_frame_definition_for_ts_mode_.has_value()) {
            stream << "  Bit map of common frames for TS mode (odd multiframes): 0b"
                   << std::bitset<20>(*upperMac.odd_multi_frame_definition_for_ts_mode_) << std::endl;
        }

        if (upperMac.defaults_for_access_code_a_.system_info_) {
            stream << "  Default definition for access code A information element:" << std::endl;
            stream << "    Immediate: 0b" << std::bitset<4>(upperMac.extended_service_broadcast_.security_information_)
                   << std::endl;
            stream << "    Waiting time: 0b" << std::bitset<4>(upperMac.defaults_for_access_code_a_.waiting_time_)
                   << std::endl;
            stream << "    Number of random access transmissions on uplink: 0b"
                   << std::bitset<4>(
                          upperMac.defaults_for_access_code_a_.number_of_random_access_transmissions_on_up_link_)
                   << std::endl;
            stream << "    Frame-length factor: 0b"
                   << std::bitset<1>(upperMac.defaults_for_access_code_a_.frame_length_factor_) << std::endl;
            stream << "    Timeslot pointer: 0b"
                   << std::bitset<4>(upperMac.defaults_for_access_code_a_.timeslot_pointer_) << std::endl;
            stream << "    Minimum PDU priority: 0b"
                   << std::bitset<3>(upperMac.defaults_for_access_code_a_.minimum_pdu_priority_) << std::endl;
        }

        if (upperMac.defaults_for_access_code_a_.system_info_) {
            stream << "  Extended services broadcast:" << std::endl;
            stream << "    Security information: 0b"
                   << std::bitset<8>(upperMac.extended_service_broadcast_.security_information_) << std::endl;
            stream << "    SDS-TL addressing method: 0b"
                   << std::bitset<2>(upperMac.extended_service_broadcast_.sdstl_addressing_method_) << std::endl;
            stream << "    GCK supported: 0b" << std::bitset<1>(upperMac.extended_service_broadcast_.gck_supported_)
                   << std::endl;
            if (upperMac.extended_service_broadcast_.system_info_section_1_) {
                stream << "    Data priority supported: 0b"
                       << std::bitset<1>(upperMac.extended_service_broadcast_.data_priority_supported_) << std::endl;
                stream << "    Extended advanced links and MAC-U-BLCK supported: 0b"
                       << std::bitset<1>(
                              upperMac.extended_service_broadcast_.extended_advanced_links_and_max_ublck_supported_)
                       << std::endl;
                stream << "    QoS negotiation supported: 0b"
                       << std::bitset<1>(upperMac.extended_service_broadcast_.qos_negotiation_supported_) << std::endl;
                stream << "    D8PSK service: 0b" << std::bitset<1>(upperMac.extended_service_broadcast_.d8psk_service_)
                       << std::endl;
            }

            if (upperMac.extended_service_broadcast_.system_info_section_2_) {
                stream << "    25 kHz QAM service: 0b"
                       << std::bitset<1>(upperMac.extended_service_broadcast_.service_25Qam_) << std::endl;
                stream << "    50 kHz QAM service: 0b"
                       << std::bitset<1>(upperMac.extended_service_broadcast_.service_50Qam_) << std::endl;
                stream << "    100 kHz QAM service: 0b"
                       << std::bitset<1>(upperMac.extended_service_broadcast_.service_100Qam_) << std::endl;
                stream << "    150 kHz QAM service: 0b"
                       << std::bitset<1>(upperMac.extended_service_broadcast_.service_150Qam_) << std::endl;
            }
        }
    }

    return stream;
}
