cmake_minimum_required (VERSION 3.0.0)
project (fpga-json-parser)

add_subdirectory(nlohmann_json)

add_executable (fpga-json-parser main.cc)

target_link_libraries(fpga-json-parser PRIVATE nlohmann_json::nlohmann_json)
