#include <Decoder.hpp>

Decoder::Decoder(unsigned rxPort, unsigned txPort, bool keepFillBits,
                 bool packed, std::optional<std::filesystem::path> inFilePath,
                 std::optional<std::filesystem::path> outFilePath)
    : _rxPort(rxPort), _txPort(txPort), _keepFillBits(keepFillBits),
      _packed(packed), _inFilePath(inFilePath), _outFilePath(outFilePath) {}

void Decoder::main_loop() {}
