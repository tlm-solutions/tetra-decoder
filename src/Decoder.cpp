#include <arpa/inet.h>
#include <cassert>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <BurstType.hpp>
#include <Decoder.hpp>

Decoder::Decoder(unsigned rxPort, unsigned txPort, bool keepFillBits,
                 bool packed, std::optional<std::string> inFile,
                 std::optional<std::string> outFile)
    : _keepFillBits(keepFillBits), _packed(packed),
      _lowerMac(std::make_shared<LowerMac>()) {
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

void Decoder::rxSymbol(uint8_t symbol) {
  assert(symbol <= 1);

  // insert symbol at buffer end
  _frame.push_back(symbol);

  // not enough data to process
  if (_frame.size() < FRAME_LEN) {
    return;
  }

  bool frameFound = false;
  // XXX: this will only find Normal Continous Downlink Burst and
  // Synchronization Continous Downlink Burst
  uint32_t scoreBegin =
      patternAtPositionScore(_frame, NORMAL_TRAINING_SEQ_3_BEGIN, 0);
  uint32_t scoreEnd =
      patternAtPositionScore(_frame, NORMAL_TRAINING_SEQ_3_END, 500);

  // frame (burst) is matched and can be processed
  if ((scoreBegin == 0) && (scoreEnd < 2)) {
    frameFound = true;
    // reset missing sync synchronizer
    resetSynchronizer();
  }

  bool clearedFlag = false;

  // the frame can be processed either by presence of
  // training sequence, either by synchronized and
  // still allowed missing frames
  if (frameFound || (_isSynchronized && ((_syncBitCounter % 510) == 0))) {
    processFrame();

    // frame has been processed, so clear it
    _frame.clear();

    // set flag to prevent erasing first bit in frame
    clearedFlag = true;
  }

  _syncBitCounter--;

  // synchronization is lost
  if (_syncBitCounter <= 0) {
    printf("* synchronization lost\n");
    _isSynchronized = false;
    _syncBitCounter = 0;
  }

  // remove first symbol from buffer to make space for next one
  if (!clearedFlag) {
    _frame.erase(_frame.begin());
  }
}

void Decoder::resetSynchronizer() {
  _isSynchronized = true;
  _syncBitCounter = FRAME_LEN * 50;
}

void Decoder::processFrame() {
  auto scoreSb = patternAtPositionScore(_frame, SYNC_TRAINING_SEQ, 214);
  auto scoreNdb = patternAtPositionScore(_frame, NORMAL_TRAINING_SEQ_1, 244);
  auto scoreNdbSplit =
      patternAtPositionScore(_frame, NORMAL_TRAINING_SEQ_2, 244);

  auto scoreMin = scoreSb;
  BurstType burstType = SynchronizationBurst;

  if (scoreNdb < scoreMin) {
    scoreMin = scoreNdb;
    burstType = NormalDownlinkBurst;
  }

  if (scoreNdbSplit < scoreMin) {
    scoreMin = scoreNdbSplit;
    burstType = NormalDownlinkBurst_Split;
  }

  if (scoreMin <= 5) {
    // valid burst found, send it to lower MAC
    std::cout << "Processing burstType: " << burstType << std::endl;
    _lowerMac->process(_frame, burstType);
  }
}

std::size_t Decoder::patternAtPositionScore(const std::vector<uint8_t> &data,
                                            const std::vector<uint8_t> &pattern,
                                            std::size_t position) {
  std::size_t errors = 0;

  for (auto i = 0; i < pattern.size(); i++) {
    errors += (pattern[i] ^ data[position + i]);
  }

  return errors;
}
