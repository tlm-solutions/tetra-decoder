/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "l3/short_data_service.hpp"
#include "l3/short_data_service_packet.hpp"
#include <cassert>
#include <cmath>

auto ShortDataService::process(const CircuitModeControlEntityPacket& packet)
    -> std::unique_ptr<ShortDataServicePacket> {
    auto protocol_identifier = packet.sds_data_->data_.look<8>(0);
    const auto& protocol_identifier_name = sds_pdu_description_.at(protocol_identifier);

    if (metrics_) {
        metrics_->increment(protocol_identifier_name);
    }

    return std::make_unique<ShortDataServicePacket>(packet);
}