#include <l2/lower_mac.hpp>

#include <fmt/color.h>
#include <fmt/core.h>

LowerMac::LowerMac(std::shared_ptr<Reporter> reporter)
    : reporter_(reporter) {
    viter_bi_codec_1614_ = std::make_shared<ViterbiCodec>();
    upper_mac_ = std::make_shared<UpperMac>(reporter_);
}

static auto vectorAppend(const std::vector<uint8_t>& vec, std::vector<uint8_t>& res, std::size_t pos,
                         std::size_t length) -> void {
    std::copy(vec.begin() + pos, vec.begin() + pos + length, std::back_inserter(res));
}

auto LowerMac::process(const std::vector<uint8_t>& frame, BurstType burst_type) -> std::vector<std::function<void()>> {
    std::vector<uint8_t> sb{};
    std::vector<uint8_t> bkn1{};
    std::vector<uint8_t> bkn2{};
    std::vector<uint8_t> bb{};
    std::vector<uint8_t> cb{};

    std::vector<std::function<void()>> functions{};

    // The BLCH may be mapped onto block 2 of the downlink slots, when a SCH/HD,
    // SCH-P8/HD or a BSCH is mapped onto block 1. The number of BLCH occurrences
    // on one carrier shall not exceed one per 4 multiframe periods.
    if (burst_type == BurstType::SynchronizationBurst) {
        upper_mac_->incrementTn();

        // sb contains BSCH
        // ✅ done
        uint8_t sb_desc[120];
        descramble(frame.data() + 94, sb_desc, 120, 0x0003);
        sb = deinterleave(sb_desc, 120, 11);
        sb = viter_bi_decode_1614(depuncture23(sb.data(), 120));
        if (check_crc_16_ccitt(sb.data(), 76)) {
            sb = std::vector(sb.begin(), sb.begin() + 60);
            upper_mac_->process_BSCH(burst_type, sb);
        }

        // bb contains AACH
        // ✅ done
        uint8_t bb_desc[30];
        descramble(frame.data() + 252, bb_desc, 30, upper_mac_->scrambling_code());
        bb = reed_muller_3014_decode(bb_desc);
        upper_mac_->process_AACH(burst_type, bb);

        // bkn2 block
        // ✅ done
        // any off SCH/HD, BNCH, STCH
        // see ETSI EN 300 392-2 V3.8.1 (2016-08) Figure 8.6: Error control
        // structure for π4DQPSK logical channels (part 2)
        uint8_t bkn2_desc[216];
        descramble(frame.data() + 282, bkn2_desc, 216, upper_mac_->scrambling_code());
        bkn2 = deinterleave(bkn2_desc, 216, 101);
        bkn2 = viter_bi_decode_1614(depuncture23(bkn2.data(), 216));
        // if the crc does not work, then it might be a BLCH
        if (check_crc_16_ccitt(bkn2.data(), 140)) {
            bkn2 = std::vector(bkn2.begin(), bkn2.begin() + 124);

            // SCH/HD or BNCH mapped
            upper_mac_->process_SCH_HD(burst_type, bkn2);
        }
    } else if (burst_type == BurstType::NormalDownlinkBurst) {
        upper_mac_->incrementTn();

        // bb contains AACH
        // ✅ done
        vectorAppend(frame, bb, 230, 14);
        vectorAppend(frame, bb, 266, 16);
        uint8_t bb_des[30];
        descramble(bb.data(), bb_des, 30, upper_mac_->scrambling_code());
        bb = reed_muller_3014_decode(bb_des);
        upper_mac_->process_AACH(burst_type, bb);

        // TCH or SCH/F
        vectorAppend(frame, bkn1, 14, 216);
        vectorAppend(frame, bkn1, 282, 216);
        uint8_t bkn1_desc[432];
        descramble(bkn1.data(), bkn1_desc, 432, upper_mac_->scrambling_code());

        if (upper_mac_->downlink_usage() == DownlinkUsage::Traffic && upper_mac_->time_slot() <= 17) {
            // TODO: handle TCH
            std::cout << "AACH indicated traffic with usagemarker: "
                      << std::to_string(upper_mac_->downlink_traffic_usage_marker()) << std::endl;
        } else {
            // control channel
            // ✅done
            bkn1 = deinterleave(bkn1_desc, 432, 103);
            bkn1 = viter_bi_decode_1614(depuncture23(bkn1.data(), 432));
            if (check_crc_16_ccitt(bkn1.data(), 284)) {
                bkn1 = std::vector(bkn1.begin(), bkn1.begin() + 268);
                upper_mac_->process_SCH_F(burst_type, bkn1);
            }
        }
    } else if (burst_type == BurstType::NormalDownlinkBurstSplit) {
        upper_mac_->incrementTn();

        // bb contains AACH
        // ✅ done
        vectorAppend(frame, bb, 230, 14);
        vectorAppend(frame, bb, 266, 16);
        uint8_t bb_desc[30];
        descramble(bb.data(), bb_desc, 30, upper_mac_->scrambling_code());
        bb = reed_muller_3014_decode(bb_desc);
        upper_mac_->process_AACH(burst_type, bb);

        // STCH + TCH
        // STCH + STCH
        uint8_t bkn1_desc[216];
        uint8_t bkn2_desc[216];
        descramble(frame.data() + 14, bkn1_desc, 216, upper_mac_->scrambling_code());
        descramble(frame.data() + 282, bkn2_desc, 216, upper_mac_->scrambling_code());

        if (upper_mac_->downlink_usage() == DownlinkUsage::Traffic && upper_mac_->time_slot() <= 17) {
            bkn1 = deinterleave(bkn1_desc, 216, 101);
            bkn1 = viter_bi_decode_1614(depuncture23(bkn1.data(), 216));
            if (check_crc_16_ccitt(bkn1.data(), 140)) {
                bkn1 = std::vector(bkn1.begin(), bkn1.begin() + 124);
                upper_mac_->process_STCH(burst_type, bkn1);
            }

            if (upper_mac_->second_slot_stolen()) {
                bkn2 = deinterleave(bkn2_desc, 216, 101);
                bkn2 = viter_bi_decode_1614(depuncture23(bkn2.data(), 216));
                if (check_crc_16_ccitt(bkn2.data(), 140)) {
                    bkn2 = std::vector(bkn2.begin(), bkn2.begin() + 124);
                    upper_mac_->process_STCH(burst_type, bkn2);
                }
            } else {
                // TODO: handle this TCH
                std::cout << "AACH indicated traffic with usagemarker: "
                          << std::to_string(upper_mac_->downlink_traffic_usage_marker()) << std::endl;
            }
        } else {
            // SCH/HD + SCH/HD
            // SCH/HD + BNCH
            // ✅done

            bkn1 = deinterleave(bkn1.data(), 216, 101);
            bkn1 = viter_bi_decode_1614(depuncture23(bkn1.data(), 216));
            if (check_crc_16_ccitt(bkn1.data(), 140)) {
                bkn1 = std::vector(bkn1.begin(), bkn1.begin() + 124);
                upper_mac_->process_SCH_HD(burst_type, bkn1);
            }
            // control channel
            bkn2 = deinterleave(bkn2.data(), 216, 101);
            bkn2 = viter_bi_decode_1614(depuncture23(bkn2.data(), 216));
            if (check_crc_16_ccitt(bkn2.data(), 140)) {
                bkn2 = std::vector(bkn2.begin(), bkn2.begin() + 124);
                upper_mac_->process_SCH_HD(burst_type, bkn2);
            }
        }
    } else if (burst_type == BurstType::ControlUplinkBurst) {
        vectorAppend(frame, cb, 4, 84);
        vectorAppend(frame, cb, 118, 84);
        uint8_t cb_desc[168];
        descramble(cb.data(), cb_desc, 168, upper_mac_->scrambling_code());

        // XXX: assume to be control channel
        cb = deinterleave(cb_desc, 168, 13);
        cb = viter_bi_decode_1614(depuncture23(cb.data(), 168));
        if (check_crc_16_ccitt(cb.data(), 108)) {
            cb = std::vector(cb.begin(), cb.begin() + 92);
            functions.push_back(std::bind(&UpperMac::process_SCH_HU, upper_mac_, burst_type, cb));
        }
    } else if (burst_type == BurstType::NormalUplinkBurst) {
        vectorAppend(frame, bkn1, 4, 216);
        vectorAppend(frame, bkn1, 242, 216);
        uint8_t bkn1_desc[432];
        descramble(bkn1.data(), bkn1_desc, 432, upper_mac_->scrambling_code());

        // XXX: assume to be control channel
        bkn1 = deinterleave(bkn1_desc, 432, 103);
        bkn1 = viter_bi_decode_1614(depuncture23(bkn1.data(), 432));
        if (check_crc_16_ccitt(bkn1.data(), 284)) {
            bkn1 = std::vector(bkn1.begin(), bkn1.begin() + 268);
            // fmt::print("NUB Burst crc good\n");
            functions.push_back(std::bind(&UpperMac::process_SCH_F, upper_mac_, burst_type, bkn1));
        }
    } else if (burst_type == BurstType::NormalUplinkBurstSplit) {
        // TODO: finish NormalUplinkBurstSplit implementation
        uint8_t bkn1_desc[216];
        uint8_t bkn2_desc[216];
        descramble(frame.data() + 4, bkn1_desc, 216, upper_mac_->scrambling_code());
        descramble(frame.data() + 242, bkn2_desc, 216, upper_mac_->scrambling_code());

        bkn1 = deinterleave(bkn1_desc, 216, 101);
        bkn1 = viter_bi_decode_1614(depuncture23(bkn1.data(), 216));
        if (check_crc_16_ccitt(bkn1.data(), 140)) {
            bkn1 = std::vector(bkn1.begin(), bkn1.begin() + 124);
            // fmt::print("NUB_S 1 Burst crc good\n");
            functions.push_back(std::bind(&UpperMac::process_STCH, upper_mac_, burst_type, bkn1));
        }

        bkn2 = deinterleave(bkn2_desc, 216, 101);
        bkn2 = viter_bi_decode_1614(depuncture23(bkn2.data(), 216));
        if (check_crc_16_ccitt(bkn2.data(), 140)) {
            bkn2 = std::vector(bkn2.begin(), bkn2.begin() + 124);
            if (upper_mac_->second_slot_stolen()) {
                // fmt::print("NUB_S 2 Burst crc good\n");
                functions.push_back(std::bind(&UpperMac::process_STCH, upper_mac_, burst_type, bkn2));
            }
        }
    } else {
        throw std::runtime_error("LowerMac does not implement the burst type supplied");
    }

    return functions;
}
