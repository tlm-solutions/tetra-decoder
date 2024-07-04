/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "l3/mobile_management_packet.hpp"
#include "utils/address.hpp"
#include "utils/bit_vector.hpp"

struct ShortSubscriberIdentity {
  public:
    ShortSubscriberIdentity() = delete;
    explicit ShortSubscriberIdentity(BitVector& data) { address_.set_ssi(data.take<24>()); };

    // NOLINTNEXTLINE(google-explicit-constructor)
    operator Address() const noexcept { return address_; };

  private:
    Address address_;
};

struct MobileNetworkInformation {
  public:
    MobileNetworkInformation() = delete;
    explicit MobileNetworkInformation(BitVector& data) {
        address_.set_country_code(data.take<10>());
        address_.set_network_code(data.take<14>());
    };

    // NOLINTNEXTLINE(google-explicit-constructor)
    operator Address() const noexcept { return address_; };

  private:
    Address address_;
};

struct ScchInformationAndDistributionOn18ThFrame {
  public:
    ScchInformationAndDistributionOn18ThFrame() = delete;
    explicit ScchInformationAndDistributionOn18ThFrame(BitVector& data)
        : scch_information_(data.take<4>())
        , distribution_on_18th_frame_(data.take<2>()){};

    auto to(MobileManagementDownlinkLocationUpdateAccept* to_ref) -> void {
        to_ref->scch_information_ = scch_information_;
        to_ref->distribution_on_18th_frame_ = distribution_on_18th_frame_;
    };

  private:
    unsigned _BitInt(4) scch_information_;
    unsigned _BitInt(2) distribution_on_18th_frame_;
};

MobileManagementDownlinkLocationUpdateAccept::MobileManagementDownlinkLocationUpdateAccept(BitVector& data)
    : location_update_accept_type_(LocationUpdateAcceptType(data.take<4>())) {
    auto parser = Type234Parser<MobileManagementDownlinkType34ElementIdentifiers>(
        data,
        {MobileManagementDownlinkType34ElementIdentifiers::kSecurityDownlink,
         MobileManagementDownlinkType34ElementIdentifiers::kGroupIdentityLocationAccept,
         MobileManagementDownlinkType34ElementIdentifiers::kDefaultGroupAttachLifetime,
         MobileManagementDownlinkType34ElementIdentifiers::kAuthenticationDownlink,
         MobileManagementDownlinkType34ElementIdentifiers::kCellTypeControl,
         MobileManagementDownlinkType34ElementIdentifiers::kProprietary},
        {MobileManagementDownlinkType34ElementIdentifiers::kNewRegisteredArea,
         MobileManagementDownlinkType34ElementIdentifiers::kGroupIdentitySecurityRelatedInformation});

    address_.merge(parser.parse_type2<ShortSubscriberIdentity>(data));
    address_.merge(parser.parse_type2<MobileNetworkInformation>(data));
    subscriber_class_ = parser.parse_type2<BitVectorElement<16>>(data);
    energy_saving_information_ = parser.parse_type2<BitVectorElement<14>>(data);
    auto ssch = parser.parse_type2<ScchInformationAndDistributionOn18ThFrame>(data);
    if (ssch) {
        ssch->to(this);
    }
    optional_elements_ = parser.parse_type34(data);
};

MobileManagementPacket::MobileManagementPacket(const MobileLinkEntityPacket& packet)
    : MobileLinkEntityPacket(packet) {
    auto data = BitVector(sdu_);
    auto pdu_type = data.take<4>();
    if (is_downlink()) {
        packet_type_ = MobileManagementDownlinkPacketType(pdu_type);
    } else {
        packet_type_ = MobileManagementUplinkPacketType(pdu_type);
    }

    if (packet_type_ == MobileManagementPacketType(MobileManagementDownlinkPacketType::kDLocationUpdateAccept)) {
        downlink_location_update_accept_ = MobileManagementDownlinkLocationUpdateAccept(data);
    }
};