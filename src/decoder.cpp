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

Decoder::Decoder(unsigned receive_port, unsigned send_port, bool packed, std::optional<std::string> input_file,
                 std::optional<std::string> output_file, bool iq_or_bit_stream,
                 std::optional<unsigned int> uplink_scrambling_code)
    : reporter_(std::make_shared<Reporter>(send_port))
    , packed_(packed)
    , uplink_scrambling_code_(uplink_scrambling_code)
    , iq_or_bit_stream_(iq_or_bit_stream) {

    lower_mac_ = std::make_shared<LowerMac>(reporter_);
    bit_stream_decoder_ = std::make_unique<BitStreamDecoder>(lower_mac_, uplink_scrambling_code_.has_value());

    if (uplink_scrambling_code_.has_value()) {
        // set scrambling_code for uplink
        lower_mac_->set_scrambling_code(uplink_scrambling_code_.value());

        if (iq_or_bit_stream_) {
            throw std::runtime_error("IQ Stream is not supported for uplink decoding");
        }
    } else {
        if (iq_or_bit_stream_) {
            throw std::runtime_error("IQ Stream is not supported for downlink decoding");
        }
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

    if (output_file.has_value()) {
        // output file descriptor for saving data to file
        output_file_fd_ = open(output_file->c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP);
        if (*output_file_fd_ < 0) {
            throw std::runtime_error("Couldn't open output file");
        }
    }
}

Decoder::~Decoder() {
    close(input_fd_);
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
        if (write(*output_file_fd_, rx_buffer, bytes_read) != bytes_read) {
            // unable to write to output TODO: possible log or fail hard
            return;
        }
    }

    if (iq_or_bit_stream_) {
        throw std::runtime_error("IQ Stream is not supported");
    } else {
        for (auto i = 0; i < bytes_read; i++) {
            if (packed_) {
                for (auto j = 0; j < 8; j++) {
                    bit_stream_decoder_->process_bit((rx_buffer[i] >> j) & 0x1);
                }
            } else {
                bit_stream_decoder_->process_bit(rx_buffer[i]);
            }
        }
    }
}
