project(tetra-decoder)

cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 17)

option(NIX_BUILD "Is CMake called by a nix build?" OFF)

add_executable(tetra-decoder
               src/main.cpp
               src/decoder.cpp
               src/bit_stream_decoder.cpp
               src/iq_stream_decoder.cpp
               src/prometheus.cpp
               src/borzoi/borzoi_converter.cpp
               src/borzoi/borzoi_sender.cpp
               src/l2/access_assignment_channel.cpp
               src/l2/broadcast_synchronization_channel.cpp        
               src/l2/logical_link_control_formatter.cpp
               src/l2/logical_link_control_packet.cpp
               src/l2/lower_mac.cpp
               src/l2/slot.cpp
               src/l2/timebase_counter.cpp
               src/l2/upper_mac.cpp
               src/l2/upper_mac_packet.cpp
               src/l2/upper_mac_packet_builder.cpp
               src/l2/upper_mac_packet_formatter.cpp
               src/l3/circuit_mode_control_entity_formatter.cpp
               src/l3/circuit_mode_control_entity_packet.cpp
               src/l3/mobile_link_entity_formatter.cpp
               src/l3/mobile_link_entity_packet.cpp
               src/l3/short_data_service_formatter.cpp
               src/l3/short_data_service_packet.cpp
               src/utils/address.cpp
               src/utils/bit_vector.cpp
               src/utils/viter_bi_codec.cpp)

add_executable(tetra-puncturing
               src/examples/tetra_puncturing.cpp)

add_executable(tetra-viterbi
               src/examples/viter_bi_codec.cpp
               src/examples/tetra_viterbi.cpp)

target_compile_options(tetra-decoder PUBLIC -std=c++17 -Wall -Wno-unused-variable -msse4.1 -O3 -fcolor-diagnostics)
target_compile_options(tetra-puncturing PUBLIC -std=c++17 -Wall -Wno-unused-variable)
target_compile_options(tetra-viterbi PUBLIC -std=c++17 -Wall -Wno-unused-variable)

include(lib/ViterbiDecoderCpp/viterbi-config.cmake)

include_directories(src)

find_package(cxxopts CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(prometheus-cpp CONFIG REQUIRED)
find_package(cpr REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include)

if (NOT NIX_BUILD)
	target_link_libraries(tetra-decoder cxxopts::cxxopts)
endif()

target_link_libraries(tetra-decoder ZLIB::ZLIB fmt::fmt nlohmann_json::nlohmann_json viterbi prometheus-cpp::pull cpr::cpr)
target_link_libraries(tetra-viterbi viterbi)

install(TARGETS tetra-decoder DESTINATION bin)
install(TARGETS tetra-puncturing DESTINATION bin)
install(TARGETS tetra-viterbi DESTINATION bin)
