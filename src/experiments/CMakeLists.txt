# Here we create an executable for each experiment and link it to the rest of the software.

add_executable(packet-parser-example
               src/experiments/packet_parser_example.cpp)

target_link_libraries(packet-parser-example tetra-decoder-library)

add_executable(slots-parser-example
               src/experiments/slots_parser_example.cpp)

target_link_libraries(slots-parser-example tetra-decoder-library)