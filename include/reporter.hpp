/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <nlohmann/json.hpp>

class Reporter {
  public:
    Reporter(unsigned send_port);
    ~Reporter();

    void emit_report(nlohmann::json& message);

  private:
    int output_socket_fd_ = 0;
    struct sockaddr_in destination {};
};
