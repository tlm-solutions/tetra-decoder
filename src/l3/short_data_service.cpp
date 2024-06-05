#include <bitset>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>

#include <l3/short_data_service.hpp>

void ShortDataService::process(const AddressType to_address, const AddressType from_address, BitVector& vec) {
    message_ = {};

    message_["type"] = "SDS";
    message_["to"] = to_address;
    message_["from"] = from_address;

    auto protocol_identifier = vec.take<8>();

    message_["protocol_identifier"] = static_cast<unsigned>(protocol_identifier);

    std::cout << "SDS " << std::bitset<8>(protocol_identifier) << std::endl;
    std::cout << "  From: " << from_address << "To: " << to_address << std::endl;

    {
        auto vec_copy = BitVector();
        vec_copy.append(vec);

        message_["data"] = nlohmann::json::array();

        for (uint64_t bits; vec_copy.bits_left() >= 8;) {
            bits = vec_copy.take<8>();
            message_["data"].push_back(bits);
        }

        message_["bits_in_last_byte"] = vec_copy.bits_left();
        message_["data"].push_back(static_cast<unsigned>(vec_copy.take_last(vec_copy.bits_left())));
    }

    switch (protocol_identifier) {
    case 0b00000010:
        process_simple_text_messaging(to_address, from_address, vec);
        break;
    case 0b00001010:
        process_location_information_protocol(to_address, from_address, vec);
        break;
    default:
        process_default(to_address, from_address, vec);
        break;
    }

    reporter_->emit_report(message_);
}

void ShortDataService::process_default(const AddressType to_address, const AddressType from_address, BitVector& vec) {

    std::stringstream stream;
    for (uint64_t bits; vec.bits_left() >= 8;) {
        bits = vec.take<8>();
        stream << " " << std::hex << std::setw(2) << std::setfill('0') << bits;
    }

    std::cout << "  decoded: " << stream.str() << std::endl;
    std::cout << "  " << vec << std::endl;
}

void ShortDataService::process_simple_text_messaging(const AddressType to_address, const AddressType from_address,
                                                     BitVector& vec) {
    auto reserved = vec.take<1>();
    auto text_coding_scheme = vec.take<7>();

    std::stringstream stream;
    for (uint64_t bits; vec.bits_left() >= 8;) {
        bits = vec.take<8>();
        stream << " " << std::hex << std::setw(2) << std::setfill('0') << bits;
    }

    std::cout << "  text_coding_scheme: 0b" << std::bitset<7>(text_coding_scheme) << std::endl;
    std::cout << "  decoded: " << stream.str() << std::endl;
    std::cout << "  " << vec << std::endl;
}

static auto integer_to_double(uint32_t data, std::size_t bits, double multiplier) -> double {
    if (data & (1 << (bits - 1))) {
        data = ~data;
        data += 1;
        data &= (0xFFFFFFFF >> (32 - bits));

        return -1 * multiplier * data / static_cast<double>(1 << (bits - 1));
    } else {
        return multiplier * data / static_cast<double>(1 << (bits - 1));
    }
}

static auto decode_longitude(uint64_t v) -> double {
    assert(v < std::pow(2, 25));

    return integer_to_double(static_cast<uint32_t>(v), 25, 180.0);
}

static auto decode_latitude(uint64_t v) -> double {
    assert(v < std::pow(2, 24));

    return integer_to_double(static_cast<uint32_t>(v), 24, 90.0);
}

static auto decode_position_error(uint64_t v) -> std::string {
    assert(v < 8);

    const std::string position_error[] = {"< 2 m",   "< 20 m",    "< 200 m",  "< 2 km",
                                          "< 20 km", "<= 200 km", "> 200 km", "unknown"};

    return position_error[v];
}

static auto decode_horizontal_velocity(uint64_t v) -> double {
    assert(v < std::pow(2, 7));

    if (v == 127) {
        return -1.0;
    } else {
        const double C = 16.0;
        const double x = 0.038;
        const double A = 13.0;
        const double B = 0.0;

        return C * std::pow((1 + x), (v - A)) + B;
    }
}

static auto decode_direction_of_travel(uint64_t v) -> std::string {
    assert(v < std::pow(2, 4));

    const std::string direction_of_travel[] = {"0 N",    "22.5 NNE",  "45 NE",  "67.5 ENE",  "90 E",   "112.5 ESE",
                                               "135 SE", "157.5 SSE", "180 S",  "202.5 SSW", "225 SW", "247.5 WSW",
                                               "270 W",  "292.5 WNW", "315 NW", "337.5 NNW"};

    return direction_of_travel[v];
}

void ShortDataService::process_location_information_protocol(const AddressType to_address,
                                                             const AddressType from_address, BitVector& vec) {
    auto pdu_type = vec.take<2>();

    if (pdu_type == 0b00) {
        // Short location report
        auto time_elapsed = vec.take<2>();
        auto longitude = decode_longitude(vec.take<25>());
        auto latitude = decode_latitude(vec.take<24>());
        auto position_error = decode_position_error(vec.take<3>());
        auto horizontal_velocity = decode_horizontal_velocity(vec.take<7>());
        auto direction_of_travel = decode_direction_of_travel(vec.take<4>());
        auto type_of_addition_data = vec.take<1>();
        auto additional_data = vec.take<8>();
        std::cout << "  Short Location Report" << std::endl;
        std::cout << "  Time elapsed: " << std::bitset<2>(time_elapsed) << " Lon: " << longitude << " Lat: " << latitude
                  << " Position error: " << position_error << " horizontal_velocity: " << horizontal_velocity
                  << " direction_of_travel: " << direction_of_travel
                  << " type_of_addition_data: " << std::bitset<1>(additional_data)
                  << " additional_data: " << std::bitset<8>(additional_data) << std::endl;
    } else if (pdu_type == 0b01) {
        // Location protocol PDU with extension
        auto pdu_type_extension = vec.take<4>();
        std::cout << "  Location Information Protocol extension: 0b" << std::bitset<4>(pdu_type_extension) << std::endl;
    } else {
        // reserved
    }
}
