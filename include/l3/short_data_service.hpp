/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include <reporter.hpp>
#include <utility>
#include <utils/address_type.hpp>
#include <utils/bit_vector.hpp>

class ShortDataService {
  public:
    ShortDataService(std::shared_ptr<Reporter> reporter)
        : reporter_(std::move(reporter)){};
    ~ShortDataService() noexcept = default;

    void process(AddressType to_address, AddressType from_address, BitVector& vec);

  private:
    void process_simple_text_messaging(AddressType to_address, AddressType from_address, BitVector& vec);
    void process_location_information_protocol(AddressType to_address, AddressType from_address, BitVector& vec);
    void process_default(AddressType to_address, AddressType from_address, BitVector& vec);

    nlohmann::json message_;
    std::shared_ptr<Reporter> reporter_{};
};