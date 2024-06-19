/*
 * Copyright (C) 2022-2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#pragma once

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

class Reporter {
  public:
    explicit Reporter(unsigned send_port);
    ~Reporter();

    void emit_report(nlohmann::json& message);

  private:
    int output_socket_fd_ = 0;
    struct sockaddr_in destination_ {};
};
