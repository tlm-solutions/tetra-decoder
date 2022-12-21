cmake_minimum_required(VERSION 3.22)

add_executable(tetra-impl
               src/main.cpp
               src/Decoder.cpp)

target_compile_options(tetra-impl PUBLIC -std=c++17 -Wall)

include_directories(src)

find_package(cxxopts REQUIRED)
find_package(ZLIB REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include)

target_link_libraries(tetra-impl ZLIB::ZLIB)

install(TARGETS tetra-impl DESTINATION bin)
