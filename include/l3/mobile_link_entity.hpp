/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#pragma once

#include <cstdint>
#include <iostream>
#include <memory>

#include <l3/circuit_mode_control_entity.hpp>
#include <l3/mobile_management.hpp>
#include <reporter.hpp>
#include <utils/bit_vector.hpp>

class MobileLinkEntity {
  public:
    MobileLinkEntity() = delete;
    MobileLinkEntity(std::shared_ptr<Reporter> reporter, bool is_downlink)
        : reporter_(std::move(reporter))
        , cmce_(std::make_unique<CircuitModeControlEntity>(reporter_))
        , mm_(std::make_unique<MobileManagement>())
        , is_downlink_(is_downlink){};
    ~MobileLinkEntity() noexcept = default;

    void service_user_pdu(Address address, BitVector& vec);

  private:
    void service_data_pdu(Address address, BitVector& vec);

    std::shared_ptr<Reporter> reporter_{};
    std::unique_ptr<CircuitModeControlEntity> cmce_{};
    std::unique_ptr<MobileManagement> mm_{};

    // Wheather this MLE is for decoding downlink or uplink data
    const bool is_downlink_;
};