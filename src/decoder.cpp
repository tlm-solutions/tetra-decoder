/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#include "l2/upper_mac.hpp"
#include <arpa/inet.h>
#include <cassert>
#include <complex>
#include <cstring>
#include <fcntl.h>
#include <memory>
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
                 std::optional<unsigned int> uplink_scrambling_code,
                 const std::shared_ptr<PrometheusExporter>& prometheus_exporter)
    : packed_(packed)
    , uplink_scrambling_code_(uplink_scrambling_code)
    , iq_or_bit_stream_(iq_or_bit_stream) {
    auto lower_mac_work_queue = std::make_shared<StreamingOrderedOutputThreadPoolExecutor<LowerMac::return_type>>(4);
    auto reporter = std::make_shared<Reporter>(send_port);
    auto is_uplink = uplink_scrambling_code_.has_value();
    auto lower_mac = std::make_shared<LowerMac>(prometheus_exporter, uplink_scrambling_code);
    bit_stream_decoder_ =
        std::make_shared<BitStreamDecoder>(lower_mac_work_queue, lower_mac, uplink_scrambling_code_.has_value());
    iq_stream_decoder_ =
        std::make_unique<IQStreamDecoder>(lower_mac_work_queue, lower_mac, bit_stream_decoder_, is_uplink);
    upper_mac_ = std::make_unique<UpperMac>(lower_mac_work_queue, prometheus_exporter, reporter,
                                            /*is_downlink=*/!is_uplink);

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
        stop = 1;
        return;
    } else if (bytes_read < 0) {
        throw std::runtime_error("Read error");
    } else if (bytes_read == 0) {
        stop = 1;
        return;
    }

    if (output_file_fd_.has_value()) {
        if (write(*output_file_fd_, rx_buffer, bytes_read) != bytes_read) {
            // unable to write to output TODO: possible log or fail hard
            stop = 1;
            return;
        }
    }

    if (iq_or_bit_stream_) {
        std::complex<float>* rx_buffer_complex = reinterpret_cast<std::complex<float>*>(rx_buffer);

        assert(("Size of rx_buffer is not a multiple of std::complex<float>",
                bytes_read % sizeof(*rx_buffer_complex) == 0));
        auto size = bytes_read / sizeof(*rx_buffer_complex);

        for (auto i = 0; i < size; i++) {
            iq_stream_decoder_->process_complex(rx_buffer_complex[i]);
        }
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
