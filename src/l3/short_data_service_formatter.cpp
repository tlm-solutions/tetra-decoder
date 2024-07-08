/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "l3/short_data_service_packet.hpp"

auto operator<<(std::ostream& stream, const ShortLocationReport& slr) -> std::ostream& {
    stream << "  Short Location Report" << std::endl;
    stream << "  Time elapsed: " << std::bitset<2>(slr.time_elapsed_) << " Lon: " << slr.longitude_
           << " Lat: " << slr.latitude_ << " Position error: " << slr.position_error_
           << " horizontal_velocity: " << slr.horizontal_velocity_
           << " direction_of_travel: " << slr.direction_of_travel_
           << " type_of_addition_data: " << std::bitset<1>(slr.additional_data_)
           << " additional_data: " << std::bitset<8>(slr.additional_data_) << std::endl;
    return stream;
}

auto operator<<(std::ostream& stream, const LocationInformationProtocol& lip) -> std::ostream& {
    if (lip.short_location_report_) {
        stream << *lip.short_location_report_ << std::endl;
    } else if (lip.pdu_type_ == 0b01) {
        stream << "Location protocol PDU with extension" << std::endl;
    } else {
        stream << "Reserved LIP" << std::endl;
    }
    return stream;
}

auto operator<<(std::ostream& stream, const ShortDataServicePacket& sds) -> std::ostream& {
    stream << "SDS " << std::bitset<8>(sds.protocol_identifier_) << std::endl;
    stream << "  L2 Address: " << sds.address_ << " SDS Address: " << sds.sds_data_->address_ << std::endl;
    if (sds.location_information_protocol_) {
        stream << *sds.location_information_protocol_;
    }
    stream << "  Data: " << sds.sds_data_->data_ << std::endl;
    stream << "  decoded: ";
    const auto len = sds.sds_data_->data_.bits_left();
    for (auto i = 8; i + 8 <= len; i += 8) {
        auto bits = sds.sds_data_->data_.look<8>(i);
        stream << " " << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(bits);
    }
    stream << std::endl;

    return stream;
}