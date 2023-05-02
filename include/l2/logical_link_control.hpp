/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#ifndef L2_LLC_HPP
#define L2_LLC_HPP

#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

#include <l2/address_type.hpp>
#include <utils/bit_vector.hpp>

class LogicalLinkControl {
  public:
    LogicalLinkControl() noexcept = default;
    ~LogicalLinkControl() noexcept = default;

    void process(AddressType address_type, BitVector& vec);

    friend std::ostream& operator<<(std::ostream& stream, const LogicalLinkControl& llc);
};

std::ostream& operator<<(std::ostream& stream, const LogicalLinkControl& llc);

#endif
