/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include <l3/mobile_link_entity.hpp>
#include <reporter.hpp>
#include <utils/address.hpp>
#include <utils/bit_vector.hpp>

class LogicalLinkControl {
  public:
    LogicalLinkControl(std::shared_ptr<Reporter> reporter, std::shared_ptr<MobileLinkEntity> mle)
        : reporter_(std::move(reporter))
        , mle_(std::move(mle)){};
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

    std::shared_ptr<Reporter> reporter_{};
    std::shared_ptr<MobileLinkEntity> mle_{};
};

std::ostream& operator<<(std::ostream& stream, const LogicalLinkControl& llc);