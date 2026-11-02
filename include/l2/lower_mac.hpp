/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#pragma once

#include "burst_type.hpp"
#include "l2/access_assignment_channel.hpp"
#include "l2/broadcast_synchronization_channel.hpp"
#include "l2/lower_mac_coding.hpp"
#include "l2/lower_mac_metrics.hpp"
#include "l2/slot.hpp"
#include "prometheus.h"
#include "utils/viter_bi_codec.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

class LowerMac {
  public:
    using return_type = std::optional<Slots>;

    LowerMac() = delete;
    explicit LowerMac(const std::shared_ptr<PrometheusExporter>& prometheus_exporter,
                      std::optional<uint32_t> scrambling_code = std::nullopt);
    ~LowerMac() = default;

    /// Get the correct viterbi decoder either for hard decision for bit or soft decision for soft bits.
    /// \targ DataType with bool (bits) the hard decision decoder is returned, with int16_t we have soft bits and return
    /// the soft decision decoder
    template <typename DataType> [[nodiscard]] auto getViterbi() const -> const ViterbiCodec& {
        if constexpr (std::is_same_v<DataType, bool>) {
            return viter_bi_codec_1614_hard_decision_;
        }
        if constexpr (std::is_same_v<DataType, int16_t>) {
            return viter_bi_codec_1614_soft_decision_;
        }
        assert(false && "Compiling descramble with template paramater not bool or int16_t");
    }

    /// handles the decoding of the synchronization bursts and once synchronized passes the data to the decoding of the
    /// channels. keeps track of the current network time
    /// \targ DataType with bool is selected we have bits, with int16_t we have soft bits
    template <typename DataType>
    [[nodiscard]] auto process(std::vector<DataType> frame, BurstType burst_type) -> return_type {
        // Set to true if there was some decoding error in the lower MAC
        bool decode_error = false;

        // fmt::print("[Physical Channel] Decoding: {}\n", burst_type);

        // Once we received the Synchronization on the downlink, increment the time counter for every received
        // burst. We do not have any time handling for uplink processing.
        if (sync_ && is_downlink_burst(burst_type)) {
            sync_->time.increment();
            if (metrics_) {
                metrics_->set_time(sync_->time);
            }
        }

        if (burst_type == BurstType::SynchronizationBurst) {
            std::optional<BroadcastSynchronizationChannel> current_sync;

            // sb contains BSCH
            // ✅ done
            std::array<DataType, 120> sb_input{};
            for (auto i = 0; i < 120; i++) {
                sb_input[i] = frame[94 + i];
            };

            auto sb_bits = LowerMacCoding::viter_bi_decode_1614(
                getViterbi<DataType>(), LowerMacCoding::depuncture23(LowerMacCoding::deinterleave(
                                            LowerMacCoding::descramble(sb_input, 0x0003), 11)));

            if (LowerMacCoding::check_crc_16_ccitt<76>(sb_bits)) {
                current_sync = BroadcastSynchronizationChannel(
                    burst_type, BitVector(std::vector(sb_bits.cbegin(), sb_bits.cbegin() + 60)));
            } else {
                decode_error |= true;
            }

            // Update the mismatching received number of bursts metrics
            if (current_sync && sync_ && metrics_) {
                metrics_->set_time(/*current_timestamp=*/current_sync->time, /*expected_timestamp=*/sync_->time);
            }
            sync_ = current_sync;
        }

        std::optional<Slots> slots;

        // We got a sync, continue with further processing of channels
        if (sync_) {
            slots = processChannels(frame, burst_type, *sync_);

            // check if we have crc decode errors in the lower mac
            decode_error |= slots->has_crc_error();
        }

        // Update the received burst type metrics
        if (metrics_) {
            metrics_->increment(burst_type, decode_error);
        }

        return slots;
    }

  private:
    // does the signal processing and then returns the slots containing the correct logical channels and their
    // associated data to be passed to the upper mac and further processed in a sequential order.
    /// \targ DataType with bool is selected we have bits, with int16_t we have symbols
    template <typename DataType>
    [[nodiscard]] auto processChannels(const std::vector<DataType>& frame, BurstType burst_type,
                                       const BroadcastSynchronizationChannel& bsc) -> Slots {
        std::optional<Slots> slots;

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
            std::array<DataType, 30> bb_input{};
            for (auto i = 0; i < 30; i++) {
                bb_input[i] = frame[252 + i];
            }

            auto bb_rm = LowerMacCoding::reed_muller_3014_decode(
                LowerMacCoding::softbits_to_bits(LowerMacCoding::descramble(bb_input, bsc.scrambling_code)));
            auto _aach =
                AccessAssignmentChannel(burst_type, bsc.time, BitVector(std::vector(bb_rm.cbegin(), bb_rm.cend())));

            // bkn2 block
            // ✅ done
            // any off SCH/HD, BNCH, STCH
            // see ETSI EN 300 392-2 V3.8.1 (2016-08) Figure 8.6: Error control
            // structure for π4DQPSK logical channels (part 2)
            std::array<DataType, 216> bkn2_input{};
            for (auto i = 0; i < 216; i++) {
                bkn2_input[i] = frame[282 + i];
            }

            auto bkn2_bits = LowerMacCoding::viter_bi_decode_1614(
                getViterbi<DataType>(), LowerMacCoding::depuncture23(LowerMacCoding::deinterleave(
                                            LowerMacCoding::descramble(bkn2_input, bsc.scrambling_code), 101)));

            slots = Slots(burst_type, SlotType::kOneSubslot,
                          Slot(LogicalChannelDataAndCrc{
                              .channel = LogicalChannel::kSignallingChannelHalfDownlink,
                              .data = BitVector(std::vector(bkn2_bits.cbegin(), bkn2_bits.cbegin() + 124)),
                              .crc_ok = LowerMacCoding::check_crc_16_ccitt<140>(bkn2_bits),
                          }));
        } else if (burst_type == BurstType::NormalDownlinkBurst) {
            // bb contains AACH
            // ✅
            std::array<DataType, 30> bb_input{};
            for (auto i = 0; i < 30; i++) {
                auto offset = i > 14 ? 266 - 14 : 230;
                bb_input[i] = frame[offset + i];
            }

            auto bb_rm = LowerMacCoding::reed_muller_3014_decode(
                LowerMacCoding::softbits_to_bits(LowerMacCoding::descramble(bb_input, bsc.scrambling_code)));
            auto aach =
                AccessAssignmentChannel(burst_type, bsc.time, BitVector(std::vector(bb_rm.cbegin(), bb_rm.cend())));

            // TCH or SCH/F
            std::array<DataType, 432> bkn1_input{};
            for (auto i = 0; i < 432; i++) {
                auto offset = i > 216 ? 282 - 216 : 14;
                bkn1_input[i] = frame[offset + i];
            };

            auto bkn1_descrambled = LowerMacCoding::descramble(bkn1_input, bsc.scrambling_code);
            auto bkn1_descrambled_bits = LowerMacCoding::softbits_to_bits(bkn1_descrambled);

            auto bkn1_bits = LowerMacCoding::viter_bi_decode_1614(
                getViterbi<DataType>(),
                LowerMacCoding::depuncture23(LowerMacCoding::deinterleave(bkn1_descrambled, 103)));

            if (aach.downlink_usage == DownlinkUsage::Traffic) {
                // Full slot traffic channel defined type 4 bits (only descrambling)
                slots = Slots(
                    burst_type, SlotType::kFullSlot,
                    Slot(LogicalChannelDataAndCrc{
                        .channel = LogicalChannel::kTrafficChannel,
                        .data = BitVector(std::vector(bkn1_descrambled_bits.cbegin(), bkn1_descrambled_bits.cend())),
                        .crc_ok = true,
                    }));
            } else {
                // control channel
                // ✅done
                slots = Slots(burst_type, SlotType::kFullSlot,
                              Slot(LogicalChannelDataAndCrc{
                                  .channel = LogicalChannel::kSignallingChannelFull,
                                  .data = BitVector(std::vector(bkn1_bits.cbegin(), bkn1_bits.cbegin() + 268)),
                                  .crc_ok = LowerMacCoding::check_crc_16_ccitt<284>(bkn1_bits),
                              }));
            }
        } else if (burst_type == BurstType::NormalDownlinkBurstSplit) {
            // bb contains AACH
            // ✅ done
            std::array<DataType, 30> bb_input{};
            for (auto i = 0; i < 30; i++) {
                auto offset = i > 14 ? 266 - 14 : 230;
                bb_input[i] = frame[offset + i];
            }

            auto bb_rm = LowerMacCoding::reed_muller_3014_decode(
                LowerMacCoding::softbits_to_bits(LowerMacCoding::descramble(bb_input, bsc.scrambling_code)));
            auto aach =
                AccessAssignmentChannel(burst_type, bsc.time, BitVector(std::vector(bb_rm.cbegin(), bb_rm.cend())));

            std::array<DataType, 216> bkn1_input{};
            for (auto i = 0; i < 216; i++) {
                bkn1_input[i] = frame[14 + i];
            };

            auto bkn1_bits = LowerMacCoding::viter_bi_decode_1614(
                getViterbi<DataType>(), LowerMacCoding::depuncture23(LowerMacCoding::deinterleave(
                                            LowerMacCoding::descramble(bkn1_input, bsc.scrambling_code), 101)));

            std::array<DataType, 216> bkn2_input{};
            for (auto i = 0; i < 216; i++) {
                bkn2_input[i] = frame[282 + i];
            }

            auto bkn2_deinterleaved =
                LowerMacCoding::deinterleave(LowerMacCoding::descramble(bkn2_input, bsc.scrambling_code), 101);
            auto bkn2_deinterleaved_bits = LowerMacCoding::softbits_to_bits(bkn2_deinterleaved);
            auto bkn2_bits = LowerMacCoding::viter_bi_decode_1614(getViterbi<DataType>(),
                                                                  LowerMacCoding::depuncture23(bkn2_deinterleaved));

            // Half slot traffic channel defines type 3 bits (deinterleaved)
            if (aach.downlink_usage == DownlinkUsage::Traffic) {
                // STCH + TCH
                // STCH + STCH
                slots = Slots(burst_type, SlotType::kTwoSubslots,
                              Slot(LogicalChannelDataAndCrc{
                                  .channel = LogicalChannel::kStealingChannel,
                                  .data = BitVector(std::vector(bkn1_bits.cbegin(), bkn1_bits.cbegin() + 124)),
                                  .crc_ok = LowerMacCoding::check_crc_16_ccitt<140>(bkn1_bits),
                              }),
                              Slot({LogicalChannelDataAndCrc{
                                        .channel = LogicalChannel::kStealingChannel,
                                        .data = BitVector(std::vector(bkn2_bits.cbegin(), bkn2_bits.cbegin() + 124)),
                                        .crc_ok = LowerMacCoding::check_crc_16_ccitt<140>(bkn2_bits),
                                    },
                                    LogicalChannelDataAndCrc{
                                        .channel = LogicalChannel::kTrafficChannel,
                                        .data = BitVector(std::vector(bkn2_deinterleaved_bits.cbegin(),
                                                                      bkn2_deinterleaved_bits.cend())),
                                        .crc_ok = true,
                                    }}));
            } else {
                // SCH/HD + SCH/HD
                // SCH/HD + BNCH
                slots = Slots(burst_type, SlotType::kTwoSubslots,
                              Slot(LogicalChannelDataAndCrc{
                                  .channel = LogicalChannel::kSignallingChannelHalfDownlink,
                                  .data = BitVector(std::vector(bkn1_bits.cbegin(), bkn1_bits.cbegin() + 124)),
                                  .crc_ok = LowerMacCoding::check_crc_16_ccitt<140>(bkn1_bits),
                              }),
                              Slot(LogicalChannelDataAndCrc{
                                  .channel = LogicalChannel::kSignallingChannelHalfDownlink,
                                  .data = BitVector(std::vector(bkn2_bits.cbegin(), bkn2_bits.cbegin() + 124)),
                                  .crc_ok = LowerMacCoding::check_crc_16_ccitt<140>(bkn2_bits),
                              }));
            }
        } else if (burst_type == BurstType::ControlUplinkBurst) {
            std::array<DataType, 168> cb_input{};
            for (auto i = 0; i < 168; i++) {
                auto offset = i > 84 ? 118 - 84 : 4;
                cb_input[i] = frame[offset + i];
            };

            auto cb_bits = LowerMacCoding::viter_bi_decode_1614(
                getViterbi<DataType>(), LowerMacCoding::depuncture23(LowerMacCoding::deinterleave(
                                            LowerMacCoding::descramble(cb_input, bsc.scrambling_code), 13)));

            // SCH/HU
            slots = Slots(burst_type, SlotType::kOneSubslot,
                          Slot(LogicalChannelDataAndCrc{
                              .channel = LogicalChannel::kSignallingChannelHalfUplink,
                              .data = BitVector(std::vector(cb_bits.cbegin(), cb_bits.cbegin() + 92)),
                              .crc_ok = LowerMacCoding::check_crc_16_ccitt<108>(cb_bits),
                          }));
        } else if (burst_type == BurstType::NormalUplinkBurst) {
            std::array<DataType, 432> bkn1_input{};
            for (auto i = 0; i < 432; i++) {
                auto offset = i > 216 ? 242 - 216 : 4;
                bkn1_input[i] = frame[offset + i];
            }

            // TODO: this can either be a SCH_H or a TCH, depending on the uplink usage marker, but the uplink
            // and downlink processing are seperated. We assume a SCH_H here.
            auto bkn1_descrambled = LowerMacCoding::descramble(bkn1_input, bsc.scrambling_code);
            auto bkn1_descrambled_bits = LowerMacCoding::softbits_to_bits(bkn1_descrambled);

            auto bkn1_bits = LowerMacCoding::viter_bi_decode_1614(
                getViterbi<DataType>(),
                LowerMacCoding::depuncture23(LowerMacCoding::deinterleave(bkn1_descrambled, 103)));

            slots = Slots(
                burst_type, SlotType::kFullSlot,
                Slot({
                    LogicalChannelDataAndCrc{
                        .channel = LogicalChannel::kSignallingChannelFull,
                        .data = BitVector(std::vector(bkn1_bits.cbegin(), bkn1_bits.cbegin() + 268)),
                        .crc_ok = LowerMacCoding::check_crc_16_ccitt<284>(bkn1_bits),
                    },
                    LogicalChannelDataAndCrc{
                        .channel = LogicalChannel::kTrafficChannel,
                        .data = BitVector(std::vector(bkn1_descrambled_bits.cbegin(), bkn1_descrambled_bits.cend())),
                        .crc_ok = true,
                    },
                }));
        } else if (burst_type == BurstType::NormalUplinkBurstSplit) {
            std::array<DataType, 216> bkn1_input{};
            for (auto i = 0; i < 216; i++) {
                bkn1_input[i] = frame[4 + i];
            }

            auto bkn1_bits = LowerMacCoding::viter_bi_decode_1614(
                getViterbi<DataType>(), LowerMacCoding::depuncture23(LowerMacCoding::deinterleave(
                                            LowerMacCoding::descramble(bkn1_input, bsc.scrambling_code), 101)));

            std::array<DataType, 216> bkn2_input{};
            for (auto i = 0; i < 216; i++) {
                bkn2_input[i] = frame[242 + i];
            };

            auto bkn2_deinterleaved =
                LowerMacCoding::deinterleave(LowerMacCoding::descramble(bkn2_input, bsc.scrambling_code), 101);
            auto bkn2_deinterleaved_bits = LowerMacCoding::softbits_to_bits(bkn2_deinterleaved);
            auto bkn2_bits = LowerMacCoding::viter_bi_decode_1614(getViterbi<DataType>(),
                                                                  LowerMacCoding::depuncture23(bkn2_deinterleaved));

            // STCH + TCH
            // STCH + STCH
            slots = Slots(burst_type, SlotType::kTwoSubslots,
                          Slot(LogicalChannelDataAndCrc{
                              .channel = LogicalChannel::kStealingChannel,
                              .data = BitVector(std::vector(bkn1_bits.cbegin(), bkn1_bits.cbegin() + 124)),
                              .crc_ok = LowerMacCoding::check_crc_16_ccitt<140>(bkn1_bits),
                          }),
                          Slot({LogicalChannelDataAndCrc{
                                    .channel = LogicalChannel::kStealingChannel,
                                    .data = BitVector(std::vector(bkn2_bits.cbegin(), bkn2_bits.cbegin() + 124)),
                                    .crc_ok = LowerMacCoding::check_crc_16_ccitt<140>(bkn2_bits),
                                },
                                LogicalChannelDataAndCrc{
                                    .channel = LogicalChannel::kTrafficChannel,
                                    .data = BitVector(
                                        std::vector(bkn2_deinterleaved_bits.cbegin(), bkn2_deinterleaved_bits.cend())),
                                    .crc_ok = true,
                                }}));
        } else {
            throw std::runtime_error("LowerMac does not implement the burst type supplied");
        }

        return *slots;
    }

    const ViterbiCodecHardDecision viter_bi_codec_1614_hard_decision_;
    const ViterbiCodecSoftDecision viter_bi_codec_1614_soft_decision_;

    std::unique_ptr<LowerMacMetrics> metrics_;

    /// The last received synchronization burst.
    /// This include the current scrambling code. Set by Synchronization Burst on downlink or injected from the side for
    /// uplink processing, as we decouple it from the downlink for data/control packets.
    std::optional<BroadcastSynchronizationChannel> sync_;
};