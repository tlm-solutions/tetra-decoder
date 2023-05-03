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

#include <l3/mobile_link_entity.hpp>
#include <utils/address_type.hpp>
#include <utils/bit_vector.hpp>

class LogicalLinkControl {
  public:
    LogicalLinkControl(std::shared_ptr<MobileLinkEntity> mle)
        : mle_(mle){};
    ~LogicalLinkControl() noexcept = default;

    void process(const AddressType address, BitVector& vec);

    friend std::ostream& operator<<(std::ostream& stream, const LogicalLinkControl& llc);

  private:
    void process_bl_data_without_fcs(const AddressType address, BitVector& vec);

    std::shared_ptr<MobileLinkEntity> mle_;
};

std::ostream& operator<<(std::ostream& stream, const LogicalLinkControl& llc);

#endif
