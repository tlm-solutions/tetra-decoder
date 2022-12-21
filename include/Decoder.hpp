#ifndef DECODER_HPP
#define DECODER_HPP

#include <optional>
#include <string>
#include <vector>

/**
 * Tetra downlink decoder for PI/4-DQPSK modulation
 *
 * Following downlink burst types for Phase Modulation are supported: (See TSI
 * EN 300 392-2 V3.8.1 (2016-08) Table 9.2)
 *
 * normal continuous downlink burst (NDB)
 *
 * synchronization continuous downlink burst (SB)
 *
 * Follawing downlink burst types for Phase Modulation are not supported:
 * discontinuous downlink burst (NDB)
 * synchronization discontinuous downlink burst (SB)
 *
 * Uplink burst types are not supported:
 * control uplink burst (CB)
 * normal uplink burst (NUB)
 */
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

  const std::size_t _rxBufSize = 4096;
  const std::size_t FRAME_LEN = 510;

  std::vector<uint8_t> _frame;

  // 9.4.4.3.2 Normal training sequence
  const std::vector<uint8_t> NORMAL_TRAINING_SEQ_1 = {
      1, 1, 0, 1, 0, 0, 0, 0, 1, 1, 1,
      0, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0}; // n1..n22
  const std::vector<uint8_t> NORMAL_TRAINING_SEQ_2 = {
      0, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0,
      0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0}; // p1..p22
  const std::vector<uint8_t> NORMAL_TRAINING_SEQ_3_BEGIN = {
      0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1}; // q11..q22
  const std::vector<uint8_t> NORMAL_TRAINING_SEQ_3_END = {
      1, 0, 1, 1, 0, 1, 1, 1, 0, 0}; // q1..q10

  // 9.4.4.3.4 Synchronisation training sequence
  const std::vector<uint8_t> SYNC_TRAINING_SEQ = {
      1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1,
      0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1}; // y1..y38

  bool _isSynchronized = false;
  std::size_t _syncBitCounter = 0;

  /**
   * @brief Process a received symbol.
   *
   * This function is called by "physical layer" when a bit is ready
   * to be processed.
   *
   * Note that "frame" is actually called "burst" in Tetra doc
   *
   * @return true if frame (burst) found, false otherwise
   *
   */
  void rxSymbol(uint8_t symbol);

  /**
   * @brief Reset the synchronizer
   *
   * Burst was matched, we can reset the synchronizer to allow 50 missing frames
   * (expressed in burst units = 50 * 510 bits)
   *
   */
  void resetSynchronizer();

  /**
   * @brief Process frame to decide which type of burst it is then service lower
   * MAC
   *
   */
  void processFrame();

  /**
   * @brief Return pattern/data comparison errors count at position in data
   * vector
   *
   * @param data      Vector to look in from pattern
   * @param pattern   Pattern to search
   * @param position  Position in vector to start search
   *
   * @return Score based on similarity with pattern (differences count between
   * vector and pattern)
   *
   */
  std::size_t patternAtPositionScore(const std::vector<uint8_t> &data,
                                     const std::vector<uint8_t> &pattern,
                                     std::size_t position);
};

#endif
