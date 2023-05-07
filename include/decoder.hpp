/*
 * Copyright (C) 2022 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 *   Tassilo Tanneberger
 */

#ifndef TETRA_DECODER_DECODER_HPP
#define TETRA_DECODER_DECODER_HPP

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <l2/lower_mac.hpp>

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
    Decoder(unsigned int receive_port, unsigned int send_port, bool packed, std::optional<std::string> input_file,
            std::optional<std::string> output_file, std::optional<unsigned int> uplink_scrambling_code);
    ~Decoder();

    void main_loop();

  private:
    std::shared_ptr<LowerMac> lower_mac_ = nullptr;

    bool packed_ = false;
    bool is_synchronized_ = false;
    std::size_t sync_bit_counter_ = 0;

    // input and output file descriptor
    int input_fd_ = 0;
    int output_socket_fd_ = 0;

    // optional output file
    std::optional<int> output_file_fd_ = std::nullopt;

    // uplink ?
    std::optional<unsigned int> uplink_scrambling_code_;

    const std::size_t kRX_BUFFER_SIZE = 4096;
    const std::size_t kFRAME_LEN = 510;

    std::vector<uint8_t> frame_{};

    // 9.4.4.3.2 Normal training sequence
    const std::vector<uint8_t> kNORMAL_TRAINING_SEQ_1 = {1, 1, 0, 1, 0, 0, 0, 0, 1, 1, 1,
                                                         0, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0}; // n1..n22
    const std::vector<uint8_t> kNORMAL_TRAINING_SEQ_2 = {0, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0,
                                                         0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0};          // p1..p22
    const std::vector<uint8_t> kNORMAL_TRAINING_SEQ_3_BEGIN = {0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1}; // q11..q22
    const std::vector<uint8_t> kNORMAL_TRAINING_SEQ_3_END = {1, 0, 1, 1, 0, 1, 1, 1, 0, 0};         // q1..q10

    // 9.4.4.3.3 Extended training sequence
    const std::vector<uint8_t> kEXTENDED_TRAINING_SEQ = {1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 1, 1,
                                                         0, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 1}; // x1..x30

    // 9.4.4.3.4 Synchronisation training sequence
    const std::vector<uint8_t> kSYNC_TRAINING_SEQ = {1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0,
                                                     1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1}; // y1..y38

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
    void process_bit(uint8_t symbol) noexcept;

    /**
     * @brief Reset the synchronizer
     *
     * Burst was matched, we can reset the synchronizer to allow 50 missing frames
     * (expressed in burst units = 50 * 510 bits)
     *
     */
    void reset_synchronizer() noexcept;

    /**
     * @brief Process frame to decide which type of burst it is then service lower
     * MAC
     *
     */
    void process_downlink_frame() noexcept;

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
    static auto pattern_at_position_score(const std::vector<uint8_t>& data, const std::vector<uint8_t>& pattern,
                                          std::size_t position) noexcept -> std::size_t;
};

#endif // TETRA_DECODER_DECODER_HPP
