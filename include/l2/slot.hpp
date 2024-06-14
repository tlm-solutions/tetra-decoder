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

    friend auto operator<<(std::ostream& stream, const Slot& slot) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const Slot& slot) -> std::ostream&;

/// defines the number and types of slots in a packet
enum class SlotsType { kOneSubslot, kTwoSubslots, kFullSlot };

constexpr auto to_string(SlotsType type) noexcept -> const char* {
    switch (type) {
    case SlotsType::kOneSubslot:
        return "OneSubslot";
    case SlotsType::kTwoSubslots:
        return "TwoSubslots";
    case SlotsType::kFullSlot:
        return "FullSlot";
    }
};

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

    Slots(const Slots&) = default;

    /// constructor for one subslot or a full slot
    Slots(BurstType burst_type, SlotsType slot_type, Slot&& slot);

    /// construct for two half slot
    Slots(BurstType burst_type, SlotsType slot_type, Slot&& first_slot, Slot&& second_slot);

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
    [[nodiscard]] auto has_crc_error() -> bool;

    /// get the type of the underlying burst
    [[nodiscard]] auto get_burst_type() const noexcept -> BurstType { return burst_type_; }

    friend auto operator<<(std::ostream& stream, const Slots& slots) -> std::ostream&;
};

auto operator<<(std::ostream& stream, const Slots& slots) -> std::ostream&;