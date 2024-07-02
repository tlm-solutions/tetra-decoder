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
    message["source_ssi"] = packet->sds_data_->address_.ssi()->to_ulong();
    message["destination_ssi"] = packet->address_.ssi()->to_ulong();
    message["protocol_identifier"] = static_cast<unsigned>(packet->protocol_identifier_);
    message["telegram_type"] = "SDS";
    message["data"] = nlohmann::json::array();
    auto data = BitVector(packet->sds_data_->data_);
    while (data.bits_left() >= 8) {
        unsigned bits = data.take<8>();
        message["data"].push_back(bits);
    }
    message["arbitrary"] = nlohmann::json::object();
    message["arbitrary"]["bits_in_last_byte"] = data.bits_left();
    message["data"].push_back(data.take_all());
    message["time"] = get_time();

    return message;
}