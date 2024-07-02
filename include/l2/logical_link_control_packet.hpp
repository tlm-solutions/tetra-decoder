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

    BasicLinkInformation() = delete;

    /// construct a BasicLinkInformation from a BitVector
    explicit BasicLinkInformation(BitVector& data);
};

auto operator<<(std::ostream& stream, const BasicLinkInformation& bli) -> std::ostream&;

/// The packet that is parsed in the logical link control layer. Currently we only implement basic link.
struct LogicalLinkControlPacket : public UpperMacCPlaneSignallingPacket {
    std::optional<BasicLinkInformation> basic_link_information_;
    BitVector tl_sdu_;

    LogicalLinkControlPacket() = delete;

    explicit LogicalLinkControlPacket(const UpperMacCPlaneSignallingPacket& packet);
};

auto operator<<(std::ostream& stream, const LogicalLinkControlPacket& llc) -> std::ostream&;