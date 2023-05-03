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
#include <utils/address_type.hpp>
#include <utils/bit_vector.hpp>

class CircuitModeControlEntity {
  public:
    CircuitModeControlEntity()
        : sds_(std::make_shared<ShortDataService>()){};
    ~CircuitModeControlEntity() noexcept = default;

    void process(bool is_downlink, const AddressType address, BitVector& vec);

  private:
    void process_d_sds_data(const AddressType to_address, BitVector& vec);
    void process_u_sds_data(const AddressType from_address, BitVector& vec);

    std::shared_ptr<ShortDataService> sds_;
};

#endif
