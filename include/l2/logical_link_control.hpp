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
#include <reporter.hpp>
#include <utils/address_type.hpp>
#include <utils/bit_vector.hpp>

class LogicalLinkControl {
  public:
    LogicalLinkControl(std::shared_ptr<Reporter> reporter, std::shared_ptr<MobileLinkEntity> mle)
        : reporter_(reporter)
        , mle_(mle){};
    ~LogicalLinkControl() noexcept = default;

    void process(const AddressType address, BitVector& vec);

    friend auto operator<<(std::ostream& stream, const LogicalLinkControl& llc) -> std::ostream&;

  private:
    // Basic link (acknowledged service in connectionless mode) without Frame Check Sequence
    void process_bl_adata_without_fcs(const AddressType address, BitVector& vec);
    // Basic link (acknowledged service in connectionless mode) without Frame Check Sequence
    void process_bl_data_without_fcs(const AddressType address, BitVector& vec);

    std::shared_ptr<Reporter> reporter_{};
    std::shared_ptr<MobileLinkEntity> mle_{};
};

std::ostream& operator<<(std::ostream& stream, const LogicalLinkControl& llc);

#endif
