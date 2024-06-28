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
#include <stdexcept>

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
    explicit BasicLinkInformation(BitVector& data) {
        auto pdu_type = data.take<4>();

        switch (pdu_type) {
        case 0b0000:
            basic_link_type_ = BasicLinkType::kBlAdataWithoutFcs;
            n_r_ = data.take<1>();
            n_s_ = data.take<1>();
            break;
        case 0b0001:
            basic_link_type_ = BasicLinkType::kBlDataWithoutFcs;
            n_s_ = data.take<1>();
            break;
        case 0b0010:
            basic_link_type_ = BasicLinkType::kBlUdataWithoutFcs;
            break;
        case 0b0011:
            basic_link_type_ = BasicLinkType::kBlAckWithoutFcs;
            n_r_ = data.take<1>();
            break;
        case 0b0100:
            basic_link_type_ = BasicLinkType::kBlAdataWithFcs;
            n_r_ = data.take<1>();
            n_s_ = data.take<1>();
            break;
        case 0b0101:
            basic_link_type_ = BasicLinkType::kBlDataWithFcs;
            n_s_ = data.take<1>();
            break;
        case 0b0110:
            basic_link_type_ = BasicLinkType::kBlUdataWithFcs;
            break;
        case 0b0111:
            basic_link_type_ = BasicLinkType::kBlAckWithFcs;
            n_r_ = data.take<1>();
            break;
        default:
            throw std::runtime_error(
                "Did not expect something other than basic link in LogicalLinkControlPacket parser!");
        }

        if (pdu_type >= 0b0100) {
            auto fcs = data.take_last<32>();
            auto computed_fcs = data.compute_fcs();
            fcs_good_ = fcs == computed_fcs;
        }
    }
};

auto operator<<(std::ostream& stream, const BasicLinkInformation& llc) -> std::ostream&;

/// The packet that is parsed in the logical link control layer. Currently we only implement basic link.
struct LogicalLinkControlPacket : public UpperMacCPlaneSignallingPacket {
    std::optional<BasicLinkInformation> basic_link_information_;
    BitVector tl_sdu_;

    LogicalLinkControlPacket() = delete;

    explicit LogicalLinkControlPacket(const UpperMacCPlaneSignallingPacket& packet)
        : UpperMacCPlaneSignallingPacket(packet) {
        auto data = BitVector(*tm_sdu_);
        basic_link_information_ = BasicLinkInformation(data);
        tl_sdu_ = data;
    }
};

auto operator<<(std::ostream& stream, const LogicalLinkControlPacket& llc) -> std::ostream&;