#include "burst_type.hpp"
#include "l2/broadcast_synchronization_channel.hpp"
#include <l2/lower_mac.hpp>

#include <fmt/color.h>
#include <fmt/core.h>

LowerMac::LowerMac(std::shared_ptr<Reporter> reporter, std::shared_ptr<PrometheusExporter>& prometheus_exporter)
    : reporter_(reporter) {
    viter_bi_codec_1614_ = std::make_shared<ViterbiCodec>();
    upper_mac_ = std::make_shared<UpperMac>(reporter_);

    if (prometheus_exporter) {
        metrics_ = std::make_unique<LowerMacPrometheusCounters>(prometheus_exporter);
    }
}

static auto vectorAppend(const std::vector<uint8_t>& vec, std::vector<uint8_t>& res, std::size_t pos,
                         std::size_t length) -> void {
    std::copy(vec.begin() + pos, vec.begin() + pos + length, std::back_inserter(res));
}

auto LowerMac::processChannels(const std::vector<uint8_t>& frame, BurstType burst_type,
                               const BroadcastSynchronizationChannel& bsc) -> std::vector<std::function<void()>> {
    std::vector<uint8_t> bkn1{};
    std::vector<uint8_t> bkn2{};
    std::vector<uint8_t> cb{};

    std::vector<std::function<void()>> functions{};

    // The BLCH may be mapped onto block 2 of the downlink slots, when a SCH/HD,
    // SCH-P8/HD or a BSCH is mapped onto block 1. The number of BLCH occurrences
    // on one carrier shall not exceed one per 4 multiframe periods.
    //
    // if any of the downlink functions need some data from the upper mac,
    // except for the scrambling code, preprocess everything and then do
    // the access to the data in the upper mac in the correct order!
    //
    // The scrambling code has a special handling, see the specific comments where
    // it is used.

    if (burst_type == BurstType::SynchronizationBurst) {
        // bb contains AACH
        // ✅ done
        uint8_t bb_desc[30];
        descramble(frame.data() + 252, bb_desc, 30, bsc.scrambling_code);
        std::vector<uint8_t> bb_rm(14);
        reed_muller_3014_decode(bb_desc, bb_rm.data());
        functions.push_back(std::bind(&UpperMac::process_AACH, upper_mac_, burst_type, bb_rm));

        // bkn2 block
        // ✅ done
        // any off SCH/HD, BNCH, STCH
        // see ETSI EN 300 392-2 V3.8.1 (2016-08) Figure 8.6: Error control
        // structure for π4DQPSK logical channels (part 2)
        uint8_t bkn2_desc[216];
        descramble(frame.data() + 282, bkn2_desc, 216, bsc.scrambling_code);
        uint8_t bkn2_dei[216];
        deinterleave(bkn2_desc, bkn2_dei, 216, 101);
        bkn2 = viter_bi_decode_1614(depuncture23(bkn2_dei, 216));
        // if the crc does not work, then it might be a BLCH
        if (check_crc_16_ccitt(bkn2.data(), 140)) {
            bkn2 = std::vector(bkn2.begin(), bkn2.begin() + 124);

            // SCH/HD or BNCH mapped
            functions.push_back(std::bind(&UpperMac::process_SCH_HD, upper_mac_, burst_type, bkn2));
        }
    } else if (burst_type == BurstType::NormalDownlinkBurst) {
        // bb contains AACH
        // ✅ done
        std::vector<uint8_t> bb{};
        vectorAppend(frame, bb, 230, 14);
        vectorAppend(frame, bb, 266, 16);
        uint8_t bb_des[30];
        descramble(bb.data(), bb_des, 30, bsc.scrambling_code);
        std::vector<uint8_t> bb_rm(14);
        reed_muller_3014_decode(bb_des, bb_rm.data());
        functions.push_back(std::bind(&UpperMac::process_AACH, upper_mac_, burst_type, bb_rm));

        // TCH or SCH/F
        vectorAppend(frame, bkn1, 14, 216);
        vectorAppend(frame, bkn1, 282, 216);
        uint8_t bkn1_desc[432];
        descramble(bkn1.data(), bkn1_desc, 432, bsc.scrambling_code);

        // TODO: fix this decoding order
        uint8_t bkn1_dei[432];
        deinterleave(bkn1_desc, bkn1_dei, 432, 103);
        bkn1 = viter_bi_decode_1614(depuncture23(bkn1_dei, 432));
        auto bkn1_crc = check_crc_16_ccitt(bkn1.data(), 284);
        bkn1 = std::vector(bkn1.begin(), bkn1.begin() + 268);

        functions.push_back([this, burst_type, bkn1, bkn1_crc]() {
            if (upper_mac_->downlink_usage() == DownlinkUsage::Traffic) {
                // TODO: handle TCH
                std::cout << "AACH indicated traffic with usagemarker: "
                          << std::to_string(upper_mac_->downlink_traffic_usage_marker()) << std::endl;
            } else {
                // control channel
                // ✅done
                if (bkn1_crc) {
                    upper_mac_->process_SCH_F(burst_type, bkn1);
                }
            }
        });
    } else if (burst_type == BurstType::NormalDownlinkBurstSplit) {
        // bb contains AACH
        // ✅ done
        std::vector<uint8_t> bb{};
        vectorAppend(frame, bb, 230, 14);
        vectorAppend(frame, bb, 266, 16);
        uint8_t bb_desc[30];
        descramble(bb.data(), bb_desc, 30, bsc.scrambling_code);
        std::vector<uint8_t> bb_rm(14);
        reed_muller_3014_decode(bb_desc, bb_rm.data());
        functions.push_back(std::bind(&UpperMac::process_AACH, upper_mac_, burst_type, bb_rm));

        // STCH + TCH
        // STCH + STCH
        uint8_t bkn1_desc[216];
        uint8_t bkn2_desc[216];
        descramble(frame.data() + 14, bkn1_desc, 216, bsc.scrambling_code);
        descramble(frame.data() + 282, bkn2_desc, 216, bsc.scrambling_code);

        // We precompute as much as possible. Only when all computations are done we schedule the task.
        uint8_t bkn1_dei[216];
        deinterleave(bkn1_desc, bkn1_dei, 216, 101);
        bkn1 = viter_bi_decode_1614(depuncture23(bkn1_dei, 216));
        uint8_t bkn2_dei[216];
        deinterleave(bkn2_desc, bkn2_dei, 216, 101);
        bkn2 = viter_bi_decode_1614(depuncture23(bkn2_dei, 216));

        if (check_crc_16_ccitt(bkn1.data(), 140)) {
            bkn1 = std::vector(bkn1.begin(), bkn1.begin() + 124);
            functions.push_back([this, burst_type, bkn1]() {
                if (upper_mac_->downlink_usage() == DownlinkUsage::Traffic) {
                    upper_mac_->process_STCH(burst_type, bkn1);
                } else {
                    // SCH/HD + SCH/HD
                    // SCH/HD + BNCH
                    // ✅done
                    upper_mac_->process_SCH_HD(burst_type, bkn1);
                }
            });
        }

        if (check_crc_16_ccitt(bkn2.data(), 140)) {
            bkn2 = std::vector(bkn2.begin(), bkn2.begin() + 124);
            functions.push_back([this, burst_type, bkn2]() {
                if (upper_mac_->downlink_usage() == DownlinkUsage::Traffic) {
                    if (upper_mac_->second_slot_stolen()) {
                        upper_mac_->process_STCH(burst_type, bkn2);
                    } else {
                        // TODO: handle this TCH
                        std::cout << "AACH indicated traffic with usagemarker: "
                                  << std::to_string(upper_mac_->downlink_traffic_usage_marker()) << std::endl;
                    }
                } else {
                    // control channel
                    upper_mac_->process_SCH_HD(burst_type, bkn2);
                }
            });
        }
    } else if (burst_type == BurstType::ControlUplinkBurst) {
        vectorAppend(frame, cb, 4, 84);
        vectorAppend(frame, cb, 118, 84);
        uint8_t cb_desc[168];
        descramble(cb.data(), cb_desc, 168, bsc.scrambling_code);

        // XXX: assume to be control channel
        uint8_t cb_dei[168];
        deinterleave(cb_desc, cb_dei, 168, 13);
        cb = viter_bi_decode_1614(depuncture23(cb_dei, 168));
        if (check_crc_16_ccitt(cb.data(), 108)) {
            cb = std::vector(cb.begin(), cb.begin() + 92);
            functions.push_back(std::bind(&UpperMac::process_SCH_HU, upper_mac_, burst_type, cb));
        }
    } else if (burst_type == BurstType::NormalUplinkBurst) {
        vectorAppend(frame, bkn1, 4, 216);
        vectorAppend(frame, bkn1, 242, 216);
        uint8_t bkn1_desc[432];
        descramble(bkn1.data(), bkn1_desc, 432, bsc.scrambling_code);

        // XXX: assume to be control channel
        uint8_t bkn1_dei[432];
        deinterleave(bkn1_desc, bkn1_dei, 432, 103);
        bkn1 = viter_bi_decode_1614(depuncture23(bkn1_dei, 432));
        if (check_crc_16_ccitt(bkn1.data(), 284)) {
            bkn1 = std::vector(bkn1.begin(), bkn1.begin() + 268);
            // fmt::print("NUB Burst crc good\n");
            functions.push_back(std::bind(&UpperMac::process_SCH_F, upper_mac_, burst_type, bkn1));
        }
    } else if (burst_type == BurstType::NormalUplinkBurstSplit) {
        // TODO: finish NormalUplinkBurstSplit implementation
        uint8_t bkn1_desc[216];
        uint8_t bkn2_desc[216];
        descramble(frame.data() + 4, bkn1_desc, 216, bsc.scrambling_code);
        descramble(frame.data() + 242, bkn2_desc, 216, bsc.scrambling_code);

        uint8_t bkn1_dei[216];
        deinterleave(bkn1_desc, bkn1_dei, 216, 101);
        bkn1 = viter_bi_decode_1614(depuncture23(bkn1_dei, 216));
        if (check_crc_16_ccitt(bkn1.data(), 140)) {
            bkn1 = std::vector(bkn1.begin(), bkn1.begin() + 124);
            // fmt::print("NUB_S 1 Burst crc good\n");
            functions.push_back(std::bind(&UpperMac::process_STCH, upper_mac_, burst_type, bkn1));
        }

        uint8_t bkn2_dei[216];
        deinterleave(bkn2_desc, bkn2_dei, 216, 101);
        bkn2 = viter_bi_decode_1614(depuncture23(bkn2_dei, 216));
        if (check_crc_16_ccitt(bkn2.data(), 140)) {
            bkn2 = std::vector(bkn2.begin(), bkn2.begin() + 124);
            functions.push_back([this, burst_type, bkn2]() {
                if (upper_mac_->second_slot_stolen()) {
                    // fmt::print("NUB_S 2 Burst crc good\n");
                    upper_mac_->process_STCH(burst_type, bkn2);
                }
            });
        }
    } else {
        throw std::runtime_error("LowerMac does not implement the burst type supplied");
    }

    return functions;
}

auto LowerMac::process(const std::vector<uint8_t>& frame, BurstType burst_type) -> std::vector<std::function<void()>> {
    std::vector<uint8_t> sb{};

    static std::optional<BroadcastSynchronizationChannel> last_sync{};

    // This value is set to true if there was a crc error.
    // In the NormalDownlinkBurst we cannot set this value since the AACH channel has not been decoded yet.
    // TODO: We need some code restructuring and split away the relevant parts from the lower mac from the upper
    // mac.
    // TODO: also the handling of the traffic channel is broken
    bool decode_error = false;

    fmt::print("[Physical Channel] Decoding: {}\n", burst_type);

    if (last_sync && is_downlink_burst(burst_type)) {
        last_sync->time.increment();
    }

    if (burst_type == BurstType::SynchronizationBurst) {
        std::optional<BroadcastSynchronizationChannel> current_sync;

        // sb contains BSCH
        // ✅ done
        uint8_t sb_desc[120];
        descramble(frame.data() + 94, sb_desc, 120, 0x0003);
        uint8_t sb_dei[120];
        deinterleave(sb_desc, sb_dei, 120, 11);
        sb = viter_bi_decode_1614(depuncture23(sb_dei, 120));
        if (check_crc_16_ccitt(sb.data(), 76)) {
            sb = std::vector(sb.begin(), sb.begin() + 60);
            current_sync = BroadcastSynchronizationChannel(burst_type, sb);
        } else {
            decode_error = true;
        }

        if (current_sync && last_sync && current_sync->time != last_sync->time) {
            // We did not receive the correct number of bursts!
            // TODO: Increment the dropped bursts counter
        }
        last_sync = current_sync;
    }

    std::vector<std::function<void()>> callbacks{};
    if (last_sync) {
        callbacks = processChannels(frame, burst_type, *last_sync);
        // We assume to encountered an error decoding in the lower MAC if we do not get any callbacks back.
        // TODO: check if this assumption holds true
        decode_error = callbacks.empty();
    }

    // Update the received burst type metrics
    if (metrics_) {
        metrics_->increment(burst_type, decode_error);
    }

    return callbacks;
}
