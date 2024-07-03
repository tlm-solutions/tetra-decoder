/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#include "bit_stream_decoder.hpp"
#include "burst_type.hpp"
#include "l2/lower_mac.hpp"
#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/format.h>

void BitStreamDecoder::process_bit(uint8_t symbol) noexcept {
    assert(symbol <= 1);

    // insert symbol at buffer end
    frame_.push_back(symbol);

    // not enough data to process
    if (frame_.size() < kFRAME_LEN) {
        return;
    }

    if (!is_uplink_) {
        bool frame_found = false;
        // XXX: this will only find Normal Continous Downlink Burst and
        // Synchronization Continous Downlink Burst
        uint32_t score_begin = pattern_at_position_score(frame_, kNORMAL_TRAINING_SEQ_3_BEGIN, 0);
        uint32_t score_end = pattern_at_position_score(frame_, kNORMAL_TRAINING_SEQ_3_END, 500);

        // frame (burst) is matched and can be processed
        if ((score_begin == 0) && (score_end < 2)) {
            frame_found = true;
            // reset missing sync synchronizer
            reset_synchronizer();
        }

        bool cleared_flag = false;

        // the frame can be processed either by presence of
        // training sequence, either by synchronized and
        // still allowed missing frames
        if (frame_found || (is_synchronized_ && ((sync_bit_counter_ % 510) == 0))) {
            process_downlink_frame();

            // frame has been processed, so clear it
            frame_.clear();

            // set flag to prevent erasing first bit in frame
            cleared_flag = true;
        }

        sync_bit_counter_--;

        // synchronization is lost
        if (sync_bit_counter_ <= 0) {
            printf("* synchronization lost\n");
            is_synchronized_ = false;
            sync_bit_counter_ = 0;
        }

        // remove first symbol from buffer to make space for next one
        if (!cleared_flag) {
            frame_.erase(frame_.begin());
        }
    } else {
        // check at the end
        auto score_ssn = pattern_at_position_score(frame_, kEXTENDED_TRAINING_SEQ, 88);

        auto score_nub = pattern_at_position_score(frame_, kNORMAL_TRAINING_SEQ_1, 220);
        auto score_nub_split = pattern_at_position_score(frame_, kNORMAL_TRAINING_SEQ_2, 220);

        auto minimum_score = score_ssn;
        auto burst_type = BurstType::ControlUplinkBurst;

        if (score_nub < minimum_score) {
            minimum_score = score_nub;
            burst_type = BurstType::NormalUplinkBurst;
        }

        if (score_nub_split < minimum_score) {
            minimum_score = score_nub_split;
            burst_type = BurstType::NormalUplinkBurstSplit;
        }

        if (score_ssn <= 4) {
            lower_mac_worker_queue_->queue_work(std::bind(&LowerMac::process, lower_mac_, frame_, burst_type));

            std::vector<uint8_t>(frame_.begin() + 200, frame_.end()).swap(frame_);
        } else if (minimum_score <= 2) {
            // valid burst found, send it to lower MAC
            lower_mac_worker_queue_->queue_work(std::bind(&LowerMac::process, lower_mac_, frame_, burst_type));

            frame_.erase(frame_.begin());
            // std::vector<uint8_t>(frame_.begin()+462, frame_.end()).swap(frame_);
        } else {
            frame_.erase(frame_.begin());
        }
    }
}

void BitStreamDecoder::reset_synchronizer() noexcept {
    is_synchronized_ = true;
    sync_bit_counter_ = kFRAME_LEN * 50;
}

void BitStreamDecoder::process_downlink_frame() noexcept {
    auto score_sb = pattern_at_position_score(frame_, kSYNC_TRAINING_SEQ, 214);
    auto score_ndb = pattern_at_position_score(frame_, kNORMAL_TRAINING_SEQ_1, 244);
    auto score_ndb_split = pattern_at_position_score(frame_, kNORMAL_TRAINING_SEQ_2, 244);

    auto minimum_score = score_sb;
    auto burst_type = BurstType::SynchronizationBurst;

    if (score_ndb < minimum_score) {
        minimum_score = score_ndb;
        burst_type = BurstType::NormalDownlinkBurst;
    }

    if (score_ndb_split < minimum_score) {
        minimum_score = score_ndb_split;
        burst_type = BurstType::NormalDownlinkBurstSplit;
    }

    if (minimum_score <= 5) {
        // valid burst found, send it to lower MAC
        lower_mac_worker_queue_->queue_work(std::bind(&LowerMac::process, lower_mac_, frame_, burst_type));
    }
}

auto BitStreamDecoder::pattern_at_position_score(const std::vector<uint8_t>& data, const std::vector<uint8_t>& pattern,
                                                 std::size_t position) noexcept -> std::size_t {
    std::size_t errors = 0;

    for (auto i = 0ul; i < pattern.size(); i++) {
        errors += (pattern[i] ^ data[position + i]);
    }

    return errors;
}
