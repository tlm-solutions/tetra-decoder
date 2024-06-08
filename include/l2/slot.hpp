/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "burst_type.hpp"
#include "l2/logical_channel.hpp"
#include <cassert>
#include <set>
#include <vector>

/// describe a slot (full or half) and its content with data and logical channels. it can be non concreate i.e.,
/// multiple logical channels are present and the correct one still needs to be selected
class Slot {
  private:
    std::vector<LogicalChannelDataAndCrc> data_;

  public:
    Slot() = delete;

    /// construct a slot with a defined channel and data
    explicit Slot(LogicalChannelDataAndCrc&& data)
        : data_({std::move(data)}){};

    /// construct a slot with any of two input data and different channel
    explicit Slot(std::vector<LogicalChannelDataAndCrc> data)
        : data_(std::move(data)) {
        std::set<LogicalChannel> channels_in_data;
        for (const auto& channel_data : data_) {
            const auto& channel = channel_data.channel;
            channels_in_data.insert(channel);
        }

        if (data_.size() != channels_in_data.size()) {
            throw std::runtime_error("Found duplicate entries of channels in initilization of Slot");
        }
    };

    /// if there is only one possibility for a logical channel, the the slot is concreate
    [[nodiscard]] auto is_concreate() const noexcept -> bool { return data_.size() == 1; };

    /// get the concreate logical channel, data and crc
    [[nodiscard]] auto get_logical_channel_data_and_crc() -> LogicalChannelDataAndCrc& {
        if (!is_concreate()) {
            throw std::runtime_error("Attempted to get a concreate channel that is not concreate.");
        }
        return data_.front();
    }

    /// get the set of potential logical channels
    [[nodiscard]] auto get_logical_channels() const noexcept -> std::set<LogicalChannel> {
        std::set<LogicalChannel> channels;
        for (const auto& channel_data : data_) {
            const auto& channel = channel_data.channel;
            channels.insert(channel);
        };
        return channels;
    }

    /// select a specific logical channel and make the slot concreate
    auto select_logical_channel(LogicalChannel channel) -> void {
        for (auto it = data_.begin(); it != data_.end();) {
            if (it->channel != channel) {
                it = data_.erase(it);
            } else {
                ++it;
            }
        }
        if (!is_concreate()) {
            throw std::runtime_error("Attempted to select a channel that is not availabe.");
        }
    }
};

/// defines the number and types of slots in a packet
enum class SlotsType { kOneSubslot, kTwoSubslots, kFullSlot };

/// defines the slots in a packet
class Slots {
  private:
    /// which burst type ths slots originated from
    BurstType burst_type_;
    /// the number and types of slots
    SlotsType slot_type_;
    /// the slots, either one half or full slot or two half slots
    std::vector<Slot> slots_;

  public:
    Slots() = delete;

    /// constructor for one subslot or a full slot
    Slots(BurstType burst_type, SlotsType slot_type, Slot&& slot)
        : burst_type_(burst_type)
        , slot_type_(slot_type)
        , slots_({std::move(slot)}) {
        if (slot_type_ == SlotsType::kTwoSubslots) {
            throw std::runtime_error("Two subslots need to to have two subslots");
        }

        /// If we processes the normal uplink burst assume that the slot is signalling and not traffic. We would need
        /// the information from the access assignment from the corresponding downlink timeslot to make the right
        /// decision here.
        if (burst_type_ == BurstType::NormalUplinkBurst) {
            get_first_slot().select_logical_channel(LogicalChannel::kSignalingChannelFull);
        }

        if (!get_first_slot().is_concreate()) {
            throw std::runtime_error("The first or only slot is not concreate.");
        }
    };

    /// construct for two half slot
    Slots(BurstType burst_type, SlotsType slot_type, Slot&& first_slot, Slot&& second_slot)
        : burst_type_(burst_type)
        , slot_type_(slot_type)
        , slots_({std::move(first_slot), std::move(second_slot)}) {
        if (slot_type_ != SlotsType::kTwoSubslots) {
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
                        auto length_indication =
                            first_slot_data.look<6>(length_indication_or_capacity_request_offset + 1);
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

    /// access the first slot
    [[nodiscard]] auto get_first_slot() noexcept -> Slot& { return slots_.front(); };

    /// access the second slot
    [[nodiscard]] auto get_second_slot() -> Slot& {
        if (!has_second_slot()) {
            throw std::runtime_error("Attempted to accesses the second slot, but we do not have two slots.");
        }
        return slots_.at(1);
    };

    /// check if we have two subslots
    [[nodiscard]] auto has_second_slot() const noexcept -> bool { return slots_.size() == 2; }

    // check if there was any crc mismatch on the signalling or stealing channels
    [[nodiscard]] auto has_crc_error() -> bool {
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
};