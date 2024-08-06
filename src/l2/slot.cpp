/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "l2/slot.hpp"

auto operator<<(std::ostream& stream, const Slot& slot) -> std::ostream& {
    for (const auto& data : slot.data_) {
        stream << "    Channel: " << to_string(data.channel) << std::endl;
        stream << "    Data: " << data.data << std::endl;
        stream << "    Crc ok: " << (data.crc_ok ? "true" : "false") << std::endl;
    }
    return stream;
}

Slots::Slots(BurstType burst_type, SlotType slot_type, Slot&& slot)
    : burst_type_(burst_type)
    , slot_type_(slot_type)
    , slots_({std::move(slot)}) {
    if (slot_type_ == SlotType::kTwoSubslots) {
        throw std::runtime_error("Two subslots need to to have two subslots");
    }

    /// If we processes the normal uplink burst assume that the slot is signalling and not traffic. We would need
    /// the information from the access assignment from the corresponding downlink timeslot to make the right
    /// decision here.
    if (burst_type_ == BurstType::NormalUplinkBurst) {
        get_first_slot().select_logical_channel(LogicalChannel::kSignallingChannelFull);
    }

    if (!get_first_slot().is_concreate()) {
        throw std::runtime_error("The first or only slot is not concreate.");
    }
};

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
Slots::Slots(BurstType burst_type, SlotType slot_type, Slot&& first_slot, Slot&& second_slot)
    : burst_type_(burst_type)
    , slot_type_(slot_type)
    , slots_({std::move(first_slot), std::move(second_slot)}) {
    if (slot_type_ != SlotType::kTwoSubslots) {
        throw std::runtime_error("Only two subslots is allowed to have two subslots");
    }

    if (!get_first_slot().is_concreate()) {
        throw std::runtime_error("The first subslot is not concreate.");
    }

    const auto& first_slot_data = get_first_slot().get_logical_channel_data_and_crc().data;
    if (get_first_slot().get_logical_channel_data_and_crc().channel == LogicalChannel::kStealingChannel) {
        // The first subslot is stolen, now we need to decide if the second is also stolen or traffic
        auto second_half_slot_stolen = false;

        if (burst_type_ == BurstType::NormalUplinkBurstSplit) {
            auto pdu_type = first_slot_data.look<2>(0);
            // read the stolen flag from the MAC-DATA
            // 21.4.2.3 MAC-DATA
            if (pdu_type == 0b00) {
                auto address_type = first_slot_data.look<2>(4);
                auto length_indication_or_capacity_request_offset = 6 + (address_type == 0b01 ? 10 : 24);
                auto length_indication_or_capacity_request =
                    first_slot_data.look<1>(length_indication_or_capacity_request_offset);
                if (length_indication_or_capacity_request == 0b0) {
                    auto length_indication = first_slot_data.look<6>(length_indication_or_capacity_request_offset + 1);
                    if (length_indication == 0b111110 || length_indication == 0b111111) {
                        second_half_slot_stolen = true;
                    }
                }
            }

            // read the stolen flag from the MAC-U-SIGNAL PDU
            // 21.4.5 TMD-SAP: MAC PDU structure for U-plane signalling
            if (pdu_type == 0b11) {
                if (first_slot_data.look<1>(2)) {
                    second_half_slot_stolen = true;
                };
            }
        }

        if (burst_type_ == BurstType::NormalDownlinkBurstSplit) {
            auto pdu_type = first_slot_data.look<2>(0);
            // read the sloten flag from the MAC-RESOURCE PDU
            // 21.4.3.1 MAC-RESOURCE
            if (pdu_type == 0b00) {
                auto length_indication = first_slot_data.look<6>(7);
                if (length_indication == 0b111110 || length_indication == 0b111111) {
                    second_half_slot_stolen = true;
                }
            }

            // read the stolen flag from the MAC-U-SIGNAL PDU
            // 21.4.5 TMD-SAP: MAC PDU structure for U-plane signalling
            if (pdu_type == 0b11) {
                if (first_slot_data.look<1>(2)) {
                    second_half_slot_stolen = true;
                };
            }
        }

        get_second_slot().select_logical_channel(second_half_slot_stolen ? LogicalChannel::kStealingChannel
                                                                         : LogicalChannel::kTrafficChannel);
    }

    if (!get_second_slot().is_concreate()) {
        throw std::runtime_error("The second slot is not concreate.");
    }
};

auto Slots::has_crc_error() -> bool {
    auto error = false;

    const auto& first_slot = slots_.front().get_logical_channel_data_and_crc();
    if (first_slot.channel != LogicalChannel::kTrafficChannel) {
        error |= !first_slot.crc_ok;
    }

    if (has_second_slot()) {
        const auto& second_slot = slots_.at(1).get_logical_channel_data_and_crc();
        if (second_slot.channel != LogicalChannel::kTrafficChannel) {
            error |= !first_slot.crc_ok;
        }
    }

    return error;
};

auto operator<<(std::ostream& stream, const Slots& slots) -> std::ostream& {
    stream << "Slots:" << std::endl;
    stream << "  [BurstType] " << to_string(slots.burst_type_) << std::endl;
    stream << "  [SlotType] " << to_string(slots.slot_type_) << std::endl;
    for (const auto& slot : slots.slots_) {
        stream << "  Slot:" << std::endl;
        stream << slot;
    }
    return stream;
}