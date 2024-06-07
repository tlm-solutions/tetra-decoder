#include "burst_type.hpp"
#include "l2/access_assignment_channel.hpp"
#include "l2/broadcast_synchronization_channel.hpp"
#include "l2/lower_mac_coding.hpp"
#include "utils/bit_vector.hpp"
#include <array>
#include <cstdint>
#include <cstring>
#include <l2/lower_mac.hpp>
#include <utility>

#include <fmt/color.h>
#include <fmt/core.h>

LowerMac::LowerMac(std::shared_ptr<Reporter> reporter, std::shared_ptr<PrometheusExporter>& prometheus_exporter,
                   std::optional<uint32_t> scrambling_code)
    : reporter_(std::move(reporter)) {
    // For decoupled uplink processing we need to inject a scrambling code. Inject it into the correct place that would
    // normally be filled by a Synchronization Burst
    if (scrambling_code.has_value()) {
        sync_ = BroadcastSynchronizationChannel();
        sync_->scrambling_code = *scrambling_code;
    }

    upper_mac_ = std::make_shared<UpperMac>(reporter_, /*is_downlink=*/!scrambling_code.has_value());

    if (prometheus_exporter) {
        metrics_ = std::make_unique<LowerMacPrometheusCounters>(prometheus_exporter);
    }
}

auto LowerMac::processChannels(const std::vector<uint8_t>& frame, BurstType burst_type,
                               const BroadcastSynchronizationChannel& bsc) -> std::vector<std::function<void()>> {
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
        std::array<bool, 30> bb_input{};
        std::array<bool, 30> bb_desc{};
        std::array<bool, 14> bb_rm{};
        for (auto i = 0; i < 30; i++) {
            bb_input[i] = frame[252 + i];
        }
        LowerMacCoding::descramble(bb_input, bb_desc, bsc.scrambling_code);
        LowerMacCoding::reed_muller_3014_decode(bb_desc, bb_rm);
        auto _aach =
            AccessAssignmentChannel(burst_type, bsc.time, BitVector(std::vector(bb_rm.cbegin(), bb_rm.cend())));

        // bkn2 block
        // ✅ done
        // any off SCH/HD, BNCH, STCH
        // see ETSI EN 300 392-2 V3.8.1 (2016-08) Figure 8.6: Error control
        // structure for π4DQPSK logical channels (part 2)
        std::array<bool, 216> bkn2_input{};
        std::array<bool, 216> bkn2_desc{};
        std::array<bool, 216> bkn2_dei{};
        for (auto i = 0; i < 216; i++) {
            bkn2_input[i] = frame[282 + i];
        }
        LowerMacCoding::descramble(bkn2_input, bkn2_desc, bsc.scrambling_code);
        LowerMacCoding::deinterleave(bkn2_desc, bkn2_dei, 101);
        auto bkn2_bits =
            LowerMacCoding::viter_bi_decode_1614(viter_bi_codec_1614_, LowerMacCoding::depuncture23(bkn2_dei));
        if (LowerMacCoding::check_crc_16_ccitt<140>(bkn2_bits)) {
            // SCH/HD or BNCH mapped
            functions.emplace_back([this, burst_type, bkn2_bits] {
                auto bkn2_bv = BitVector(std::vector(bkn2_bits.cbegin(), bkn2_bits.cbegin() + 124));
                upper_mac_->process_SCH_HD(burst_type, bkn2_bv);
            });
        }
    } else if (burst_type == BurstType::NormalDownlinkBurst) {
        // bb contains AACH
        // ✅
        std::array<bool, 30> bb_input{};
        std::array<bool, 30> bb_desc{};
        std::array<bool, 14> bb_rm{};
        for (auto i = 0; i < 30; i++) {
            auto offset = i > 14 ? 266 : 230;
            bb_input[i] = frame[offset + i];
        }
        LowerMacCoding::descramble(bb_input, bb_desc, bsc.scrambling_code);
        LowerMacCoding::reed_muller_3014_decode(bb_desc, bb_rm);
        auto aach = AccessAssignmentChannel(burst_type, bsc.time, BitVector(std::vector(bb_rm.cbegin(), bb_rm.cend())));

        // TCH or SCH/F
        std::array<bool, 432> bkn1_input{};
        std::array<bool, 432> bkn1_desc{};
        std::array<bool, 432> bkn1_dei{};
        for (auto i = 0; i < 432; i++) {
            auto offset = i > 216 ? 282 : 14;
            bkn1_input[i] = frame[offset + i];
        }
        LowerMacCoding::descramble(bkn1_input, bkn1_desc, bsc.scrambling_code);
        LowerMacCoding::deinterleave(bkn1_desc, bkn1_dei, 103);
        auto bkn1_bits =
            LowerMacCoding::viter_bi_decode_1614(viter_bi_codec_1614_, LowerMacCoding::depuncture23(bkn1_dei));

        if (aach.downlink_usage == DownlinkUsage::Traffic) {
            // TODO: handle TCH
            functions.emplace_back([aach]() {
                std::cout << "AACH indicated traffic with usagemarker: "
                          << std::to_string(*aach.downlink_traffic_usage_marker) << std::endl;
            });
        } else {
            // control channel
            // ✅done
            if (LowerMacCoding::check_crc_16_ccitt<284>(bkn1_bits)) {
                functions.emplace_back([this, burst_type, bkn1_bits] {
                    auto bkn1_bv = BitVector(std::vector(bkn1_bits.cbegin(), bkn1_bits.cbegin() + 268));
                    upper_mac_->process_SCH_F(burst_type, bkn1_bv);
                });
            }
        }
    } else if (burst_type == BurstType::NormalDownlinkBurstSplit) {
        // bb contains AACH
        // ✅ done
        std::array<bool, 30> bb_input{};
        std::array<bool, 30> bb_desc{};
        std::array<bool, 14> bb_rm{};
        for (auto i = 0; i < 30; i++) {
            auto offset = i > 14 ? 266 : 230;
            bb_input[i] = frame[offset + i];
        }
        LowerMacCoding::descramble(bb_input, bb_desc, bsc.scrambling_code);
        LowerMacCoding::reed_muller_3014_decode(bb_desc, bb_rm);
        auto aach = AccessAssignmentChannel(burst_type, bsc.time, BitVector(std::vector(bb_rm.cbegin(), bb_rm.cend())));

        std::array<bool, 216> bkn1_input{};
        std::array<bool, 216> bkn1_desc{};
        std::array<bool, 216> bkn1_dei{};
        for (auto i = 0; i < 216; i++) {
            bkn1_input[i] = frame[14 + i];
        }
        LowerMacCoding::descramble(bkn1_input, bkn1_desc, bsc.scrambling_code);
        LowerMacCoding::deinterleave(bkn1_desc, bkn1_dei, 101);
        auto bkn1_bits =
            LowerMacCoding::viter_bi_decode_1614(viter_bi_codec_1614_, LowerMacCoding::depuncture23(bkn1_dei));

        std::array<bool, 216> bkn2_input{};
        std::array<bool, 216> bkn2_desc{};
        std::array<bool, 216> bkn2_dei{};
        for (auto i = 0; i < 216; i++) {
            bkn2_input[i] = frame[282 + i];
        }
        LowerMacCoding::descramble(bkn2_input, bkn2_desc, bsc.scrambling_code);
        LowerMacCoding::deinterleave(bkn2_desc, bkn2_dei, 101);
        auto bkn2_bits =
            LowerMacCoding::viter_bi_decode_1614(viter_bi_codec_1614_, LowerMacCoding::depuncture23(bkn2_dei));

        if (LowerMacCoding::check_crc_16_ccitt<140>(bkn1_bits)) {
            if (aach.downlink_usage == DownlinkUsage::Traffic) {
                // STCH + TCH
                // STCH + STCH
                functions.emplace_back([this, burst_type, bkn1_bits] {
                    auto bkn1_bv = BitVector(std::vector(bkn1_bits.cbegin(), bkn1_bits.cbegin() + 124));
                    upper_mac_->process_STCH(burst_type, bkn1_bv);
                });
            } else {
                // SCH/HD + SCH/HD
                // SCH/HD + BNCH
                // ✅done
                functions.emplace_back([this, burst_type, bkn1_bits] {
                    auto bkn1_bv = BitVector(std::vector(bkn1_bits.cbegin(), bkn1_bits.cbegin() + 124));
                    upper_mac_->process_SCH_HD(burst_type, bkn1_bv);
                });
            }
        }

        auto bkn2_crc = LowerMacCoding::check_crc_16_ccitt<140>(bkn2_bits);
        if (aach.downlink_usage == DownlinkUsage::Traffic) {
            functions.emplace_back([this, burst_type, aach, bkn2_bits, bkn2_crc]() {
                if (upper_mac_->second_slot_stolen()) {
                    if (bkn2_crc) {
                        auto bkn2_bv = BitVector(std::vector(bkn2_bits.cbegin(), bkn2_bits.cbegin() + 124));
                        upper_mac_->process_STCH(burst_type, bkn2_bv);
                    }
                } else {
                    // TODO: handle this TCH
                    std::cout << "AACH indicated traffic with usagemarker: "
                              << std::to_string(*aach.downlink_traffic_usage_marker) << std::endl;
                }
            });
        } else {
            // control channel
            if (bkn2_crc) {
                functions.emplace_back([this, burst_type, bkn2_bits] {
                    auto bkn2_bv = BitVector(std::vector(bkn2_bits.cbegin(), bkn2_bits.cbegin() + 124));
                    upper_mac_->process_SCH_HD(burst_type, bkn2_bv);
                });
            }
        }
    } else if (burst_type == BurstType::ControlUplinkBurst) {
        std::array<bool, 168> cb_input{};
        std::array<bool, 168> cb_desc{};
        std::array<bool, 168> cb_dei{};
        for (auto i = 0; i < 168; i++) {
            auto offset = i > 84 ? 118 : 4;
            cb_input[i] = frame[offset + i];
        }
        LowerMacCoding::descramble(cb_input, cb_desc, bsc.scrambling_code);
        LowerMacCoding::deinterleave(cb_desc, cb_dei, 13);
        auto cb_bits = LowerMacCoding::viter_bi_decode_1614(viter_bi_codec_1614_, LowerMacCoding::depuncture23(cb_dei));

        if (LowerMacCoding::check_crc_16_ccitt<108>(cb_bits)) {
            functions.emplace_back([this, burst_type, cb_bits] {
                auto bkn1_bv = BitVector(std::vector(cb_bits.cbegin(), cb_bits.cbegin() + 92));
                upper_mac_->process_SCH_HU(burst_type, bkn1_bv);
            });
        }
    } else if (burst_type == BurstType::NormalUplinkBurst) {
        std::array<bool, 432> bkn1_input{};
        std::array<bool, 432> bkn1_desc{};
        std::array<bool, 432> bkn1_dei{};
        for (auto i = 0; i < 432; i++) {
            auto offset = i > 216 ? 242 : 4;
            bkn1_input[i] = frame[offset + i];
        }
        LowerMacCoding::descramble(bkn1_input, bkn1_desc, bsc.scrambling_code);
        // TODO: this can either be a SCH_H or a TCH, depending on the uplink usage marker, but the uplink
        // and downlink processing are seperated. We assume a SCH_H here.
        LowerMacCoding::deinterleave(bkn1_desc, bkn1_dei, 103);
        auto bkn1_bits =
            LowerMacCoding::viter_bi_decode_1614(viter_bi_codec_1614_, LowerMacCoding::depuncture23(bkn1_dei));

        if (LowerMacCoding::check_crc_16_ccitt<284>(bkn1_bits)) {
            // fmt::print("NUB Burst crc good\n");
            functions.emplace_back([this, burst_type, bkn1_bits] {
                auto bkn1_bv = BitVector(std::vector(bkn1_bits.cbegin(), bkn1_bits.cbegin() + 268));
                upper_mac_->process_SCH_HU(burst_type, bkn1_bv);
            });
        } else {
            // Either we have a faulty burst or a TCH here.
        }
    } else if (burst_type == BurstType::NormalUplinkBurstSplit) {
        std::array<bool, 216> bkn1_input{};
        std::array<bool, 216> bkn1_desc{};
        std::array<bool, 216> bkn1_dei{};
        for (auto i = 0; i < 216; i++) {
            bkn1_input[i] = frame[4 + i];
        }
        LowerMacCoding::descramble(bkn1_input, bkn1_desc, bsc.scrambling_code);
        LowerMacCoding::deinterleave(bkn1_desc, bkn1_dei, 101);
        auto bkn1_bits =
            LowerMacCoding::viter_bi_decode_1614(viter_bi_codec_1614_, LowerMacCoding::depuncture23(bkn1_dei));

        std::array<bool, 216> bkn2_input{};
        std::array<bool, 216> bkn2_desc{};
        std::array<bool, 216> bkn2_dei{};
        for (auto i = 0; i < 216; i++) {
            bkn2_input[i] = frame[242 + i];
        }
        LowerMacCoding::descramble(bkn2_input, bkn2_desc, bsc.scrambling_code);
        LowerMacCoding::deinterleave(bkn2_desc, bkn2_dei, 101);
        auto bkn2_bits =
            LowerMacCoding::viter_bi_decode_1614(viter_bi_codec_1614_, LowerMacCoding::depuncture23(bkn2_dei));

        // STCH + TCH
        // STCH + STCH
        if (LowerMacCoding::check_crc_16_ccitt<140>(bkn1_bits)) {
            // fmt::print("NUB_S 1 Burst crc good\n");
            functions.emplace_back([this, burst_type, bkn1_bits] {
                auto bkn1_bv = BitVector(std::vector(bkn1_bits.cbegin(), bkn1_bits.cbegin() + 124));
                upper_mac_->process_STCH(burst_type, bkn1_bv);
            });
        }
        if (LowerMacCoding::check_crc_16_ccitt<140>(bkn2_bits)) {
            functions.emplace_back([this, burst_type, bkn2_bits]() {
                if (upper_mac_->second_slot_stolen()) {
                    // fmt::print("NUB_S 2 Burst crc good\n");
                    auto bkn2_bv = BitVector(std::vector(bkn2_bits.cbegin(), bkn2_bits.cbegin() + 124));
                    upper_mac_->process_STCH(burst_type, bkn2_bv);
                }
            });
        } else {
            // Either we have a faulty burst or a TCH here.
        }
    } else {
        throw std::runtime_error("LowerMac does not implement the burst type supplied");
    }

    return functions;
}

auto LowerMac::process(const std::vector<uint8_t>& frame, BurstType burst_type) -> std::vector<std::function<void()>> {
    // Set to true if there was some decoding error in the lower MAC
    bool decode_error = false;

    fmt::print("[Physical Channel] Decoding: {}\n", burst_type);

    // Once we received the Synchronization on the downlink, increment the time counter for every received burst.
    // We do not have any time handling for uplink processing.
    if (sync_ && is_downlink_burst(burst_type)) {
        sync_->time.increment();
    }

    if (burst_type == BurstType::SynchronizationBurst) {
        std::optional<BroadcastSynchronizationChannel> current_sync;

        // sb contains BSCH
        // ✅ done
        std::array<bool, 120> sb_input{};
        std::array<bool, 120> sb_desc{};
        std::array<bool, 120> sb_dei{};
        for (auto i = 0; i < 30; i++) {
            sb_input[i] = frame[94 + i];
        }
        LowerMacCoding::descramble(sb_input, sb_desc, 0x0003);
        LowerMacCoding::deinterleave(sb_desc, sb_dei, 11);
        auto sb_bits = LowerMacCoding::viter_bi_decode_1614(viter_bi_codec_1614_, LowerMacCoding::depuncture23(sb_dei));

        if (LowerMacCoding::check_crc_16_ccitt<76>(sb_bits)) {
            current_sync = BroadcastSynchronizationChannel(
                burst_type, BitVector(std::vector(sb_bits.cbegin(), sb_bits.cbegin() + 60)));
        } else {
            decode_error |= true;
        }

        // Update the mismatching received number of bursts metrics
        if (current_sync && sync_ && metrics_) {
            metrics_->increment(/*current_timestamp=*/current_sync->time, /*expected_timestamp=*/sync_->time);
        }
        sync_ = current_sync;
    }

    std::vector<std::function<void()>> callbacks{};
    // We got a sync, continue with further processing of channels
    if (sync_) {
        callbacks = processChannels(frame, burst_type, *sync_);

        // We assume to encountered an error decoding in the lower MAC if we do not get the correct number of callbacks
        // back. For normal channels this is one. For split channels this is two.
        std::size_t correct_number_of_callbacks;
        switch (burst_type) {
        case BurstType::ControlUplinkBurst:
        case BurstType::NormalUplinkBurst:
        case BurstType::NormalDownlinkBurst:
        case BurstType::SynchronizationBurst:
            correct_number_of_callbacks = 1;
            break;
        case BurstType::NormalDownlinkBurstSplit:
        case BurstType::NormalUplinkBurstSplit:
            correct_number_of_callbacks = 2;
            break;
        }

        decode_error |= callbacks.size() != correct_number_of_callbacks;
    }

    // Update the received burst type metrics
    if (metrics_) {
        metrics_->increment(burst_type, decode_error);
    }

    return callbacks;
}
