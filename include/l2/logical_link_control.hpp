/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l3/mobile_link_entity.hpp"
#include "utils/address.hpp"
#include "utils/bit_vector.hpp"
#include <cstdint>
#include <iostream>
#include <vector>

class LogicalLinkControl {
  public:
    LogicalLinkControl() = delete;
    explicit LogicalLinkControl(MobileLinkEntity&& mle)
        : mle_(mle){};
    ~LogicalLinkControl() noexcept = default;

    void process(Address address, BitVector& vec);

    friend auto operator<<(std::ostream& stream, const LogicalLinkControl& llc) -> std::ostream&;

  private:
    // Basic link (acknowledged service in connectionless mode) without Frame Check Sequence
    void process_bl_adata_without_fcs(Address address, BitVector& vec);
    // Basic link (acknowledged service in connectionless mode) without Frame Check Sequence
    void process_bl_data_without_fcs(Address address, BitVector& vec);
    void process_bl_udata_without_fcs(Address address, BitVector& vec);
    void process_bl_ack_without_fcs(Address address, BitVector& vec);

    // Basic link (acknowledged service in connectionless mode) with Frame Check Sequence
    void process_bl_adata_with_fcs(Address address, BitVector& vec);
    // Basic link (acknowledged service in connectionless mode) with Frame Check Sequence
    void process_bl_data_with_fcs(Address address, BitVector& vec);
    void process_bl_udata_with_fcs(Address address, BitVector& vec);
    void process_bl_ack_with_fcs(Address address, BitVector& vec);

    void process_supplementary_llc_pdu(Address address, BitVector& vec);

    static auto compute_fcs(std::vector<uint8_t> const& data) -> uint32_t;
    static auto check_fcs(BitVector& vec) -> bool;

    MobileLinkEntity mle_;
};

auto operator<<(std::ostream& stream, const LogicalLinkControl& llc) -> std::ostream&;