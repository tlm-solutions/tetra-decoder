#include <l2/upper_mac.hpp>

void UpperMac::fragmentation_start_burst() {
    // TODO: clear last_address_type_ for uplink
    fragment_end_received_ = false;
    fragment_end_hu_received_ = false;
    // fragment_list_ = {};
}

void UpperMac::fragmentation_end_burst() {
    // TODO: do the processing here

    // no fragments received, return
    if (fragment_list_.empty()) {
        return;
    }

    // exception for MAC-END-HU
    if (fragment_end_hu_received_ == true) {
        if (fragment_map_.find(last_address_type_end_hu_) == fragment_map_.end()) {
            std::cout << "MAC fragementation error. Could not find start fragment for address "
                      << last_address_type_end_hu_ << std::endl;
            return;
        }

        if (fragment_map_[last_address_type_end_hu_].size() != 1) {
            std::cout << "MAC fragementation error. MAC-END-HU unplausible for address " << last_address_type_end_hu_
                      << ". Aborting." << std::endl;
            return;
        }

        if (fragment_list_.size() != 1) {
            std::cout << "MAC fragementation error. MAC-END-HU unplausible amount of fragments: "
                      << fragment_list_.size() << std::endl;
            return;
        }

        // append fragment
        fragment_map_[last_address_type_end_hu_].push_back(fragment_list_[0]);

        fragment_list_.clear();

        auto tm_sdu = BitVector();

        // combine to tm_sdu
        for (auto it = fragment_map_[last_address_type_end_hu_].begin();
             it != fragment_map_[last_address_type_end_hu_].end(); it++) {
            tm_sdu.append(*it);
        }

        // remove key from the map
        fragment_map_.erase(last_address_type_end_hu_);

        logical_link_control_->process(last_address_type_end_hu_, tm_sdu);

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

    fragment_list_.clear();

    // frag end received. send to logical link control.
    if (fragment_end_received_) {
        auto tm_sdu = BitVector();

        // combine to tm_sdu
        for (auto it = fragment_map_[last_address_type_].begin(); it != fragment_map_[last_address_type_].end(); it++) {
            tm_sdu.append(*it);
        }

        // remove key from the map
        fragment_map_.erase(last_address_type_);

        logical_link_control_->process(last_address_type_, tm_sdu);
    }
}

void UpperMac::fragmentation_push_tm_sdu_start(const AddressType address_type, BitVector& vec) {
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

void UpperMac::fragmentation_push_tm_sdu_end_hu(BitVector& vec) {
    fragment_end_hu_received_ = true;
    fragment_list_.push_back(vec);
    std::cout << "MAC frag end HU" << std::endl;
}
