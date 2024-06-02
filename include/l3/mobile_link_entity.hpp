/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#pragma once

#include <cstdint>
#include <iostream>
#include <memory>

#include <l3/circuit_mode_control_entity.hpp>
#include <l3/mobile_management.hpp>
#include <reporter.hpp>
#include <utils/bit_vector.hpp>

class MobileLinkEntity {
  public:
    explicit MobileLinkEntity(std::shared_ptr<Reporter> reporter)
        : reporter_(std::move(reporter))
        , cmce_(std::make_unique<CircuitModeControlEntity>(reporter_))
        , mm_(std::make_unique<MobileManagement>()){};
    ~MobileLinkEntity() noexcept = default;

    void service_DMle_system_info(BitVector& vec);

    void service_user_pdu(AddressType address, BitVector& vec);

    friend auto operator<<(std::ostream& stream, const MobileLinkEntity& mle) -> std::ostream&;

  private:
    void service_data_pdu(AddressType address, BitVector& vec);
    void service_d_network_broadcast(const AddressType address, BitVector& vec);

    bool system_info_received_ = false;
    uint16_t location_area_ = 0;
    uint16_t subscriber_class_ = 0;
    uint8_t registration_ = 0;
    uint8_t deregistration_ = 0;
    uint8_t priority_cell_ = 0;
    uint8_t minimum_mode_service_ = 0;
    uint8_t migration_ = 0;
    uint8_t system_wide_service_ = 0;
    uint8_t tetra_voice_service_ = 0;
    uint8_t circuit_mode_data_service_ = 0;
    uint8_t sndcp_service_ = 0;
    uint8_t air_interface_encryption_service_ = 0;
    uint8_t advanced_link_supported_ = 0;

    // this variable is set to true if sync is received. this is not available in uplink
    bool is_downlink_ = false;

    std::shared_ptr<Reporter> reporter_{};
    std::unique_ptr<CircuitModeControlEntity> cmce_{};
    std::unique_ptr<MobileManagement> mm_{};
};

auto operator<<(std::ostream& stream, const MobileLinkEntity& mle) -> std::ostream&;