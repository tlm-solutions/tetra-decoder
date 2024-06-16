/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#pragma once

#include "l3/circuit_mode_control_entity.hpp"
#include "l3/mobile_management.hpp"
#include "reporter.hpp"
#include "utils/bit_vector.hpp"

class MobileLinkEntity {
  public:
    MobileLinkEntity() = delete;
    MobileLinkEntity(Reporter&& reporter, bool is_downlink)
        : cmce_(CircuitModeControlEntity(std::move(reporter)))
        , is_downlink_(is_downlink){};
    ~MobileLinkEntity() noexcept = default;

    void service_user_pdu(Address address, BitVector& vec);

  private:
    void service_data_pdu(Address address, BitVector& vec);

    CircuitModeControlEntity cmce_;
    MobileManagement mm_;

    // Wheather this MLE is for decoding downlink or uplink data
    const bool is_downlink_;
};