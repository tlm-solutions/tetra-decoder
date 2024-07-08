/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "l3/circuit_mode_control_entity_packet.hpp"

auto SdsData::from_d_sds_data(BitVector& data) -> SdsData {
    SdsData sds;
    auto calling_party_type_identifier = data.take<2>();

    if (calling_party_type_identifier == 1 || calling_party_type_identifier == 2) {
        sds.address_.set_ssi(data.take<24>());
    }
    if (calling_party_type_identifier == 2) {
        sds.address_.set_country_code(data.take<10>());
        sds.address_.set_network_code(data.take<14>());
    }

    auto short_data_type_identifier = data.take<2>();

    unsigned length_identifier = 0;
    switch (short_data_type_identifier) {
    case 0b00:
        length_identifier = 16;
        break;
    case 0b01:
        length_identifier = 32;
        break;
    case 0b10:
        length_identifier = 64;
        break;
    case 0b11:
        length_identifier = data.take<11>();
        break;
    }
    sds.data_ = data.take_vector(length_identifier);
    auto parser = Type234Parser<CircuitModeControlEntityType3ElementIdentifiers>(
        data,
        {CircuitModeControlEntityType3ElementIdentifiers::kExternalSubsriberNumber,
         CircuitModeControlEntityType3ElementIdentifiers::kDmMsAddress},
        {});
    sds.optional_elements_ = parser.parse_type34(data);

    return sds;
}

auto SdsData::from_u_sds_data(BitVector& data) -> SdsData {
    SdsData sds;
    sds.area_selection_ = data.take<4>();
    auto calling_party_type_identifier = data.take<2>();

    if (calling_party_type_identifier == 0) {
        sds.address_.set_sna(data.take<8>());
    }
    if (calling_party_type_identifier == 1 || calling_party_type_identifier == 2) {
        sds.address_.set_ssi(data.take<24>());
    }
    if (calling_party_type_identifier == 2) {
        sds.address_.set_country_code(data.take<10>());
        sds.address_.set_network_code(data.take<14>());
    }

    auto short_data_type_identifier = data.take<2>();
    unsigned length_identifier = 0;
    switch (short_data_type_identifier) {
    case 0b00:
        length_identifier = 16;
        break;
    case 0b01:
        length_identifier = 32;
        break;
    case 0b10:
        length_identifier = 64;
        break;
    case 0b11:
        length_identifier = data.take<11>();
        break;
    }
    sds.data_ = data.take_vector(length_identifier);
    auto parser = Type234Parser<CircuitModeControlEntityType3ElementIdentifiers>(
        data,
        {CircuitModeControlEntityType3ElementIdentifiers::kExternalSubsriberNumber,
         CircuitModeControlEntityType3ElementIdentifiers::kDmMsAddress},
        {});
    sds.optional_elements_ = parser.parse_type34(data);

    return sds;
}

CircuitModeControlEntityPacket::CircuitModeControlEntityPacket(const MobileLinkEntityPacket& packet)
    : MobileLinkEntityPacket(packet) {
    auto data = BitVector(sdu_);

    auto pdu_type = data.take<5>();
    if (is_downlink()) {
        packet_type_ = CircuitModeControlEntityDownlinkPacketType(pdu_type);
    } else {
        packet_type_ = CircuitModeControlEntityUplinkPacketType(pdu_type);
    }

    if (packet_type_ == CircuitModeControlEntityPacketType(CircuitModeControlEntityDownlinkPacketType::kDSdsData)) {
        sds_data_ = SdsData::from_d_sds_data(data);
    }

    if (packet_type_ == CircuitModeControlEntityPacketType(CircuitModeControlEntityUplinkPacketType::kUSdsData)) {
        sds_data_ = SdsData::from_u_sds_data(data);
    }
}