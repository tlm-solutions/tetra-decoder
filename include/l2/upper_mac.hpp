/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#ifndef L2_UPPERMAC_HPP
#define L2_UPPERMAC_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include <burst_type.hpp>
#include <l2/logical_link_control.hpp>
#include <l3/mobile_link_entity.hpp>
#include <reporter.hpp>
#include <utils/address_type.hpp>

enum DownlinkUsage { CommonControl, Unallocated, AssignedControl, CommonAndAssignedControl, Traffic };

class UpperMac {
  public:
    UpperMac(std::shared_ptr<Reporter> reporter)
        : reporter_(reporter)
        , mobile_link_entity_(std::make_shared<MobileLinkEntity>(reporter_))
        , logical_link_control_(std::make_unique<LogicalLinkControl>(reporter_, mobile_link_entity_)){};
    ~UpperMac() noexcept = default;
    ;

    void incrementTn();
    void set_scrambling_code(unsigned int scrambling_code) { scrambling_code_ = scrambling_code; };

    void processAACH(const BurstType burst_type, const std::vector<uint8_t>& data);
    void processBSCH(const BurstType burst_type, const std::vector<uint8_t>& data);
    void processSCH_HD(const BurstType burst_type, const std::vector<uint8_t>& data);
    void processSCH_HU(const BurstType burst_type, const std::vector<uint8_t>& data);
    void processSCH_F(const BurstType burst_type, const std::vector<uint8_t>& data);
    void processSTCH(const BurstType burst_type, const std::vector<uint8_t>& data);

    [[nodiscard]] auto scrambling_code() const noexcept -> uint32_t { return scrambling_code_; }
    [[nodiscard]] auto color_code() const noexcept -> uint16_t { return color_code_; }
    [[nodiscard]] auto downlink_frequency() const noexcept -> int32_t { return downlink_frequency_; }
    [[nodiscard]] auto uplink_frequency() const noexcept -> int32_t { return uplink_frequency_; }
    [[nodiscard]] auto downlink_usage() const noexcept -> DownlinkUsage { return downlink_usage_; }
    [[nodiscard]] auto downlink_traffic_usage_marker() const noexcept -> int { return downlink_traffic_usage_marker_; }
    [[nodiscard]] auto second_slot_stolen() const noexcept -> bool { return second_slot_stolen_; }

    [[nodiscard]] auto time_slot() const noexcept -> uint16_t { return time_slot_; }
    [[nodiscard]] auto frame_number() const noexcept -> uint16_t { return frame_number_; }
    [[nodiscard]] auto multi_frame_number() const noexcept -> uint16_t { return multi_frame_number_; }

    friend std::ostream& operator<<(std::ostream& stream, const UpperMac& upperMac);

  private:
    std::shared_ptr<Reporter> reporter_;

    void process_signalling_channel(const BurstType burst_type, const std::vector<uint8_t>& data, bool isHalfChannel,
                                    bool isStolenChannel);
    void process_signalling_channel(const BurstType burst_type, BitVector& vec, bool isHalfChannel,
                                    bool isStolenChannel);

    void update_scrambling_code();

    void process_broadcast(BitVector& vec);
    void process_supplementary_mac_pdu(const BurstType burst_type, BitVector& vec);

    // TMA-SAP Uplink
    void process_mac_access(BitVector& vec);
    void process_mac_end_hu(BitVector& vec);
    void process_mac_data(BitVector& vec);
    void process_mac_frag_uplink(BitVector& vec);
    void process_mac_end_uplink(BitVector& vec);
    void process_mac_u_blck(BitVector& vec);
    // TMA-SAP Downlink
    void process_mac_resource(BitVector& vec);
    void process_mac_frag_downlink(BitVector& vec);
    void process_mac_end_downlink(BitVector& vec);
    void process_mac_d_blck(BitVector& vec);
    // TMB-SAP broadcast
    void process_system_info_pdu(BitVector& vec);
    static void process_access_define_pdu(BitVector& vec);
    void process_system_info_da(BitVector& vec){};
    // TMD-SAP
    void process_mac_usignal(BitVector& vec);

    // fragmentation
    // XXX: might have to delay processing as SSI may only be known after the Null PDU
    void fragmentation_start_burst();
    void fragmentation_end_burst();
    void fragmentation_push_tm_sdu_start(AddressType address_type, BitVector& vec);
    void fragmentation_push_tm_sdu_frag(BitVector& vec);
    void fragmentation_push_tm_sdu_end(BitVector& vec);
    void fragmentation_push_tm_sdu_end_hu(BitVector& vec);

    void remove_fill_bits(BitVector& vec);
    bool remove_fill_bits_{};

    // SYNC PDU
    bool sync_received_ = false;
    uint8_t system_code_{};
    uint32_t color_code_ = 0;
    // time slot
    uint16_t time_slot_ = 1;
    // frame number
    uint16_t frame_number_ = 1;
    // multi frame number
    uint16_t multi_frame_number_ = 1;
    uint8_t sharing_mode_{};
    uint8_t time_slot_reserved_frames_{};
    uint8_t up_lane_dtx_{};
    uint8_t frame_18_extension_{};

    uint32_t scrambling_code_ = 0;

    // SYSINFO PDU
    bool system_info_received_ = false;
    int32_t downlink_frequency_ = 0;
    int32_t uplink_frequency_ = 0;
    uint8_t number_secondary_control_channels_main_carrier_{};
    uint8_t ms_txpwr_max_cell_{};
    uint8_t rxlev_access_min_{};
    uint8_t access_parameter_{};
    uint8_t radio_downlink_timeout_{};
    uint8_t hyper_frame_cipher_key_flag_{};
    uint8_t hyper_frame_number_{};
    uint8_t common_cipher_key_identifier_or_static_cipher_key_version_number_{};
    std::optional<uint32_t> even_multi_frame_definition_for_ts_mode_;
    std::optional<uint32_t> odd_multi_frame_definition_for_ts_mode_;

    struct DefaultDefinitionForAccessCodeA {
        bool system_info_ = false;
        uint8_t immediate_ = 0;
        uint8_t waiting_time_ = 0;
        uint8_t number_of_random_access_transmissions_on_up_link_ = 0;
        uint8_t frame_length_factor_ = 0;
        uint8_t timeslot_pointer_ = 0;
        uint8_t minimum_pdu_priority_ = 0;
    } defaults_for_access_code_a_;

    struct ExtendedServiceBroadcast {
        // TODO: I dont know how this works but maybe smart making for the sections
        //  separate structs which then live in std::optionals
        bool system_info_ = false;
        uint8_t security_information_ = 0;
        uint8_t sdstl_addressing_method_ = 0;
        uint8_t gck_supported_ = 0;

        bool system_info_section_1_ = false;
        uint8_t data_priority_supported_ = 0;
        uint8_t extended_advanced_links_and_max_ublck_supported_ = 0;
        uint8_t qos_negotiation_supported_ = 0;
        uint8_t d8psk_service_ = 0;

        bool system_info_section_2_ = false;
        uint8_t service_25Qam_ = 0;
        uint8_t service_50Qam_ = 0;
        uint8_t service_100Qam_ = 0;
        uint8_t service_150Qam_ = 0;
    } extended_service_broadcast_;

    // AACH
    enum DownlinkUsage downlink_usage_;
    int downlink_traffic_usage_marker_{};

    // STCH
    bool second_slot_stolen_{};

    std::shared_ptr<MobileLinkEntity> mobile_link_entity_;
    std::unique_ptr<LogicalLinkControl> logical_link_control_;

    // hashmap to keep track of framented mac segments
    std::unordered_map<AddressType, std::vector<BitVector>> fragment_map_ = {};
    std::vector<BitVector> fragment_list_{};
    bool fragment_end_received_{};
    bool fragment_end_hu_received_{};
    AddressType last_address_type_{};
    // save the last MAC-ACCESS or MAC-DATA where reservation_requirement is 0b0000 (1 sublot) for END-HU
    AddressType last_address_type_end_hu_{};
};

std::ostream& operator<<(std::ostream& stream, const UpperMac& upperMac);

#endif
