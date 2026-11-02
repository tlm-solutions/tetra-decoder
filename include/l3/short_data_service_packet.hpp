/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l3/circuit_mode_control_entity_packet.hpp"
#include "utils/bit_vector.hpp"
#include <cstddef>
#include <optional>

struct ShortLocationReport {
    unsigned _BitInt(2) time_elapsed_{};
    double longitude_{};
    double latitude_{};
    std::string position_error_;
    double horizontal_velocity_{};
    std::string direction_of_travel_;
    unsigned _BitInt(1) type_of_addition_data_{};
    unsigned _BitInt(8) additional_data_{};

    ShortLocationReport() = default;
    explicit ShortLocationReport(BitVector& data);

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ShortLocationReport, time_elapsed_, longitude_, latitude_, position_error_,
                                   horizontal_velocity_, direction_of_travel_, type_of_addition_data_, additional_data_)
};

auto operator<<(std::ostream& stream, const ShortLocationReport& slr) -> std::ostream&;

struct LocationInformationProtocol {
    unsigned _BitInt(2) pdu_type_{};
    std::optional<ShortLocationReport> short_location_report_;

    LocationInformationProtocol() = default;

    explicit LocationInformationProtocol(BitVector& data);

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(LocationInformationProtocol, pdu_type_, short_location_report_)
};

auto operator<<(std::ostream& stream, const LocationInformationProtocol& lip) -> std::ostream&;

struct SimpleTextMessaging {
    unsigned _BitInt(7) text_coding_scheme_{};
    BitVector message_;

    SimpleTextMessaging() = default;

    explicit SimpleTextMessaging(BitVector& data);

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SimpleTextMessaging, text_coding_scheme_, message_)
};

auto operator<<(std::ostream& stream, const SimpleTextMessaging& msg) -> std::ostream&;

enum class DeliveryReportRequest {
    kNoDeliveryReportRequested,
    kMessageReceivedReportRequested,
    kMessageConsumedReportRequested,
    kMessageReceivedAndConsumedReportRequested,
};

constexpr auto to_string(DeliveryReportRequest type) -> const char* {
    switch (type) {
    case DeliveryReportRequest::kNoDeliveryReportRequested:
        return "No delivery report requested";
    case DeliveryReportRequest::kMessageReceivedReportRequested:
        return "Message received report requested";
    case DeliveryReportRequest::kMessageConsumedReportRequested:
        return "Message consumed report requested";
    case DeliveryReportRequest::kMessageReceivedAndConsumedReportRequested:
        return "Message received and consumed report requested";
    }
};

struct ShortDataServiceTLTransferPacketInformation {
    DeliveryReportRequest delivery_report_request_{};
    unsigned _BitInt(1) service_selection_and_short_from_report_{};
    unsigned _BitInt(1) storage_and_forward_control_{};
    unsigned _BitInt(8) message_reference_{};

    ShortDataServiceTLTransferPacketInformation() = default;

    explicit ShortDataServiceTLTransferPacketInformation(BitVector& data);

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ShortDataServiceTLTransferPacketInformation, delivery_report_request_,
                                   service_selection_and_short_from_report_, storage_and_forward_control_,
                                   message_reference_)
};

auto operator<<(std::ostream& stream, const ShortDataServiceTLTransferPacketInformation& packet_info) -> std::ostream&;

enum class ShortDataServiceTLMessageType {
    kSdsTransfer,
    kSdsReport,
    kSdsAck,
    kSds3,
    kSds4,
    kSds5,
    kSds6,
    kSds7,
    kSds8,
    kSds9,
    kSds10,
    kSds11,
    kSds12,
    kSds13,
    kSds14,
    kSds15,
};

constexpr auto to_string(ShortDataServiceTLMessageType type) -> const char* {
    switch (type) {
    case ShortDataServiceTLMessageType::kSdsTransfer:
        return "SDS-TRANSFER";
    case ShortDataServiceTLMessageType::kSdsReport:
        return "SDS-REPORT";
    case ShortDataServiceTLMessageType::kSdsAck:
        return "SDS-ACK";
    case ShortDataServiceTLMessageType::kSds3:
        return "SDS 3";
    case ShortDataServiceTLMessageType::kSds4:
        return "SDS 4";
    case ShortDataServiceTLMessageType::kSds5:
        return "SDS 5";
    case ShortDataServiceTLMessageType::kSds6:
        return "SDS 6";
    case ShortDataServiceTLMessageType::kSds7:
        return "SDS 7";
    case ShortDataServiceTLMessageType::kSds8:
        return "SDS 8";
    case ShortDataServiceTLMessageType::kSds9:
        return "SDS 9";
    case ShortDataServiceTLMessageType::kSds10:
        return "SDS 10";
    case ShortDataServiceTLMessageType::kSds11:
        return "SDS 11";
    case ShortDataServiceTLMessageType::kSds12:
        return "SDS 12";
    case ShortDataServiceTLMessageType::kSds13:
        return "SDS 13";
    case ShortDataServiceTLMessageType::kSds14:
        return "SDS 14";
    case ShortDataServiceTLMessageType::kSds15:
        return "SDS 15";
    }
};

struct ShortDataServiceTLPacketInformation {
    ShortDataServiceTLMessageType message_type_{};
    std::optional<ShortDataServiceTLTransferPacketInformation> transfer_packet_;

    ShortDataServiceTLPacketInformation() = default;

    explicit ShortDataServiceTLPacketInformation(BitVector& data);

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ShortDataServiceTLPacketInformation, message_type_, transfer_packet_)
};

auto operator<<(std::ostream& stream, const ShortDataServiceTLPacketInformation& packet_info) -> std::ostream&;

struct TextMessaging {
    unsigned _BitInt(1) time_stamp_used_{};
    unsigned _BitInt(7) text_coding_scheme_{};
    std::optional<unsigned _BitInt(24)> time_stamp_;
    BitVector message_;

    TextMessaging() = default;

    explicit TextMessaging(BitVector& data);

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TextMessaging, time_stamp_used_, text_coding_scheme_, time_stamp_, message_)
};

auto operator<<(std::ostream& stream, const TextMessaging& msg) -> std::ostream&;

struct ShortDataServicePacket : public CircuitModeControlEntityPacket {
    unsigned _BitInt(8) protocol_identifier_{};
    std::optional<SimpleTextMessaging> simple_text_messaging_;
    std::optional<LocationInformationProtocol> location_information_protocol_;
    std::optional<TextMessaging> text_messaging_;
    std::optional<ShortDataServiceTLPacketInformation> sds_tl_;

    static constexpr const std::size_t kSimpleTextMessaging = 0b00000010;
    static constexpr const std::size_t kLocationInformationProtocolIdentifier = 0b00001010;
    static constexpr const std::size_t kTextMessaging = 0b10000010;

    ShortDataServicePacket() = default;

    explicit ShortDataServicePacket(const CircuitModeControlEntityPacket& packet);

    [[nodiscard]] auto text_message() const -> std::optional<BitVector> {
        if (text_messaging_) {
            return text_messaging_->message_;
        }
        if (simple_text_messaging_) {
            return simple_text_messaging_->message_;
        }
        return {};
    };

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ShortDataServicePacket, burst_type_, logical_channel_, type_, encrypted_, address_,
                                   fragmentation_, fragmentation_on_stealling_channel_, reservation_requirement_,
                                   tm_sdu_, encryption_mode_, immediate_napping_permission_flag_,
                                   basic_slot_granting_element_, position_of_grant_, channel_allocation_element_,
                                   random_access_flag_, power_control_element_, basic_link_information_, tl_sdu_,
                                   mle_protocol_, sdu_, packet_type_, sds_data_, protocol_identifier_,
                                   simple_text_messaging_, location_information_protocol_, text_messaging_, sds_tl_)
};

auto operator<<(std::ostream& stream, const ShortDataServicePacket& sds) -> std::ostream&;