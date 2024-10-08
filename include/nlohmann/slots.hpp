/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l2/slot.hpp"
#include <nlohmann/json.hpp>

static auto stob(const std::string& str) -> bool {
    bool retval = false;

    // convert integer (0 and 1) to bool
    std::istringstream is(str);
    is >> retval;

    // convert boolean string (false and true) to bool
    if (is.fail()) {
        is.clear();
        is >> std::boolalpha >> retval;
    }

    if (is.fail()) {
        throw std::invalid_argument("Cannot convert: " + str + " to bool.");
    }

    return retval;
};

namespace nlohmann {
template <> struct adl_serializer<Slots> {
    static void to_json(json& j, const Slots& slots) {
        j["burst_type"] = slots.get_burst_type();
        j["slot_type"] = slots.get_slot_type();
        /// This may but should not throw.
        auto first_slot = slots.get_first_slot().get_logical_channel_data_and_crc();
        j["first_slot_logical_channel"] = first_slot.channel;
        j["first_slot_data"] = first_slot.data;
        j["first_slot_crc_ok"] = first_slot.crc_ok;
        auto second_slot_present = slots.has_second_slot();
        j["second_slot_present"] = second_slot_present;
        if (second_slot_present) {
            auto second_slot = slots.get_second_slot().get_logical_channel_data_and_crc();
            j["second_slot_logical_channel"] = first_slot.channel;
            j["second_slot_data"] = first_slot.data;
            j["second_slot_crc_ok"] = first_slot.crc_ok;
        }
    }

    static void from_json(const json& j, Slots& slots) {
        auto burst_type = BurstType(std::stoi(j["burst_type"].template get<std::string>()));
        auto slot_type = SlotType(std::stoi(j["slot_type"].template get<std::string>()));
        auto first_slot_logical_channel =
            LogicalChannel(std::stoi(j["first_slot_logical_channel"].template get<std::string>()));
        auto first_slot_data = j["first_slot_data"].template get<BitVector>();
        auto first_slot_crc_ok = stob(j["first_slot_crc_ok"].template get<std::string>());
        auto second_slot_present = stob(j["second_slot_present"].template get<std::string>());

        auto first_slot = Slot(LogicalChannelDataAndCrc{
            .channel = first_slot_logical_channel,
            .data = first_slot_data,
            .crc_ok = first_slot_crc_ok,
        });

        if (second_slot_present) {
            auto second_slot_logical_channel =
                LogicalChannel(std::stoi(j["second_slot_logical_channel"].template get<std::string>()));
            auto second_slot_data = j["second_slot_data"].template get<BitVector>();
            auto second_slot_crc_ok = stob(j["second_slot_crc_ok"].template get<std::string>());

            auto second_slot = Slot(LogicalChannelDataAndCrc{
                .channel = second_slot_logical_channel,
                .data = second_slot_data,
                .crc_ok = second_slot_crc_ok,
            });

            slots = Slots(burst_type, slot_type, std::move(first_slot), std::move(second_slot));
        } else {
            slots = Slots(burst_type, slot_type, std::move(first_slot));
        }
    }
};
} // namespace nlohmann
