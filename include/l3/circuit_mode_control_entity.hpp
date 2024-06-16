/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l3/short_data_service.hpp"
#include "reporter.hpp"
#include "utils/address.hpp"
#include "utils/bit_vector.hpp"

class CircuitModeControlEntity {
  public:
    CircuitModeControlEntity() = delete;
    explicit CircuitModeControlEntity(Reporter&& reporter)
        : sds_(ShortDataService(std::move(reporter))){};
    ~CircuitModeControlEntity() noexcept = default;

    void process(bool is_downlink, Address address, BitVector& vec);

  private:
    void process_d_sds_data(Address to_address, BitVector& vec);
    void process_u_sds_data(Address from_address, BitVector& vec);

    ShortDataService sds_;
};