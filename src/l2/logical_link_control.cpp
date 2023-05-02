#include <bitset>
#include <cassert>

#include <l2/logical_link_control.hpp>

void LogicalLinkControl::process(AddressType address_type, BitVector& vec) {
    std::cout << "LLC received: " << std::endl;
    std::cout << "  Address: " << address_type << std::endl;
    std::cout << "  Data: " << vec << std::endl;
}

std::ostream& operator<<(std::ostream& stream, const LogicalLinkControl& llc) { return stream; }
