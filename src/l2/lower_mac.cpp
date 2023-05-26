#include <l2/lower_mac.hpp>

#include <fmt/color.h>
#include <fmt/core.h>

LowerMac::LowerMac(std::shared_ptr<Reporter> reporter)
    : reporter_(reporter) {
    /*
     * Initialize Viterbi coder/decoder for MAC
     *
     * 8.2.3.1.1 Generator polynomials for the RCPC 16-state mother code of rate
     * 1/4
     *
     * G1 = 1 + D +             D^4 (8.3)
     * G2 = 1 +     D^2 + D^3 + D^4 (8.4)
     * G3 = 1 + D + D^2 +       D^4 (8.5)
     * G4 = 1 + D +       D^3 + D^4 (8.6)
     *
     * NOTE: representing bit order must be reversed for the codec, eg. 1 + D + 0
     * + 0 + D^4 -> 10011
     *
     */
    std::vector<int> polynomials;
    int constraint = 6;

    polynomials.push_back(0b10011);
    polynomials.push_back(0b11101);
    polynomials.push_back(0b10111);
    polynomials.push_back(0b11011);
    viter_bi_codec_1614_ = std::make_unique<ViterbiCodec>(constraint, polynomials);

    upper_mac_ = std::make_unique<UpperMac>(reporter_);
}

static auto vectorExtract(const std::vector<uint8_t>& vec, size_t pos, size_t length) -> std::vector<uint8_t> {
    std::vector<uint8_t> res;

    std::copy(vec.begin() + pos, vec.begin() + pos + length, std::back_inserter(res));

    return res;
}

static auto vectorAppend(const std::vector<uint8_t>& vec, std::vector<uint8_t>& res, std::size_t pos,
                         std::size_t length) -> std::vector<uint8_t> {
    std::copy(vec.begin() + pos, vec.begin() + pos + length, std::back_inserter(res));

    return res;
}

auto LowerMac::process(const std::vector<uint8_t>& frame, BurstType burst_type) -> bool {
    std::vector<uint8_t> sb;
    std::vector<uint8_t> bkn1;
    std::vector<uint8_t> bkn2;
    std::vector<uint8_t> bb;
    std::vector<uint8_t> cb;

    // The BLCH may be mapped onto block 2 of the downlink slots, when a SCH/HD,
    // SCH-P8/HD or a BSCH is mapped onto block 1. The number of BLCH occurrences
    // on one carrier shall not exceed one per 4 multiframe periods.
    if (burst_type == BurstType::SynchronizationBurst) {
        upper_mac_->incrementTn();

        // sb contains BSCH
        // ✅ done
        sb = vectorExtract(frame, 94, 120);
        sb = descramble(sb, 120, 0x0003);
        sb = deinterleave(sb, 120, 11);
        sb = depuncture23(sb, 120);
        sb = viter_bi_decode_1614(sb);
        if (check_crc_16_ccitt(sb, 76)) {
            sb = vectorExtract(sb, 0, 60);
            upper_mac_->process_BSCH(burst_type, sb);
        }

        // bb contains AACH
        // ✅ done
        bb = vectorExtract(frame, 252, 30);
        bb = descramble(bb, 30, upper_mac_->scrambling_code());
        bb = reed_muller_3014_decode(bb);
        upper_mac_->process_AACH(burst_type, bb);

        // bkn2 block
        // ✅ done
        // any off SCH/HD, BNCH, STCH
        // see ETSI EN 300 392-2 V3.8.1 (2016-08) Figure 8.6: Error control
        // structure for π4DQPSK logical channels (part 2)
        bkn2 = vectorExtract(frame, 282, 216);
        bkn2 = descramble(bkn2, 216, upper_mac_->scrambling_code());
        bkn2 = deinterleave(bkn2, 216, 101);
        bkn2 = depuncture23(bkn2, 216);
        bkn2 = viter_bi_decode_1614(bkn2);
        // if the crc does not work, then it might be a BLCH
        if (check_crc_16_ccitt(bkn2, 140)) {
            bkn2 = vectorExtract(bkn2, 0, 124);

            // SCH/HD or BNCH mapped
            upper_mac_->process_SCH_HD(burst_type, bkn2);
        }
    } else if (burst_type == BurstType::NormalDownlinkBurst) {
        upper_mac_->incrementTn();

        // bb contains AACH
        // ✅ done
        bb = vectorExtract(frame, 230, 14);
        bb = vectorAppend(frame, bb, 266, 16);
        bb = descramble(bb, 30, upper_mac_->scrambling_code());
        bb = reed_muller_3014_decode(bb);
        upper_mac_->process_AACH(burst_type, bb);

        // TCH or SCH/F
        bkn1 = vectorExtract(frame, 14, 216);
        bkn1 = vectorAppend(frame, bkn1, 282, 216);
        bkn1 = descramble(bkn1, 432, upper_mac_->scrambling_code());

        if (upper_mac_->downlink_usage() == DownlinkUsage::Traffic && upper_mac_->time_slot() <= 17) {
            // TODO: handle TCH
            std::cout << "AACH indicated traffic with usagemarker: "
                      << std::to_string(upper_mac_->downlink_traffic_usage_marker()) << std::endl;
        } else {
            // control channel
            // ✅done
            bkn1 = deinterleave(bkn1, 432, 103);
            bkn1 = depuncture23(bkn1, 432);
            bkn1 = viter_bi_decode_1614(bkn1);
            if (check_crc_16_ccitt(bkn1, 284)) {
                bkn1 = vectorExtract(bkn1, 0, 268);
                upper_mac_->process_SCH_F(burst_type, bkn1);
            }
        }
    } else if (burst_type == BurstType::NormalDownlinkBurstSplit) {
        upper_mac_->incrementTn();

        // bb contains AACH
        // ✅ done
        bb = vectorExtract(frame, 230, 14);
        bb = vectorAppend(frame, bb, 266, 16);
        bb = descramble(bb, 30, upper_mac_->scrambling_code());
        bb = reed_muller_3014_decode(bb);
        upper_mac_->process_AACH(burst_type, bb);

        // STCH + TCH
        // STCH + STCH
        bkn1 = vectorExtract(frame, 14, 216);
        bkn1 = descramble(bkn1, 216, upper_mac_->scrambling_code());
        bkn2 = vectorExtract(frame, 282, 216);
        bkn2 = descramble(bkn2, 216, upper_mac_->scrambling_code());

        if (upper_mac_->downlink_usage() == DownlinkUsage::Traffic && upper_mac_->time_slot() <= 17) {
            bkn1 = deinterleave(bkn1, 216, 101);
            bkn1 = depuncture23(bkn1, 216);
            bkn1 = viter_bi_decode_1614(bkn1);
            if (check_crc_16_ccitt(bkn1, 140)) {
                bkn1 = vectorExtract(bkn1, 0, 124);
                upper_mac_->process_STCH(burst_type, bkn1);
            }

            if (upper_mac_->second_slot_stolen()) {
                bkn2 = deinterleave(bkn2, 216, 101);
                bkn2 = depuncture23(bkn2, 216);
                bkn2 = viter_bi_decode_1614(bkn2);
                if (check_crc_16_ccitt(bkn2, 140)) {
                    bkn2 = vectorExtract(bkn2, 0, 124);
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

            bkn1 = deinterleave(bkn1, 216, 101);
            bkn1 = depuncture23(bkn1, 216);
            bkn1 = viter_bi_decode_1614(bkn1);
            if (check_crc_16_ccitt(bkn1, 140)) {
                bkn1 = vectorExtract(bkn1, 0, 124);
                upper_mac_->process_SCH_HD(burst_type, bkn1);
            }
            // control channel
            bkn2 = deinterleave(bkn2, 216, 101);
            bkn2 = depuncture23(bkn2, 216);
            bkn2 = viter_bi_decode_1614(bkn2);
            if (check_crc_16_ccitt(bkn2, 140)) {
                bkn2 = vectorExtract(bkn2, 0, 124);
                upper_mac_->process_SCH_HD(burst_type, bkn2);
            }
        }
    } else if (burst_type == BurstType::ControlUplinkBurst) {
        cb = vectorExtract(frame, 4, 84);
        cb = vectorAppend(frame, cb, 118, 84);
        cb = descramble(cb, 168, upper_mac_->scrambling_code());
        cb = deinterleave(cb, 168, 13);
        cb = depuncture23(cb, 168);
        cb = viter_bi_decode_1614(cb);
        if (check_crc_16_ccitt(cb, 108)) {
            cb = vectorExtract(cb, 0, 92);
            upper_mac_->process_SCH_HU(burst_type, cb);
            return true;
        } else {
            fmt::print("CUB Burst crc failed\n");
            return false;
        }
    } else if (burst_type == BurstType::NormalUplinkBurst) {
        bkn1 = vectorExtract(frame, 4, 216);
        bkn1 = vectorAppend(frame, bkn1, 242, 216);
        bkn1 = descramble(bkn1, 432, upper_mac_->scrambling_code());

        // XXX: assume to be control channel
        bkn1 = deinterleave(bkn1, 432, 103);
        bkn1 = depuncture23(bkn1, 432);
        bkn1 = viter_bi_decode_1614(bkn1);
        if (check_crc_16_ccitt(bkn1, 284)) {
            bkn1 = vectorExtract(bkn1, 0, 268);
            fmt::print("NUB Burst crc good\n");
            upper_mac_->process_SCH_F(burst_type, bkn1);
        } else {
            //						fmt::print("NUB Burst crc failed\n");
        }
    } else if (burst_type == BurstType::NormalUplinkBurstSplit) {
        // TODO: finish NormalUplinkBurstSplit implementation
        bkn1 = vectorExtract(frame, 4, 216);
        bkn1 = descramble(bkn1, 216, upper_mac_->scrambling_code());
        bkn2 = vectorExtract(frame, 242, 216);
        bkn2 = descramble(bkn2, 216, upper_mac_->scrambling_code());

        bkn1 = deinterleave(bkn1, 216, 101);
        bkn1 = depuncture23(bkn1, 216);
        bkn1 = viter_bi_decode_1614(bkn1);
        if (check_crc_16_ccitt(bkn1, 140)) {
            bkn1 = vectorExtract(bkn1, 0, 124);
            fmt::print("NUB_S 1 Burst crc good\n");
        }

        bkn2 = deinterleave(bkn2, 216, 101);
        bkn2 = depuncture23(bkn2, 216);
        bkn2 = viter_bi_decode_1614(bkn2);
        if (check_crc_16_ccitt(bkn2, 140)) {
            bkn2 = vectorExtract(bkn2, 0, 124);
            fmt::print("NUB_S 2 Burst crc good\n");
        }
    } else {
        throw std::runtime_error("LowerMac does not implement the burst type supplied");
    }

    return true;
}
