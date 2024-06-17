/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#include "l3/mobile_link_entity.hpp"
#include <cassert>

void MobileLinkEntity::service_user_pdu(const Address address, BitVector& vec) {
    if (vec.bits_left() == 0) {
        return;
    }

    auto pdu_type = vec.take<3>();
    const auto& pdu_name = protocol_discriminator_description_.at(pdu_type);

    std::cout << "MLE " << pdu_name << " " << vec << std::endl;

    if (metrics_ && pdu_type != kMleProtocol) {
        metrics_->increment(pdu_name);
    }

    switch (pdu_type) {
    case 0b001:
        mm_.process(address, vec);
        break;
    case 0b010:
        cmce_.process(address, vec);
        break;
    case kMleProtocol:
        service_data_pdu(address, vec);
        break;
    default:
        break;
    }
}

void MobileLinkEntity::service_data_pdu(const Address address, BitVector& vec) {
    if (vec.bits_left() == 0) {
        return;
    }

    auto pdu_type = vec.take<3>();
    const auto& pdu_name = mle_pdu_description_.at(pdu_type);

    if (metrics_ && pdu_type != kExtendedPdu) {
        metrics_->increment(pdu_name);
    }

    std::cout << "  " << pdu_name << std::endl;
    std::cout << "  " << vec << std::endl;

    if (pdu_type == kExtendedPdu) {
        auto pdu_type = vec.take<4>();
        const auto& pdu_name = mle_pdu_extension_description_.at(pdu_type);

        if (metrics_) {
            metrics_->increment(pdu_name);
        }

        std::cout << "  " << pdu_name << std::endl;
        std::cout << "  " << vec << std::endl;
    }
}