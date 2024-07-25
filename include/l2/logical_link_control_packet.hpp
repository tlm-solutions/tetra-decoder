/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l2/upper_mac_packet.hpp"
#include "utils/bit_vector.hpp"
#include <optional>

/// The type of the basic link packet.
enum class BasicLinkType {
    kBlAdataWithoutFcs,
    kBlDataWithoutFcs,
    kBlUdataWithoutFcs,
    kBlAckWithoutFcs,
    kBlAdataWithFcs,
    kBlDataWithFcs,
    kBlUdataWithFcs,
    kBlAckWithFcs,
};

constexpr auto to_string(BasicLinkType type) noexcept -> const char* {
    switch (type) {
    case BasicLinkType::kBlAdataWithoutFcs:
        return "BL-ADATA without FCS";
    case BasicLinkType::kBlDataWithoutFcs:
        return "BL-DATA without FCS";
    case BasicLinkType::kBlUdataWithoutFcs:
        return "BL-UDATA without FCS";
    case BasicLinkType::kBlAckWithoutFcs:
        return "BL-ACK without FCS";
    case BasicLinkType::kBlAdataWithFcs:
        return "BL-ADATA with FCS";
    case BasicLinkType::kBlDataWithFcs:
        return "BL-DATA with FCS";
    case BasicLinkType::kBlUdataWithFcs:
        return "BL-UDATA with FCS";
    case BasicLinkType::kBlAckWithFcs:
        return "BL-ACK with FCS";
    }
};

struct BasicLinkInformation {
    BasicLinkType basic_link_type_;
    std::optional<unsigned _BitInt(1)> n_r_;
    std::optional<unsigned _BitInt(1)> n_s_;
    std::optional<bool> fcs_good_;

    BasicLinkInformation() = default;

    /// construct a BasicLinkInformation from a BitVector
    explicit BasicLinkInformation(BitVector& data);

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(BasicLinkInformation, basic_link_type_, n_r_, n_s_, fcs_good_)
};

auto operator<<(std::ostream& stream, const BasicLinkInformation& bli) -> std::ostream&;

/// The packet that is parsed in the logical link control layer. Currently we only implement basic link.
struct LogicalLinkControlPacket : public UpperMacCPlaneSignallingPacket {
    std::optional<BasicLinkInformation> basic_link_information_;
    /// The data that is passed from the Logical Link Control layer to the Mobile Link Entity
    BitVector tl_sdu_;

    LogicalLinkControlPacket() = default;

    explicit LogicalLinkControlPacket(const UpperMacCPlaneSignallingPacket& packet);

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(LogicalLinkControlPacket, burst_type_, logical_channel_, type_, encrypted_, address_,
                                   fragmentation_, fragmentation_on_stealling_channel_, reservation_requirement_,
                                   tm_sdu_, encryption_mode_, immediate_napping_permission_flag_,
                                   basic_slot_granting_element_, position_of_grant_, channel_allocation_element_,
                                   random_access_flag_, power_control_element_, basic_link_information_, tl_sdu_)
};

auto operator<<(std::ostream& stream, const LogicalLinkControlPacket& llc) -> std::ostream&;