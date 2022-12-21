#ifndef DECODER_HPP
#define DECODER_HPP

#include <filesystem>
#include <optional>

class Decoder {
public:
  Decoder(unsigned rxPort, unsigned txPort, bool keepFillBits, bool packed,
          std::optional<std::filesystem::path> inFilePath,
          std::optional<std::filesystem::path> outFilePath);
  ~Decoder(){};

  void main_loop();

private:
  unsigned _rxPort;
  unsigned _txPort;
  bool _keepFillBits;
  bool _packed;
  std::optional<std::filesystem::path> _inFilePath;
  std::optional<std::filesystem::path> _outFilePath;
};

#endif
