#ifndef DECODER_HPP
#define DECODER_HPP

#include <optional>
#include <string>

class Decoder {
public:
  Decoder(unsigned rxPort, unsigned txPort, bool keepFillBits, bool packed,
          std::optional<std::string> inFile,
          std::optional<std::string> outFile);
  ~Decoder();

  void main_loop();

private:
  bool _keepFillBits;
  bool _packed;

  int _inputFd;

  int _outputSocketFd;
  std::optional<int> _outputFileFd;

  const int _rxBufSize = 4096;

  void rxSymbol(uint8_t symbol);
};

#endif
