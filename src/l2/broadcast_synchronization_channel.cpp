/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l2/broadcast_synchronization_channel.hpp"
#include "l2/timebase_counter.hpp"
#include "utils/bit_vector.hpp"
#include <bitset>
#include <cassert>

BroadcastSynchronizationChannel::BroadcastSynchronizationChannel(const BurstType burst_type,
                                                                 const std::vector<uint8_t>& data) {
    assert(data.size() == 60);
    assert(is_downlink_burst(burst_type));

    auto vec = BitVector(data);

    assert(vec.bits_left() == 60);

    system_code = vec.take(4);
    color_code = vec.take(6);
    auto time_slot = vec.take(2) + 1;
    auto frame_number = vec.take(5);
    auto multi_frame_number = vec.take(6);
    time = TimebaseCounter(time_slot, frame_number, multi_frame_number);
    sharing_mode = vec.take(2);
    time_slot_reserved_frames = vec.take(3);
    up_lane_dtx = vec.take(1);
    frame_18_extension = vec.take(1);
    auto _reserved = vec.take(1);

    assert(vec.bits_left() == 29);

    mobile_country_code = vec.take(10);
    mobile_network_code = vec.take(14);
    dNwrk_broadcast_broadcast_supported = vec.take(1);
    dNwrk_broadcast_enquiry_supported = vec.take(1);
    cell_load_ca = vec.take(2);
    late_entry_supported = vec.take(1);

    // 10 MSB of MCC
    uint16_t lmcc = mobile_country_code & 0x03ff;
    // 14 MSB of MNC
    uint16_t lmnc = mobile_network_code & 0x3fff;
    // 6 MSB of ColorCode
    uint16_t lcolor_code = color_code & 0x003f;

    // 30 MSB bits
    scrambling_code = lcolor_code | (lmnc << 6) | (lmcc << 20);
    // scrambling initialized to 1 on bits 31-32 - 8.2.5.2 (54)
    scrambling_code = (scrambling_code << 2) | 0x0003;
}

auto operator<<(std::ostream& stream, const BroadcastSynchronizationChannel& bsc) -> std::ostream& {
    stream << "[Channel] BSCH SYNC:" << std::endl;
    stream << "  System code: 0b" << std::bitset<4>(bsc.system_code) << std::endl;
    stream << "  Color code: " << std::to_string(bsc.color_code) << std::endl;
    stream << "  " << bsc.time << std::endl;
    stream << "  Scrambling code: " << std::to_string(bsc.scrambling_code) << std::endl;
    std::string sharing_mode_map[] = {"Continuous transmission", "Carrier sharing", "MCCH sharing",
                                      "Traffic carrier sharing"};
    stream << "  Sharing mode: " << sharing_mode_map[bsc.sharing_mode] << std::endl;
    uint8_t ts_reserved_frames_map[] = {1, 2, 3, 4, 6, 9, 12, 18};
    stream << "  TS reserved frames: " << std::to_string(ts_reserved_frames_map[bsc.time_slot_reserved_frames])
           << " frames reserved per 2 multiframes" << std::endl;
    stream << "  "
           << (bsc.up_lane_dtx ? "Discontinuous U-plane transmission is allowed"
                               : "Discontinuous U-plane transmission is not allowed")
           << std::endl;
    stream << "  " << (bsc.frame_18_extension ? "Frame 18 extension allowed" : "No frame 18 extension") << std::endl;

    stream << "[Channel] BSCH D-MLE-SYNC:" << std::endl;
    stream << "  MCC: " << bsc.mobile_country_code << std::endl;
    stream << "  MNC: " << bsc.mobile_network_code << std::endl;
    stream << "  Neighbour cell broadcast: "
           << (bsc.dNwrk_broadcast_broadcast_supported ? "supported" : "not supported") << std::endl;
    stream << "  Neighbour cell enquiry: " << (bsc.dNwrk_broadcast_enquiry_supported ? "supported" : "not supported")
           << std::endl;
    stream << "  Cell load CA: ";
    switch (bsc.cell_load_ca) {
    case 0b00:
        stream << "Cell load unknown";
        break;
    case 0b01:
        stream << "Low cell load";
        break;
    case 0b10:
        stream << "Medium cell load";
        break;
    case 0b11:
        stream << "High cell load";
        break;
    default:
        break;
    }
    stream << std::endl;
    stream << "  Late entry supported: "
           << (bsc.late_entry_supported ? "Late entry available" : "Late entry not supported") << std::endl;

    return stream;
}