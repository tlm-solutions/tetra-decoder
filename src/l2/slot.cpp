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

auto operator<<(std::ostream& stream, const Slots& slots) -> std::ostream& {
    stream << "Slots:" << std::endl;
    stream << "  [BurstType] " << to_string(slots.burst_type_) << std::endl;
    stream << "  [SlotsType] " << to_string(slots.slot_type_) << std::endl;
    for (const auto& slot : slots.slots_) {
        stream << "  Slot:" << std::endl;
        stream << slot;
    }
    return stream;
}