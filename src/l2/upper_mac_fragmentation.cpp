#include <l2/upper_mac.hpp>

void UpperMac::fragmentation_start_burst() {
    // TODO: clear last_address_type_ for uplink
    fragment_end_received_ = false;
    // fragment_list_ = {};
}

void UpperMac::fragmentation_end_burst() {
    // TODO: do the processing here

    // no fragments received, return
    if (fragment_list_.empty()) {
        return;
    }

    if (fragment_map_.find(last_address_type_) == fragment_map_.end()) {
        std::cout << "MAC fragementation error. Could not find start fragment for address " << last_address_type_
                  << std::endl;
        return;
    }

    // append fragments
    for (auto it = fragment_list_.begin(); it != fragment_list_.end(); it++) {
        fragment_map_[last_address_type_].push_back(*it);
    }

    // frag end received. send to logical link control.
    if (fragment_end_received_) {
        auto tm_sdu = BitVector({});

        // combine to tm_sdu
        for (auto it = fragment_map_[last_address_type_].begin(); it != fragment_map_[last_address_type_].end(); it++) {
            auto bit_vec = *it;
            tm_sdu.append(bit_vec.take_vector(bit_vec.bits_left()));
        }

        // remove key from the map
        fragment_map_.erase(last_address_type_);
        fragment_list_.clear();

        logical_link_control_->process(last_address_type_, tm_sdu);
    }
}

void UpperMac::fragmentation_push_tm_sdu_start(AddressType address_type, BitVector& vec) {
    fragment_map_[address_type] = std::vector<BitVector>({vec});
    last_address_type_ = address_type;
    fragment_list_.clear();
    std::cout << "MAC frag start inserting Address: " << address_type << std::endl;
}

void UpperMac::fragmentation_push_tm_sdu_frag(BitVector& vec) {
    fragment_list_.push_back(vec);
    std::cout << "MAC frag frag" << std::endl;
}

void UpperMac::fragmentation_push_tm_sdu_end(BitVector& vec) {
    fragment_end_received_ = true;
    fragment_list_.push_back(vec);
    std::cout << "MAC frag end" << std::endl;
}
