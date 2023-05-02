#include <bitset>
#include <cassert>
#include <iostream>

#include <l3/short_data_service.hpp>

void ShortDataService::process(BitVector& vec) {
    auto protocol_identifier = vec.take(8);

    std::cout << "SDS " << std::bitset<8>(protocol_identifier) << std::endl;

    switch (protocol_identifier) {
    case 0b00000010:
        process_simple_text_messaging(vec);
        break;
    default:
        std::cout << "  " << vec << std::endl;
        break;
    }
}

void ShortDataService::process_simple_text_messaging(BitVector& vec) {
    auto reserved = vec.take(1);
    auto text_coding_scheme = vec.take(7);

    std::string decoded = "";
    for (auto bits = vec.take(8); vec.bits_left() >= 8; bits = vec.take(8)) {
        std::cout << "  chars: 0b" << std::bitset<8>(bits);
        if (0x20 <= bits && bits <= 0x7e) {
            decoded += (char)bits;
        } else {
            decoded += ".";
        }
    }

    std::cout << "  text_coding_scheme: 0b" << std::bitset<7>(text_coding_scheme) << std::endl;
    std::cout << "  decoded: " << decoded << std::endl;
    std::cout << "  " << vec << std::endl;
}
