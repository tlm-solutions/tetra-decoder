cmake_minimum_required(VERSION 3.22)

add_executable(tetra-impl
               src/main.cpp
               src/Decoder.cpp
               src/l2/LowerMac.cpp
               src/l2/Coding.cpp
               src/l2/UpperMac.cpp
               src/l3/Mle.cpp
               src/utils/BitVec.cpp
               src/utils/Viterbi.cpp)

target_compile_options(tetra-impl PUBLIC -std=c++17 -Wall -Wno-unused-variable)

include_directories(src)

find_package(cxxopts REQUIRED)
find_package(ZLIB REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include)

target_link_libraries(tetra-impl ZLIB::ZLIB)

install(TARGETS tetra-impl DESTINATION bin)
