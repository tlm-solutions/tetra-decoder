/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "l3/mobile_management.hpp"
#include <cassert>
#include <iostream>

void MobileManagement::process(const Address /*address*/, BitVector& vec) {

    auto pdu_type = vec.take<4>();
    const auto& pdu_name = mm_pdu_description_.at(pdu_type);

    if (metrics_) {
        metrics_->increment(pdu_name);
    }

    std::cout << "MM " << pdu_name << std::endl;
    switch (pdu_type) {
    default:
        break;
    }
}
