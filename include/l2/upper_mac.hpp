/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#pragma once

#include "utils/bit_vector.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include <burst_type.hpp>
#include <l2/logical_link_control.hpp>
#include <l3/mobile_link_entity.hpp>
#include <reporter.hpp>
#include <utils/address_type.hpp>

class UpperMac {
  public:
    UpperMac() = delete;
    UpperMac(std::shared_ptr<Reporter> reporter, bool is_downlink)
        : reporter_(std::move(reporter))
        , mobile_link_entity_(std::make_shared<MobileLinkEntity>(reporter_, is_downlink))
        , logical_link_control_(std::make_unique<LogicalLinkControl>(reporter_, mobile_link_entity_)){};
    ~UpperMac() noexcept = default;

  private:
    std::shared_ptr<Reporter> reporter_{};

    // fragmentation
    // XXX: might have to delay processing as SSI may only be known after the Null PDU
    void fragmentation_start_burst();
    void fragmentation_end_burst();
    void fragmentation_push_tm_sdu_start(AddressType address_type, BitVector& vec);
    void fragmentation_push_tm_sdu_frag(BitVector& vec);
    void fragmentation_push_tm_sdu_end(BitVector& vec);
    void fragmentation_push_tm_sdu_end_hu(BitVector& vec);

    std::shared_ptr<MobileLinkEntity> mobile_link_entity_{};
    std::unique_ptr<LogicalLinkControl> logical_link_control_{};

    // hashmap to keep track of framented mac segments
    std::unordered_map<AddressType, std::vector<BitVector>> fragment_map_ = {};
    std::vector<BitVector> fragment_list_{};
    bool fragment_end_received_{};
    bool fragment_end_hu_received_{};
    AddressType last_address_type_{};
    // save the last MAC-ACCESS or MAC-DATA where reservation_requirement is 0b0000 (1 sublot) for END-HU
    AddressType last_address_type_end_hu_{};
};