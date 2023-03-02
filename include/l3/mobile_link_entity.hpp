/*
* Copyright (C) 2022 Transit Live Mapping Solutions
* All rights reserved.
*
* Authors:
*   Marenz Schmidl
*   Tassilo Tanneberger
*/

#ifndef L3_MLE_HPP
#define L3_MLE_HPP

#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

#include <utils/bit_vector.hpp>

class MobileLinkEntity {
  public:
    MobileLinkEntity() noexcept = default;
    ~MobileLinkEntity() noexcept = default;

    [[nodiscard]] inline auto mobile_country_code() const noexcept -> uint32_t { return mobile_country_code_; }
    [[nodiscard]] inline auto mobile_network_code() const noexcept -> uint32_t { return mobile_network_code_; }

    void service_DMle_sync(BitVector& vec);
    void service_DMle_system_info(BitVector& vec);

    friend std::ostream& operator<<(std::ostream& stream, const MobileLinkEntity& mle);

  private:
    bool sync_received_ = false;
    uint32_t mobile_country_code_ = 0; // mmc
    uint32_t mobile_network_code_ = 0; // mobile_network_code
    uint8_t dNwrk_broadcast_broadcast_supported_ = 0;
    uint8_t dNwrk_broadcast_enquiry_supported_ = 0;
    uint8_t cell_load_ca_ = 0;
    uint8_t late_entry_supported_ = 0;

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
};

std::ostream& operator<<(std::ostream& stream, const MobileLinkEntity& mle);

#endif
