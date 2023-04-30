/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#include <arpa/inet.h>
#include <cassert>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <burst_type.hpp>
#include <decoder.hpp>
#include <fmt/color.h>
#include <fmt/core.h>

Decoder::Decoder(unsigned receive_port, unsigned send_port, bool keep_fill_bits, bool packed,
                 std::optional<std::string> input_file, std::optional<std::string> output_file,
                 std::optional<unsigned int> uplink_scrambling_code)
    : lower_mac_(std::make_shared<LowerMac>())
    , keep_fill_bits_(keep_fill_bits)
    , packed_(packed)
    , uplink_scrambling_code_(uplink_scrambling_code) {
    // set scrambling_code for uplink
    if (uplink_scrambling_code_.has_value()) {
        lower_mac_->set_scrambling_code(uplink_scrambling_code_.value());
    }

    // read input file from file or from socket
    if (input_file.has_value()) {
        input_fd_ = open(input_file->c_str(), O_RDONLY);

        if (input_fd_ < 0) {
            throw std::runtime_error("Couldn't open input bits file");
        }
    } else {
        struct sockaddr_in addr {};
        std::memset(&addr, 0, sizeof(struct sockaddr_in));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(receive_port);
        inet_aton("127.0.0.1", &addr.sin_addr);

        input_fd_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (bind(input_fd_, (struct sockaddr*)&addr, sizeof(struct sockaddr)) < 0) {
            fmt::print(fg(fmt::color::crimson) | fmt::emphasis::bold, "ERROR cannot bind to input socket");
            // TODO: handle error
        }

        if (input_fd_ < 0) {
            throw std::runtime_error("Couldn't create input socket");
        }
    }

    // output file descriptor for the udp socket
    // there goes our nice json
    struct sockaddr_in addr_output {};
    std::memset(&addr_output, 0, sizeof(struct sockaddr_in));
    addr_output.sin_family = AF_INET;
    addr_output.sin_port = htons(send_port);
    inet_aton("127.0.0.1", &addr_output.sin_addr);

    output_socket_fd_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (connect(output_socket_fd_, (struct sockaddr*)&addr_output, sizeof(struct sockaddr)) > 0) {
        fmt::print(fg(fmt::color::crimson) | fmt::emphasis::bold, "[ERROR] cannot connect to output address ");
        // TODO: handle error
    }

    if (output_socket_fd_ < 0) {
        throw std::runtime_error("Couldn't create output socket");
    }

    if (output_file.has_value()) {
        // output file descriptor for saving data to file
        *output_file_fd_ = open(output_file->c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP);
        if (*output_file_fd_ < 0) {
            throw std::runtime_error("Couldn't open output file");
        }
    }
}

Decoder::~Decoder() {
    close(input_fd_);
    close(output_socket_fd_);
    if (output_file_fd_.has_value()) {
        close(*output_file_fd_);
    }
}

void Decoder::main_loop() {
    uint8_t rx_buffer[kRX_BUFFER_SIZE];

    auto bytes_read = read(input_fd_, rx_buffer, sizeof(rx_buffer));

    if (errno == EINTR) {
        return;
    } else if (bytes_read < 0) {
        throw std::runtime_error("Read error");
    } else if (bytes_read == 0) {
        return;
    }

    if (output_file_fd_.has_value()) {
        if (write(*output_file_fd_, rx_buffer, bytes_read) > 0) {
            // unable to write to output TODO: possible log or fail hard
            return;
        }
    }

    for (auto i = 0; i < bytes_read; i++) {
        if (packed_) {
            for (auto j = 0; j < 8; j++) {
                this->process_bit((rx_buffer[i] >> j) & 0x1);
            }
        } else {
            this->process_bit(rx_buffer[i]);
        }
    }
}

void Decoder::process_bit(uint8_t symbol) noexcept {
    assert(symbol <= 1);

    // insert symbol at buffer end
    frame_.push_back(symbol);

    // not enough data to process
    if (frame_.size() < kFRAME_LEN) {
        return;
    }

    if (!uplink_scrambling_code_.has_value()) {
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
        BurstType burstType = BurstType::ControlUplinkBurst;

        if (score_nub < minimum_score) {
            minimum_score = score_nub;
            burstType = BurstType::NormalUplinkBurst;
        }

        if (score_nub_split < minimum_score) {
            minimum_score = score_nub_split;
            burstType = BurstType::NormalUplinkBurst_Split;
        }

        if (score_ssn <= 4) {
            //					fmt::print("Processing burst type: {}\n", ControlUplinkBurst);
            if (lower_mac_->process(frame_, burstType))
                std::vector<uint8_t>(frame_.begin() + 200, frame_.end()).swap(frame_);
            else
                frame_.erase(frame_.begin());
        } else if (minimum_score <= 2) {
            // valid burst found, send it to lower MAC
            //				fmt::print("Processing burst type: {}\n", burstType);
            lower_mac_->process(frame_, burstType);

            frame_.erase(frame_.begin());
            // std::vector<uint8_t>(frame_.begin()+462, frame_.end()).swap(frame_);
        } else {
            frame_.erase(frame_.begin());
        }
    }
}

void Decoder::reset_synchronizer() noexcept {
    is_synchronized_ = true;
    sync_bit_counter_ = kFRAME_LEN * 50;
}

void Decoder::process_downlink_frame() noexcept {
    auto score_sb = pattern_at_position_score(frame_, kSYNC_TRAINING_SEQ, 214);
    auto score_ndb = pattern_at_position_score(frame_, kNORMAL_TRAINING_SEQ_1, 244);
    auto score_ndb_split = pattern_at_position_score(frame_, kNORMAL_TRAINING_SEQ_2, 244);

    auto minimum_score = score_sb;
    BurstType burstType = BurstType::SynchronizationBurst;

    if (score_ndb < minimum_score) {
        minimum_score = score_ndb;
        burstType = BurstType::NormalDownlinkBurst;
    }

    if (score_ndb_split < minimum_score) {
        minimum_score = score_ndb_split;
        burstType = BurstType::NormalDownlinkBurst_Split;
    }

    if (minimum_score <= 5) {
        // valid burst found, send it to lower MAC
        fmt::print("Processing burst type: {}\n", burstType);
        lower_mac_->process(frame_, burstType);
    }
}

auto Decoder::pattern_at_position_score(const std::vector<uint8_t>& data, const std::vector<uint8_t>& pattern,
                                        std::size_t position) noexcept -> std::size_t {
    std::size_t errors = 0;

    for (auto i = 0ul; i < pattern.size(); i++) {
        errors += (pattern[i] ^ data[position + i]);
    }

    return errors;
}
