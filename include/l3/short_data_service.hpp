/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "reporter.hpp"
#include "utils/address.hpp"
#include "utils/bit_vector.hpp"

class ShortDataService {
  public:
    ShortDataService() = delete;
    ShortDataService(Reporter&& reporter)
        : reporter_(reporter){};
    ~ShortDataService() noexcept = default;

    void process(Address to_address, Address from_address, BitVector& vec);

  private:
    void process_simple_text_messaging(Address to_address, Address from_address, BitVector& vec);
    void process_location_information_protocol(Address to_address, Address from_address, BitVector& vec);
    void process_default(Address to_address, Address from_address, BitVector& vec);

    nlohmann::json message_;
    Reporter reporter_;
};