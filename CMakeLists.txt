cmake_minimum_required (VERSION 3.0.0)
project (fpga-json-parser)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(nlohmann_json)

add_executable (fpga-json-parser 
    main.cc
    Cell.cpp
    Port.cpp
)

target_link_libraries(fpga-json-parser PRIVATE nlohmann_json::nlohmann_json)
