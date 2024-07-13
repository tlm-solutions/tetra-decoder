/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "borzoi/borzoi_converter.hpp"
#include "utils/bit_vector.hpp"
#include <nlohmann/json_fwd.hpp>

inline static auto get_time() -> std::string {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::stringstream ss;
    ss << std::put_time(&tm, "%FT%T%z");
    return ss.str();
}

auto BorzoiConverter::to_json(ShortDataServicePacket* packet) -> nlohmann::json {
    auto message = nlohmann::json::object();
    /// TODO: this may throw
    message["source_ssi"] = static_cast<unsigned>(packet->sds_data_->address_.ssi().value());
    message["destination_ssi"] = static_cast<unsigned>(packet->address_.ssi().value());
    message["protocol_identifier"] = static_cast<unsigned>(packet->protocol_identifier_);
    message["telegram_type"] = "SDS";
    message["data"] = nlohmann::json::array();
    auto data = BitVector(packet->sds_data_->data_);
    while (data.bits_left() >= 8) {
        unsigned bits = data.take<8>();
        message["data"].push_back(bits);
    }
    message["arbitrary"] = nlohmann::json::object();
    if (data.bits_left() > 0) {
        message["arbitrary"]["bits_in_last_byte"] = data.bits_left();
        message["data"].push_back(data.take_all());
    } else {
        message["arbitrary"]["bits_in_last_byte"] = 8;
    }
    message["arbitrary"]["optional_fields"] = nlohmann::json::object();
    for (const auto& [key, value] : packet->sds_data_->optional_elements_) {
        auto& vec = message["arbitrary"]["optional_fields"][to_string(key)];
        vec = nlohmann::json::object();
        vec["repeated_elements"] = value.repeated_elements;
        vec["unparsed_bits"] = nlohmann::json::array();
        auto data = BitVector(value.unparsed_bits);
        while (data.bits_left() >= 8) {
            unsigned bits = data.take<8>();
            vec["unparsed_bits"].push_back(bits);
        }
        if (data.bits_left() > 0) {
            vec["bits_in_last_byte"] = data.bits_left();
            vec["unparsed_bits"].push_back(data.take_all());
        } else {
            vec["bits_in_last_byte"] = 8;
        }
    }
    message["time"] = get_time();

    return message;
}

auto BorzoiConverter::to_json(const Slots& slots) -> nlohmann::json {
    auto message = nlohmann::json::object();

    message["time"] = get_time();
    message["burst_type"] = static_cast<unsigned>(slots.get_burst_type());
    /// This may but should not throw.
    auto first_slot = slots.get_first_slot().get_logical_channel_data_and_crc();
    message["first_slot_logical_channel"] = static_cast<unsigned>(first_slot.channel);
    message["first_slot_data"] = nlohmann::json::array();
    while (first_slot.data.bits_left()) {
        unsigned bit = first_slot.data.take<1>();
        message["first_slot_logical_data"].push_back(bit);
    }
    message["first_slot_crc_ok"] = first_slot.crc_ok;
    auto second_slot_present = slots.has_second_slot();
    message["second_slot_present"] = second_slot_present;
    if (second_slot_present) {
        auto second_slot = slots.get_second_slot().get_logical_channel_data_and_crc();
        message["second_slot_logical_channel"] = static_cast<unsigned>(first_slot.channel);
        message["second_slot_data"] = nlohmann::json::array();
        while (first_slot.data.bits_left()) {
            unsigned bit = first_slot.data.take<1>();
            message["second_slot_data"].push_back(bit);
        }
        message["second_slot_crc_ok"] = first_slot.crc_ok;
    }
    return message;
}