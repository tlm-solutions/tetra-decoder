#include <bitset>
#include <cassert>

#include <l2/upper_mac.hpp>
#include <utils/bit_vector.hpp>

void UpperMac::incrementTn() {
    time_slot_++;

    // TODO: remove magic numbers
    // time slot
    if (time_slot_ > 4) {
        frame_number_++;
        time_slot_ = 1;
    }

    // frame number
    if (frame_number_ > 18) {
        multi_frame_number_++;
        frame_number_ = 1;
    }

    // multi-frame number
    if (multi_frame_number_ > 60) {
        multi_frame_number_ = 1;
    }

    std::cout << "TN: " << time_slot_ << " FN: " << frame_number_ << " MN: " << multi_frame_number_ << std::endl;
}

/**
 * @brief Process ACCESS-ASSIGN - see 21.4.7.2
 *
 */
void UpperMac::processAACH(const std::vector<uint8_t>& data) {
    assert(data.size() == 14);

    auto vec = BitVector(data);

    auto header = vec.take(2);
    auto field1 = vec.take(6);
    auto _field2 = vec.take(6);

    // TODO: parse uplink marker and some other things relevant for the uplink
    if (frame_number_ == 18) {
        downlink_usage_ = DownlinkUsage::CommonControl;
    } else {
        if (header == 0b00) {
            downlink_usage_ = DownlinkUsage::CommonControl;
        } else {
            switch (field1) {
            case 0b00000:
                downlink_usage_ = DownlinkUsage::Unallocated;
                break;
            case 0b00001:
                downlink_usage_ = DownlinkUsage::AssignedControl;
                break;
            case 0b00010:
                downlink_usage_ = DownlinkUsage::CommonControl;
                break;
            case 0b00011:
                downlink_usage_ = DownlinkUsage::CommonAndAssignedControl;
                break;
            default:
                downlink_usage_ = DownlinkUsage::Traffic;
                downlink_traffic_usage_marker_ = field1;
                break;
            }
        }
    }

    std::cout << "AACH downlink_usage: " << downlink_usage_
              << " downlinkUsageTrafficMarker: " << downlink_traffic_usage_marker_ << std::endl;
}

/**
 * @brief Process SYNC - see 21.4.4.2
 *
 */
void UpperMac::processBSCH(const std::vector<uint8_t>& data) {
    assert(data.size() == 60);

    auto vec = BitVector(data);

    assert(vec.bits_left() == 60);

    system_code_ = vec.take(4);
    color_code_ = vec.take(6);
    time_slot_ = vec.take(2) + 1;
    frame_number_ = vec.take(5);
    multi_frame_number_ = vec.take(6);
    sharing_mode_ = vec.take(2);
    time_slot_reserved_frames_ = vec.take(3);
    up_lane_dtx_ = vec.take(1);
    frame_18_extension_ = vec.take(1);
    auto _reserved = vec.take(1);

    assert(vec.bits_left() == 29);

    mobile_link_entity_->service_DMle_sync(vec);
    update_scrambling_code();

    sync_received_ = true;

    //  std::cout << *this;
}

void UpperMac::processSCH_HD(const std::vector<uint8_t>& data) {
    std::cout << "SCH_HD" << std::endl;
    process_signalling_channel(data, true, false);
}

void UpperMac::processSCH_F(const std::vector<uint8_t>& data) {
    std::cout << "SCH_F" << std::endl;
    process_signalling_channel(data, false, false);
}

void UpperMac::processSTCH(const std::vector<uint8_t>& data) {
    std::cout << "STCH" << std::endl;

    second_slot_stolen_ = false;

    process_signalling_channel(data, true, true);
}

void UpperMac::process_signalling_channel(const std::vector<uint8_t>& data, bool isHalfChannel, bool isStolenChannel) {
    auto vec = BitVector(data);

    remove_fill_bits_ = true;
    try {
        process_signalling_channel(vec, isHalfChannel, isStolenChannel);
    } catch (std::exception& e) {
        std::cout << "Error with decoding: " << e.what() << std::endl;
    }
}

void UpperMac::process_signalling_channel(BitVector& vec, bool isHalfChannel, bool isStolenChannel) {
    auto pduType = vec.take(2);

    if (pduType == 0b00) {
        // MAC-RESOURCE (downlink) or MAC-DATA (uplink)
        // TMA-SAP
        process_mac_resource(vec);
    } else if (pduType == 0b01) {
        // MAC-END or MAC-FRAG
        // TMA-SAP
        auto subtype = vec.take(1);
        if (subtype == 0b0) {
            process_mac_frag(vec);
        } else if (subtype == 0b1) {
            process_mac_end(vec);
        }
    } else if (pduType == 0b10) {
        // Broadcast
        // TMB-SAP
        // ✅ done
        process_broadcast(vec);
    } else if (pduType == 0b11) {
        // Supplementary MAC PDU (not on STCH, SCH/HD or SCH-P8/HD)
        // MAC-U-SIGNAL (only on STCH)
        // TMA-SAP or TMD-SAP
        if (isHalfChannel && isStolenChannel) {
            process_mac_usignal(vec);
        } else if ((!isHalfChannel) && (!isStolenChannel)) {
            process_supplementary_mac_pdu(vec);
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
            process_signalling_channel(vec, isHalfChannel, isStolenChannel);
        }
    }
}

void UpperMac::process_broadcast(BitVector& vec) {
    auto broadcastType = vec.take(2);
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
    default:
        // TODO: what should happen here panic ?
        break;
    }
}

void UpperMac::process_supplementary_mac_pdu(BitVector& vec) {
    auto subtype = vec.take(1);

    switch (subtype) {
    case 0b0:
        process_mac_dblck(vec);
        break;
    case 0b1:
        // Reserved
        // TODO: print unimplemented error
        break;
    default:
        // TODO: what should happen ?
        break;
    }
}

void UpperMac::process_system_info_pdu(BitVector& vec) {
    auto mainCarrier = vec.take(12);
    auto frequency_band = vec.take(4);
    auto offset = vec.take(2);
    auto duplexSpacing = vec.take(3);
    auto reverseOperation = vec.take(1);
    number_secondary_control_channels_main_carrier_ = vec.take(2);
    ms_txpwr_max_cell_ = vec.take(3);
    rxlev_access_min_ = vec.take(4);
    access_parameter_ = vec.take(4);
    radio_downlink_timeout_ = vec.take(4);
    hyper_frame_cipher_key_flag_ = vec.take(1);
    if (hyper_frame_cipher_key_flag_ == 0) {
        hyper_frame_number_ = vec.take(16);
    } else {
        common_cipher_key_identifier_or_static_cipher_key_version_number_ = vec.take(16);
    }
    auto optionalFieldFlag = vec.take(2);
    if (optionalFieldFlag == 0b00) {
        *even_multi_frame_definition_for_ts_mode_ = vec.take(20);
    } else if (optionalFieldFlag == 0b01) {
        *odd_multi_frame_definition_for_ts_mode_ = vec.take(20);
    } else if (optionalFieldFlag == 0b10) {
        defaults_for_access_code_a_.immediate_ = vec.take(4);
        defaults_for_access_code_a_.waiting_time_ = vec.take(4);
        defaults_for_access_code_a_.number_of_random_access_transmissions_on_up_link_ = vec.take(4);
        defaults_for_access_code_a_.frame_length_factor_ = vec.take(1);
        defaults_for_access_code_a_.timeslot_pointer_ = vec.take(4);
        defaults_for_access_code_a_.minimum_pdu_priority_ = vec.take(3);
        defaults_for_access_code_a_.system_info_ = true;
    } else if (optionalFieldFlag == 0b11) {
        extended_service_broadcast_.security_information_ = vec.take(8);
        extended_service_broadcast_.sdstl_addressing_method_ = vec.take(2);
        extended_service_broadcast_.gck_supported_ = vec.take(1);
        auto section = vec.take(2);
        if (section == 0b00) {
            extended_service_broadcast_.data_priority_supported_ = vec.take(1);
            extended_service_broadcast_.extended_advanced_links_and_max_ublck_supported_ = vec.take(1);
            extended_service_broadcast_.qos_negotiation_supported_ = vec.take(1);
            extended_service_broadcast_.d8psk_service_ = vec.take(1);
            auto _sectionInformation = vec.take(3);
            extended_service_broadcast_.system_info_section_1_ = true;
        } else if (section == 0b01) {
            extended_service_broadcast_.service_25Qam_ = vec.take(1);
            extended_service_broadcast_.service_50Qam_ = vec.take(1);
            extended_service_broadcast_.service_100Qam_ = vec.take(1);
            extended_service_broadcast_.service_150Qam_ = vec.take(1);
            auto _reserved = vec.take(3);
            extended_service_broadcast_.system_info_section_2_ = true;
        } else {
            // TODO: Section 2 and 3 are reserved
            auto _reserved = vec.take(7);
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
    // auto reserved = vec.take(28);

    mobile_link_entity_->service_DMle_system_info(vec);

    system_info_received_ = true;
}

void UpperMac::process_access_define_pdu(BitVector& vec) {
    auto _ = vec.take(23);
    auto optional_field_flag = vec.take(2);
    if (optional_field_flag == 0b01) {
        auto _subscriber_class_bitmap = vec.take(16);
    } else if (optional_field_flag == 0b10) {
        auto _gssi = vec.take(24);
    }
    auto _filter_bits = vec.take(3);
}

void UpperMac::process_mac_usignal(BitVector& vec) {
    second_slot_stolen_ = (vec.take(1) == 1);

    // TODO: TM-SDU
    auto tm_sdu = BitVector(vec.take_vector(vec.bits_left()));
    std::cout << "MAC U-SIGNAL" << std::endl;
    std::cout << "  TM-SDU: size = " << std::to_string(tm_sdu.bits_left()) << ": " << tm_sdu << std::endl;
}

void UpperMac::process_mac_dblck(BitVector& vec) {
    auto fill_bit_indication = vec.take(1);
    auto encrypted = vec.take(2);
    auto address = vec.take(10);
    auto immediate_napping_permission_flag = vec.take(1);
    auto slot_granting_flag = vec.take(1);
    auto basic_slot_granting_element = 0;
    if (slot_granting_flag == 0b1) {
        basic_slot_granting_element = vec.take(8);
    }

    if (fill_bit_indication == 0b1) {
        remove_fill_bits(vec);
    }

    // TODO: TM-SDU
    auto tm_sdu = BitVector(vec.take_vector(vec.bits_left()));
    std::cout << "MAC D-BLCK" << std::endl;
    std::cout << "  TM-SDU: size = " << std::to_string(tm_sdu.bits_left()) << ": " << tm_sdu << std::endl;
}

void UpperMac::process_mac_frag(BitVector& vec) {
    auto fill_bit_indication = vec.take(1);

    if (fill_bit_indication == 0b1) {
        remove_fill_bits(vec);
    }

    // TODO: TM-SDU
    auto tm_sdu = BitVector(vec.take_vector(vec.bits_left()));
    std::cout << "MAC FRAG" << std::endl;
    std::cout << "  TM-SDU: size = " << std::to_string(tm_sdu.bits_left()) << ": " << tm_sdu << std::endl;
}

void UpperMac::process_mac_end(BitVector& vec) {
    second_slot_stolen_ = false;

    auto pre_processing_bit_count = vec.bits_left() + 3;

    auto fill_bit_indication = vec.take(1);
    auto position_of_grant = vec.take(1);
    auto length_indictaion = vec.take(6);
    // The immediate napping permission flag shall be present when the PDU is sent
    // using π/8-D8PSK or QAM modulation. It shall not be present when the PDU is
    // sent using π/4-DQPSK modulation. auto immediateNapping = vec.take(1);
    auto slot_granting_flag = vec.take(1);
    // The multiple slot granting flag shall be present when the slot granting
    // flag is set to 1 and the PDU is sent using QAM modulation. It shall not be
    // present when the slot granting flag is set to 0 or the PDU is sent using
    // π/4-DQPSK or π/8-D8PSK modulation auto multipleSlotGranting = vec.take(1);
    // The basic slot granting element shall be present when the slot granting
    // flag is set to 1 and either the PDU is sent using π/4-DQPSK or π/8-D8PSK
    // modulation, or the PDU is sent using QAM modulation and the multiple slot
    // granting flag is set to 0.
    auto basic_slot_granting_element = 0;
    if (slot_granting_flag == 0b1) {
        basic_slot_granting_element = vec.take(8);
    }
    auto channel_allocation_flag = vec.take(1);
    if (channel_allocation_flag == 0b1) {
        auto channel_allocation1 = vec.take(22);
        auto extended_carrier_numbering_flag = vec.take(1);
        if (extended_carrier_numbering_flag == 0b1) {
            auto channel_allocation2 = vec.take(9);
        }
        auto monitoring_pattern = vec.take(2);
        if (monitoring_pattern == 0b00) {
            auto frame18_monitoring_pattern = vec.take(2);
            auto up_downlink_assigned_for_augmented_channel_allocation = vec.take(2);
            auto bandwidth_of_allocated_channel = vec.take(3);
            auto modulation_mode_of_allocated_channel = vec.take(3);
            if (modulation_mode_of_allocated_channel == 0b010) {
                auto maximum_uplink_qam_modulation_level = vec.take(3);
                auto reserved = vec.take(3);
            }
            auto channel_allocation3 = vec.take(12);
            auto napping_status = vec.take(2);
            if (napping_status == 0b01) {
                auto napping_information = vec.take(11);
            }
            auto reserved = vec.take(4);

            // taking optional element a
            auto conditional_element_a_flag = vec.take(1);
            if (conditional_element_a_flag == 0b1) {
                auto conditional_element_a = vec.take(16);
            }

            // taking optional element b
            auto conditional_element_b_flag = vec.take(1);
            if (conditional_element_b_flag == 0b1) {
                auto conditional_element_b = vec.take(16);
            }
            auto further_augmentation_flag = vec.take(1);
        }
    }

    auto mac_header_length = pre_processing_bit_count - vec.bits_left();
    auto bits_left = length_indictaion * 8 - mac_header_length;

    // TODO: TM-SDU
    auto tmSdu = BitVector(vec.take_vector(bits_left));
    std::cout << "MAC END" << std::endl;
    std::cout << "  TM-SDU: size = " << std::to_string(bits_left) << ": " << tmSdu << std::endl;
    std::cout << "  mac_header_length = " << std::to_string(mac_header_length) << std::endl;
    std::cout << "  fill_bit_indication: 0b" << std::bitset<1>(fill_bit_indication) << std::endl;
}

void UpperMac::process_mac_resource(BitVector& vec) {
    std::cout << "MAC RESOURCE" << std::endl;

    auto preprocessing_bit_count = vec.bits_left() + 2;

    auto fill_bit_indication = vec.take(1);
    auto position_of_grant = vec.take(1);
    auto encryption_mode = vec.take(2);
    auto random_access_flag = vec.take(1);
    auto length_indictaion = vec.take(6);
    if (length_indictaion == 0b111110 || length_indictaion == 0b111111) {
        second_slot_stolen_ = true;
    }
    std::cout << "  length_indictaion: 0b" << std::bitset<6>(length_indictaion) << std::endl;
    auto address_type = vec.take(3);
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
        auto ssi = vec.take(24);
        std::cout << "  SSI: " << std::to_string(ssi) << std::endl;
    } else if (address_type == 0b010) {
        auto event_label = vec.take(10);
    } else if (address_type == 0b011) {
        auto ussi = vec.take(24);
        std::cout << "  USSI: " << std::to_string(ussi) << std::endl;
    } else if (address_type == 0b100) {
        auto smi = vec.take(24);
        std::cout << "  SMI: " << std::to_string(smi) << std::endl;
    } else if (address_type == 0b101) {
        auto ssi = vec.take(24);
        auto event_label = vec.take(10);
        std::cout << "  SSI: " << std::to_string(ssi) << std::endl;
        std::cout << "  EventLabel: " << std::to_string(event_label) << std::endl;
    } else if (address_type == 0b110) {
        auto ssi = vec.take(24);
        auto usage_marker = vec.take(6);
        std::cout << "  SSI: " << std::to_string(ssi) << std::endl;
        std::cout << "  UsageMarker: " << std::to_string(usage_marker) << std::endl;
    } else if (address_type == 0b111) {
        auto smi = vec.take(24);
        auto event_label = vec.take(10);
        std::cout << "  SMI: " << std::to_string(smi) << std::endl;
        std::cout << "  EventLabel: " << std::to_string(event_label) << std::endl;
    }
    // The immediate napping permission flag shall be present when the PDU is sent
    // using π/8-D8PSK or QAM modulation. It shall not be present when the PDU is
    // sent using π/4-DQPSK modulation. auto immediateNappingPermisionFlag =
    // vec.take(1);
    auto power_control_flag = vec.take(1);
    if (power_control_flag == 0b1) {
        auto power_control_element = vec.take(4);
    }
    auto slot_granting_flag = vec.take(1);
    // The multiple slot granting flag shall be present when the slot granting
    // flag is set to 1 and the PDU is sent using QAM modulation. It shall not be
    // present when the slot granting flag is set to 0 or the PDU is sent using
    // π/4-DQPSK or π/8-D8PSK modulation. auto multipleSlotGrantingFlag =
    // vec.take(1); The basic slot granting element shall be present when the slot
    // granting flag is set to 1 and either the PDU is sent using π/4-DQPSK or
    // π/8-D8PSK modulation, or the PDU is sent using QAM modulation and the
    // multiple slot granting flag is set to 0.
    if (slot_granting_flag == 0b1) {
        auto basic_slot_granting_element = vec.take(8);
    }
    auto channel_allocation_flag = vec.take(1);
    if (channel_allocation_flag == 0b1) {
        auto allocation_type = vec.take(2);
        auto time_slot_assigned = vec.take(4);
        auto up_downlink_assigned = vec.take(2);
        auto clch_permission = vec.take(1);
        auto cell_change_flag = vec.take(1);
        auto carrier_number = vec.take(12);
        auto extended_carrier_numbering_flag = vec.take(1);
        if (extended_carrier_numbering_flag == 0b1) {
            auto channel_allocation_2 = vec.take(10);
        }
        auto monitoring_pattern = vec.take(2);
        if (monitoring_pattern == 0b00) {
            auto frame18_monitoring_pattern = vec.take(2);
        }
        if (up_downlink_assigned == 0b00) {
            auto up_downlink_assigned_for_augmented_channel_allocation = vec.take(2);
            auto bandwidth_of_allocated_channel = vec.take(3);
            auto modulation_mode_of_allocated_channel = vec.take(3);
            if (modulation_mode_of_allocated_channel == 0b010) {
                auto maximumUplinkQamModulationLevel = vec.take(3);
            } else {
                auto reserved = vec.take(3);
            }
            auto channel_allocation3 = vec.take(12);
            auto napping_status = vec.take(2);
            if (napping_status == 0b01) {
                auto napping_information = vec.take(11);
            }
            auto reserved = vec.take(4);
            auto conditional_element_a_flag = vec.take(1);
            if (conditional_element_a_flag == 0b1) {
                auto conditional_element_a = vec.take(16);
            }
            auto conditional_element_b_flag = vec.take(1);
            if (conditional_element_b_flag == 0b1) {
                auto conditional_element_b = vec.take(16);
            }
            auto further_augmentation_flag = vec.take(1);
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

    if (length_indictaion == 0b111111) {
        bits_left = vec.bits_left();
    }

    // TODO: TM-SDU
    auto tm_sdu = BitVector(vec.take_vector(bits_left));
    std::cout << "  TM-SDU: size = " << std::to_string(bits_left) << ": " << tm_sdu << std::endl;
    std::cout << "  mac_header_length = " << std::to_string(mac_header_length) << std::endl;
    std::cout << "  fill_bit_indication: 0b" << std::bitset<1>(fill_bit_indication) << std::endl;
}

void UpperMac::remove_fill_bits(BitVector& vec) {
    if (remove_fill_bits_) {
        while (vec.take_last() == 0b0);
    }
    remove_fill_bits_ = false;
}

void UpperMac::update_scrambling_code() {
    // 10 MSB of MCC
    uint16_t lmcc = mobile_link_entity_->mobile_country_code() & 0x03ff;
    // 14 MSB of MNC
    uint16_t lmnc = mobile_link_entity_->mobile_network_code() & 0x3fff;
    // 6 MSB of ColorCode
    uint16_t lcolor_code = color_code_ & 0x003f;

    // 30 MSB bits
    scrambling_code_ = lcolor_code | (lmnc << 6) | (lmcc << 20);
    // scrambling initialized to 1 on bits 31-32 - 8.2.5.2 (54)
    scrambling_code_ = (scrambling_code_ << 2) | 0x0003;
}

std::ostream& operator<<(std::ostream& stream, const UpperMac& upperMac) {
    if (upperMac.sync_received_) {
        stream << "SYNC:" << std::endl;
        stream << "  System code: 0b" << std::bitset<4>(upperMac.system_code_) << std::endl;
        stream << "  Color code: " << std::to_string(upperMac.color_code_) << std::endl;
        stream << "  TN/FN/MN: " << std::to_string(upperMac.time_slot_) << "/" << std::to_string(upperMac.frame_number_)
               << "/" << std::to_string(upperMac.multi_frame_number_) << std::endl;
        stream << "  Scrambling code: " << std::to_string(upperMac.scrambling_code_) << std::endl;
        std::string sharing_mode_map[] = {"Continuous transmission", "Carrier sharing", "MCCH sharing",
                                          "Traffic carrier sharing"};
        stream << "  Sharing mode: " << sharing_mode_map[upperMac.sharing_mode_] << std::endl;
        uint8_t ts_reserved_frames_map[] = {1, 2, 3, 4, 6, 9, 12, 18};
        stream << "  TS reserved frames: "
               << std::to_string(ts_reserved_frames_map[upperMac.time_slot_reserved_frames_])
               << " frames reserved per 2 multiframes" << std::endl;
        stream << "  "
               << (upperMac.up_lane_dtx_ ? "Discontinuous U-plane transmission is allowed"
                                         : "Discontinuous U-plane transmission is not allowed")
               << std::endl;
        stream << "  " << (upperMac.frame_18_extension_ ? "Frame 18 extension allowed" : "No frame 18 extension")
               << std::endl;
    }

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
