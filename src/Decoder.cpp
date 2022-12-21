#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <Decoder.hpp>

Decoder::Decoder(unsigned rxPort, unsigned txPort, bool keepFillBits,
                 bool packed, std::optional<std::string> inFile,
                 std::optional<std::string> outFile)
    : _keepFillBits(keepFillBits), _packed(packed) {
  // read input file from file or from socket
  if (inFile.has_value()) {
    _inputFd = open(inFile->c_str(), O_RDONLY);

    if (_inputFd < 0) {
      throw std::runtime_error("Couldn't open input bits file");
    }
  } else {
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(rxPort);
    inet_aton("127.0.0.1", &addr.sin_addr);

    _inputFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    bind(_inputFd, (struct sockaddr *)&addr, sizeof(struct sockaddr));

    if (_inputFd < 0) {
      throw std::runtime_error("Couldn't create input socket");
    }
  }

  // output file descriptor for the udp socket
  // there goes our nice json
  struct sockaddr_in addr_output;
  std::memset(&addr_output, 0, sizeof(struct sockaddr_in));
  addr_output.sin_family = AF_INET;
  addr_output.sin_port = htons(txPort);
  inet_aton("127.0.0.1", &addr_output.sin_addr);

  _outputSocketFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  connect(_outputSocketFd, (struct sockaddr *)&addr_output,
          sizeof(struct sockaddr));

  if (_outputSocketFd < 0) {
    throw std::runtime_error("Couldn't create output socket");
  }

  if (outFile.has_value()) {
    // output file descriptor for saving data to file
    *_outputFileFd =
        open(outFile->c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP);
    if (*_outputFileFd < 0) {
      throw std::runtime_error("Couldn't open output file");
    }
  }
}

Decoder::~Decoder() {
  close(_inputFd);
  close(_outputSocketFd);
  if (_outputFileFd.has_value()) {
    close(*_outputFileFd);
  }
}

void Decoder::main_loop() {
  uint8_t rxBuf[_rxBufSize];

  auto bytesRead = read(_inputFd, rxBuf, sizeof(rxBuf));

  if (errno == EINTR) {
    return;
  } else if (bytesRead < 0) {
    throw std::runtime_error("Read error");
    return;
  } else if (bytesRead == 0) {
    return;
  }

  if (_outputFileFd.has_value()) {
    write(*_outputFileFd, rxBuf, bytesRead);
  }

  for (auto i = 0; i < bytesRead; i++) {
    if (_packed) {
      for (auto j = 0; j < 8; j++) {
        this->rxSymbol((rxBuf[i] >> j) & 0x1);
      }
    } else {
      this->rxSymbol(rxBuf[i]);
    }
  }
}

void Decoder::rxSymbol(uint8_t symbol) {}
