#include <bitset>
#include <cassert>

#include <l3/mobile_link_entity.hpp>

void MobileLinkEntity::service_DMle_sync(BitVector& vec) {
    assert(vec.bits_left() == 29);

    mobile_country_code_ = vec.take(10);
    mobile_network_code_ = vec.take(14);
    dNwrk_broadcast_broadcast_supported_ = vec.take(1);
    dNwrk_broadcast_enquiry_supported_ = vec.take(1);
    cell_load_ca_ = vec.take(2);
    late_entry_supported_ = vec.take(1);

    sync_received_ = true;
    is_downlink_ = true;

    std::cout << *this;
}

void MobileLinkEntity::service_DMle_system_info(BitVector& vec) {
    assert(vec.bits_left() == 42);

    // Location area (14)
    location_area_ = vec.take(14);
    // Subscriber class (16)
    subscriber_class_ = vec.take(16);
    // BS service details (12)
    registration_ = vec.take(1);
    deregistration_ = vec.take(1);
    priority_cell_ = vec.take(1);
    minimum_mode_service_ = vec.take(1);
    migration_ = vec.take(1);
    system_wide_service_ = vec.take(1);
    tetra_voice_service_ = vec.take(1);
    circuit_mode_data_service_ = vec.take(1);
    sndcp_service_ = vec.take(1);
    air_interface_encryption_service_ = vec.take(1);
    advanced_link_supported_ = vec.take(1);

    system_info_received_ = true;

    std::cout << *this;
}

void MobileLinkEntity::service_user_pdu(const AddressType address, BitVector& vec) {
    std::string mle_pdu[] = {"Reserved",
                             "MM protocol",
                             "CMCE protocol",
                             "Reserved",
                             "SNDCP protocol",
                             "MLE protocol",
                             "TETRA management entity protocol",
                             "Reserved for testing"};

    auto pdu_type = vec.take(3);

    std::cout << "MLE " << mle_pdu[pdu_type] << " " << vec << std::endl;

    switch (pdu_type) {
    case 0b001:
        mm_->process(is_downlink_, address, vec);
        break;
    case 0b010:
        cmce_->process(is_downlink_, address, vec);
        break;
    case 0b101:
        service_data_pdu(address, vec);
        break;
    default:
        break;
    }
}

void MobileLinkEntity::service_data_pdu(const AddressType address, BitVector& vec) {

    std::string mle_downlink_pdu_type[] = {
        "D-NEW-CELL",    "D-PREPARE-FAIL", "D-NWRK-BROADCAST",   "D-NWRK-BROADCAST EXTENSION",
        "D-RESTORE-ACK", "D-RESTORE-FAIL", "D-CHANNEL RESPONSE", "Extended PDU"};
    std::string mle_uplink_pdu_type[] = {
        "U-PREPARE", "U-PREPARE-DA", "U-IRREGULAR CHANNEL ADVICE", "U-CHANNEL CLASS ADVICE",
        "U-RESTORE", "Reserved",     "U-CHANNEL REQUEST",          "Extended PDU"};

    std::string mle_downlink_pdu_type_extension[] = {"D-NWRK-BROADCAST-DA",
                                                     "D-NWRK-BROADCAST REMOVE",
                                                     "Reserved",
                                                     "Reserved",
                                                     "Reserved",
                                                     "Reserved",
                                                     "Reserved",
                                                     "Reserved",
                                                     "Reserved",
                                                     "Reserved",
                                                     "Reserved",
                                                     "Reserved",
                                                     "Reserved",
                                                     "Reserved",
                                                     "Reserved",
                                                     "Reserved"};
    std::string mle_uplink_pdu_type_extension[] = {
        "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
        "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    };

    auto pdu_type = vec.take(3);
}

auto operator<<(std::ostream& stream, const MobileLinkEntity& mle) -> std::ostream& {
    if (mle.sync_received_) {
        stream << "D-MLE-SYNC:" << std::endl;
        stream << "  MCC: " << mle.mobile_country_code_ << std::endl;
        stream << "  MNC: " << mle.mobile_network_code_ << std::endl;
        stream << "  Neighbour cell broadcast: "
               << (mle.dNwrk_broadcast_broadcast_supported_ ? "supported" : "not supported") << std::endl;
        stream << "  Neighbour cell enquiry: "
               << (mle.dNwrk_broadcast_enquiry_supported_ ? "supported" : "not supported") << std::endl;
        stream << "  Cell load CA: ";
        switch (mle.cell_load_ca_) {
        case 0b00:
            stream << "Cell load unknown";
            break;
        case 0b01:
            stream << "Low cell load";
            break;
        case 0b10:
            stream << "Medium cell load";
            break;
        case 0b11:
            stream << "High cell load";
            break;
        default:
            break;
        }
        stream << std::endl;
        stream << "  Late entry supported: "
               << (mle.late_entry_supported_ ? "Late entry available" : "Late entry not supported") << std::endl;
    }

    if (mle.system_info_received_) {
        stream << "D-MLE-SYSINFO:" << std::endl;
        stream << "  Location Area (LA): " << mle.location_area_ << std::endl;
        stream << "  Subscriber Class 1..16 allowed: 0b" << std::bitset<16>(mle.subscriber_class_) << std::endl;
        stream << "  "
               << (mle.registration_ ? "Registration mandatory on this cell" : "Registration not required on this cell")
               << std::endl;
        stream << "  "
               << (mle.deregistration_ ? "De-registration requested on this cell"
                                       : "De-registration not required on this cell")
               << std::endl;
        stream << "  " << (mle.priority_cell_ ? "Cell is a priority cell" : "Cell is not a priority cell") << std::endl;
        stream << "  " << (mle.minimum_mode_service_ ? "Cell never uses minimum mode" : "Cell may use minimum mode")
               << std::endl;
        stream << "  "
               << (mle.migration_ ? "Migration is supported by this cell" : "Migration is not supported by this cell")
               << std::endl;
        stream << "  "
               << (mle.system_wide_service_ ? "Normal mode (system wide services supported)"
                                            : "System wide services temporarily not supported")
               << std::endl;
        stream << "  "
               << (mle.tetra_voice_service_ ? "TETRA voice service is supported on this cell"
                                            : "TETRA voice service is not supported on this cell")
               << std::endl;
        stream << "  "
               << (mle.circuit_mode_data_service_ ? "Circuit mode data service is supported on this cell"
                                                  : "Circuit mode data service is not supported on this cell")
               << std::endl;
        stream << "  "
               << (mle.sndcp_service_ ? "SNDCP service is available on this cell"
                                      : "SNDCP service is not available on this cell")
               << std::endl;
        stream << "  "
               << (mle.air_interface_encryption_service_ ? "Air interface encryption is available on this cell"
                                                         : "Air interface encryption is not available on this cell")
               << std::endl;
        stream << "  "
               << (mle.advanced_link_supported_ ? "Advanced link is supported on this cell"
                                                : "Advanced link is not supported on this cell")
               << std::endl;
    }

    return stream;
}
