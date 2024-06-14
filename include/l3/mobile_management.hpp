/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include <memory>

#include <reporter.hpp>
#include <utils/address.hpp>
#include <utils/bit_vector.hpp>

class MobileManagement {
  public:
    MobileManagement(){};
    ~MobileManagement() noexcept = default;

    void process(bool is_downlink, Address address, BitVector& vec);

  private:
};
