/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include <memory>

#include <l3/short_data_service.hpp>
#include <reporter.hpp>
#include <utility>
#include <utils/address_type.hpp>
#include <utils/bit_vector.hpp>

class CircuitModeControlEntity {
  public:
    explicit CircuitModeControlEntity(std::shared_ptr<Reporter> reporter)
        : reporter_(std::move(reporter))
        , sds_(std::make_unique<ShortDataService>(reporter_)){};
    ~CircuitModeControlEntity() noexcept = default;

    void process(bool is_downlink, AddressType address, BitVector& vec);

  private:
    void process_d_sds_data(AddressType to_address, BitVector& vec);
    void process_u_sds_data(AddressType from_address, BitVector& vec);

    std::shared_ptr<Reporter> reporter_{};
    std::unique_ptr<ShortDataService> sds_{};
};