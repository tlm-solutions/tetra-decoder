/*
 * Copyright (C) 2024 Transit Live Mapping Solutionss
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "l2/access_assignment_channel.hpp"
#include "utils/bit_vector.hpp"
#include <cassert>

AccessAssignmentChannel::AccessAssignmentChannel(const BurstType burst_type, const TimebaseCounter& time,
                                                 const std::vector<uint8_t>& data) {
    assert(data.size() == 14);
    assert(is_downlink_burst(burst_type));

    auto vec = BitVector(data);

    auto header = vec.take(2);
    auto field1 = vec.take(6);
    auto _field2 = vec.take(6);

    // TODO: parse uplink marker and some other things relevant for the uplink
    if (time.frame_number() == 18) {
        downlink_usage = DownlinkUsage::CommonControl;
    } else {
        if (header == 0b00) {
            downlink_usage = DownlinkUsage::CommonControl;
        } else {
            switch (field1) {
            case 0b00000:
                downlink_usage = DownlinkUsage::Unallocated;
                break;
            case 0b00001:
                downlink_usage = DownlinkUsage::AssignedControl;
                break;
            case 0b00010:
                downlink_usage = DownlinkUsage::CommonControl;
                break;
            case 0b00011:
                downlink_usage = DownlinkUsage::CommonAndAssignedControl;
                break;
            default:
                downlink_usage = DownlinkUsage::Traffic;
                downlink_traffic_usage_marker = field1;
                break;
            }
        }
    }
}

auto operator<<(std::ostream& stream, const AccessAssignmentChannel& aac) -> std::ostream& {
    stream << "[Channel] AACH downlink_usage: " << aac.downlink_usage
           << " downlinkUsageTrafficMarker: " << aac.downlink_traffic_usage_marker << std::endl;
}