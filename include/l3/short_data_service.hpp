/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#ifndef L3_SDS_HPP
#define L3_SDS_HPP

#include <utils/bit_vector.hpp>

class ShortDataService {
  public:
    ShortDataService() noexcept = default;
    ~ShortDataService() noexcept = default;

    void process(BitVector& vec);

  private:
    void process_simple_text_messaging(BitVector& vec);
};

#endif
