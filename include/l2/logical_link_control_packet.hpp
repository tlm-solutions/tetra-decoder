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
#include <bitset>
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
            n_r_ = data.take<1>();
            n_s_ = data.take<1>();
            break;
        case 0b0001:
            n_s_ = data.take<1>();
            break;
        case 0b0010:
            break;
        case 0b0011:
            n_r_ = data.take<1>();
            break;
        case 0b0100:
            n_r_ = data.take<1>();
            n_s_ = data.take<1>();
            break;
        case 0b0101:
            n_s_ = data.take<1>();
            break;
        case 0b0110:
            break;
        case 0b0111:
            n_r_ = data.take<1>();
            break;
        default:
            throw std::runtime_error(
                "Did not expect something other than basic link in LogicalLinkControlPacket parser!");
        }

        basic_link_type_ = BasicLinkType(pdu_type);

        if (pdu_type >= 0b0100) {
            auto fcs = data.take_last<32>();
            auto computed_fcs = data.compute_fcs();
            fcs_good_ = fcs == computed_fcs;
        }
    }
};

inline auto operator<<(std::ostream& stream, const BasicLinkInformation& bli) -> std::ostream& {
    stream << "  Basic Link Information: " << to_string(bli.basic_link_type_);
    if (bli.n_r_) {
        stream << " N(R): " << std::bitset<1>(*bli.n_r_);
    }
    if (bli.n_s_) {
        stream << " N(S): " << std::bitset<1>(*bli.n_s_);
    }
    if (bli.fcs_good_) {
        stream << " FCS good: " << (*bli.fcs_good_ ? "true" : "false");
    }
    stream << std::endl;
    return stream;
}

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

inline auto operator<<(std::ostream& stream, const LogicalLinkControlPacket& llc) -> std::ostream& {
    if (llc.basic_link_information_) {
        stream << *llc.basic_link_information_;
    }
    stream << "  TL-SDU: " << llc.tl_sdu_ << std::endl;
    return stream;
}