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
#include <utils/address.hpp>
#include <utils/bit_vector.hpp>

class ShortDataService {
  public:
    ShortDataService(std::shared_ptr<Reporter> reporter)
        : reporter_(std::move(reporter)){};
    ~ShortDataService() noexcept = default;

    void process(Address to_address, Address from_address, BitVector& vec);

  private:
    void process_simple_text_messaging(Address to_address, Address from_address, BitVector& vec);
    void process_location_information_protocol(Address to_address, Address from_address, BitVector& vec);
    void process_default(Address to_address, Address from_address, BitVector& vec);

    nlohmann::json message_;
    std::shared_ptr<Reporter> reporter_{};
};