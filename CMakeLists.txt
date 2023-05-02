project(tetra)

cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 17)


add_executable(tetra
               src/main.cpp
        src/decoder.cpp
        src/l2/address_type.cpp
        src/l2/lower_mac.cpp
        src/l2/lower_mac_coding.cpp
        src/l2/upper_mac.cpp
        src/l2/upper_mac_fragmentation.cpp
        src/l2/logical_link_control.cpp
        src/l3/mobile_link_entity.cpp
        src/l3/circuit_mode_control_entity.cpp
        src/utils/bit_vector.cpp
        src/utils/viter_bi_codec.cpp)

target_compile_options(tetra PUBLIC -std=c++17 -Wall -Wno-unused-variable)

include_directories(src)

find_package(cxxopts CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(fmt REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include)

target_link_libraries(tetra ZLIB::ZLIB fmt::fmt)

install(TARGETS tetra DESTINATION bin)
