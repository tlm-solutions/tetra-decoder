/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#include "decoder.hpp"
#include "l2/upper_mac.hpp"
#include <arpa/inet.h>
#include <cassert>
#include <complex>
#include <cstring>
#include <fcntl.h>
#include <fmt/color.h>
#include <fmt/core.h>
#include <memory>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

Decoder::Decoder(unsigned receive_port, unsigned send_port, bool packed, std::optional<std::string> input_file,
                 std::optional<std::string> output_file, bool iq_or_bit_stream,
                 std::optional<unsigned int> uplink_scrambling_code,
                 const std::shared_ptr<PrometheusExporter>& prometheus_exporter)
    : lower_mac_work_queue_(std::make_shared<StreamingOrderedOutputThreadPoolExecutor<LowerMac::return_type>>(
          termination_flag_, upper_mac_termination_flag_, 4))
    , packed_(packed)
    , uplink_scrambling_code_(uplink_scrambling_code)
    , iq_or_bit_stream_(iq_or_bit_stream) {
    auto is_uplink = uplink_scrambling_code_.has_value();
    auto lower_mac = std::make_shared<LowerMac>(prometheus_exporter, uplink_scrambling_code);
    upper_mac_ = std::make_unique<UpperMac>(lower_mac_work_queue_, upper_mac_termination_flag_, prometheus_exporter);
    bit_stream_decoder_ =
        std::make_shared<BitStreamDecoder>(lower_mac_work_queue_, lower_mac, uplink_scrambling_code_.has_value());
    iq_stream_decoder_ =
        std::make_unique<IQStreamDecoder>(lower_mac_work_queue_, lower_mac, bit_stream_decoder_, is_uplink);

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
        if (input_fd_ < 0) {
            throw std::runtime_error("Couldn't create input socket");
        }

        if (bind(input_fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(struct sockaddr)) < 0) {
            throw std::runtime_error("ERROR cannot bind to input socket");
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
    /// Terminate the lower mac work queue
    termination_flag_ = true;
}

void Decoder::main_loop() {
    std::array<uint8_t, kRX_BUFFER_SIZE> rx_buffer{};

    auto bytes_read = read(input_fd_, rx_buffer.data(), sizeof(rx_buffer));

    if (errno == EINTR) {
        stop = true;
        return;
    }
    if (bytes_read < 0) {
        throw std::runtime_error("Read error.");
    }
    if (bytes_read == 0) {
        stop = true;
        return;
    }

    if (output_file_fd_.has_value()) {
        if (write(*output_file_fd_, rx_buffer.data(), bytes_read) != bytes_read) {
            throw std::runtime_error("Could not write to output file.");
            stop = true;
            return;
        }
    }

    if (iq_or_bit_stream_) {
        const auto* rx_buffer_complex = reinterpret_cast<std::complex<float>*>(rx_buffer.data());

        assert((bytes_read % sizeof(*rx_buffer_complex) == 0) &&
               "Size of rx_buffer is not a multiple of std::complex<float>");
        const auto size = bytes_read / sizeof(*rx_buffer_complex);

        for (auto i = 0; i < size; i++) {
            iq_stream_decoder_->process_complex(rx_buffer_complex[i]);
        }
    } else {
        for (auto i = 0; i < bytes_read; i++) {
            if (packed_) {
                for (auto j = 0; j < 8; j++) {
                    bit_stream_decoder_->process_bit((rx_buffer.at(i) >> j) & 0x1);
                }
            } else {
                bit_stream_decoder_->process_bit(rx_buffer.at(i));
            }
        }
    }
}
