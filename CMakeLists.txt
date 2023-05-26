project(tetra-decoder)

cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 17)


add_executable(tetra-decoder
               src/main.cpp
               src/reporter.cpp
        src/decoder.cpp
        src/l2/lower_mac.cpp
        src/l2/lower_mac_coding.cpp
        src/l2/upper_mac.cpp
        src/l2/upper_mac_fragmentation.cpp
        src/l2/logical_link_control.cpp
        src/l3/mobile_link_entity.cpp
        src/l3/mobile_management.cpp
        src/l3/circuit_mode_control_entity.cpp
        src/l3/short_data_service.cpp
        src/utils/address_type.cpp
        src/utils/bit_vector.cpp
        src/utils/viter_bi_codec.cpp)

target_compile_options(tetra-decoder PUBLIC -std=c++17 -Wall -Wno-unused-variable)

include_directories(src)

find_package(cxxopts CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(fmt REQUIRED)
find_package(nlohmann_json REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include)

target_link_libraries(tetra-decoder ZLIB::ZLIB fmt::fmt nlohmann_json::nlohmann_json)

install(TARGETS tetra-decoder DESTINATION bin)
