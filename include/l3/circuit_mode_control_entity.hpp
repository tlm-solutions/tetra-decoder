/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#ifndef L3_CMCE_HPP
#define L3_CMCE_HPP

#include <memory>

#include <l3/short_data_service.hpp>
#include <utils/bit_vector.hpp>

class CircuitModeControlEntity {
  public:
    CircuitModeControlEntity()
        : sds_(std::make_shared<ShortDataService>()){};
    ~CircuitModeControlEntity() noexcept = default;

    void process(bool is_downlink, BitVector& vec);

  private:
    void process_d_sds_data(BitVector& vec);

    std::shared_ptr<ShortDataService> sds_;
};

#endif
