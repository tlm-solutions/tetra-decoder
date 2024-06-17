/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "reporter.hpp"
#include "utils/address.hpp"
#include "utils/bit_vector.hpp"
#include "utils/packet_counter_metrics.hpp"
#include <array>
#include <string>

class ShortDataService {
  public:
    ShortDataService() = delete;
    ShortDataService(const std::shared_ptr<PrometheusExporter>& prometheus_exporter, Reporter&& reporter)
        : reporter_(reporter) {
        sds_pdu_description_[0] = "Reserved 0";
        sds_pdu_description_[1] = "OTAK (Over The Air re-Keying for end to end encryption)";
        sds_pdu_description_[2] = "Simple Text Messaging";
        sds_pdu_description_[3] = "Simple location system";
        sds_pdu_description_[4] = "Wireless Datagram Protocol WAP 4";
        sds_pdu_description_[5] = "Wireless Control Message Protocol WCMP 5";
        sds_pdu_description_[6] = "M-DMO (Managed DMO) 6";
        sds_pdu_description_[7] = "PIN authentication";
        sds_pdu_description_[8] = "End-to-end encrypted message 8";
        sds_pdu_description_[9] = "Simple immediate text messaging";
        sds_pdu_description_[10] = "Location information protocol";
        sds_pdu_description_[11] = "Net Assist Protocol 2 (NAP2)";
        sds_pdu_description_[12] = "Concatenated SDS message 12";
        sds_pdu_description_[13] = "DOTAM";
        sds_pdu_description_[14] = "Simple AGNSS service";
        for (auto i = 0b00001111; i <= 0b00111111; i++) {
            sds_pdu_description_[i] = "Reserved for future standard definition " + std::to_string(i);
        }
        for (auto i = 0b01000000; i <= 0b01111110; i++) {
            sds_pdu_description_[i] = "Available for user application definition " + std::to_string(i);
        }
        sds_pdu_description_[127] = "Reserved for extension 127";
        sds_pdu_description_[128] = "Reserved 128";
        sds_pdu_description_[129] = "Reserved 129";
        sds_pdu_description_[130] = "Text Messaging";
        sds_pdu_description_[131] = "Location system";
        sds_pdu_description_[132] = "Wireless Datagram Protocol WAP 132";
        sds_pdu_description_[133] = "Wireless Control Message Protocol WCMP 133";
        sds_pdu_description_[134] = "M-DMO (Managed DMO) 134";
        sds_pdu_description_[135] = "Reserved for future standard definition 135";
        sds_pdu_description_[136] = "End-to-end encrypted message 136";
        sds_pdu_description_[137] = "Immediate text messaging";
        sds_pdu_description_[138] = "Message with User Data Header";
        sds_pdu_description_[139] = "Reserved for future standard definition 139";
        sds_pdu_description_[140] = "Concatenated SDS message 140";
        sds_pdu_description_[141] = "AGNSS service";
        for (auto i = 0b10001110; i <= 0b10111111; i++) {
            sds_pdu_description_[i] = "Reserved for future standard definition " + std::to_string(i);
        }
        for (auto i = 0b11000000; i <= 0b11111110; i++) {
            sds_pdu_description_[i] = "Available for user application definition " + std::to_string(i);
        }
        sds_pdu_description_[255] = "Reserved for extension";

        if (prometheus_exporter) {
            metrics_ = std::make_unique<PacketCounterMetrics>(prometheus_exporter, "short_data_service");
        }
    };
    ~ShortDataService() noexcept = default;

    void process(Address to_address, Address from_address, BitVector& vec);

  private:
    void process_simple_text_messaging(Address to_address, Address from_address, BitVector& vec);
    void process_location_information_protocol(Address to_address, Address from_address, BitVector& vec);
    void process_default(Address to_address, Address from_address, BitVector& vec);

    nlohmann::json message_;
    Reporter reporter_;

    std::array<std::string, 64> sds_pdu_description_;

    std::unique_ptr<PacketCounterMetrics> metrics_;
};