/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#ifndef L3_SDS_HPP
#define L3_SDS_HPP

#include <utils/address_type.hpp>
#include <utils/bit_vector.hpp>

class ShortDataService {
  public:
    ShortDataService() noexcept = default;
    ~ShortDataService() noexcept = default;

    void process(const AddressType to_address, const AddressType from_address, BitVector& vec);

  private:
    void process_simple_text_messaging(const AddressType to_address, const AddressType from_address, BitVector& vec);
    void process_location_information_protocol(const AddressType to_address, const AddressType from_address,
                                               BitVector& vec);
    void process_default(const AddressType to_address, const AddressType from_address, BitVector& vec);
};

#endif
