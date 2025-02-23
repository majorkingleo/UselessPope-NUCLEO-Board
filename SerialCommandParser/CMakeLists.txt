cmake_minimum_required (VERSION 3.19)

set(target_name "SERIAL_COMMAND_PARSER")
message(STATUS "#                    Lib: ${target_name}")
add_library(${target_name} STATIC)

# Interface
target_include_directories(${target_name}
 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/inc"
)

target_sources(${target_name}
 PUBLIC "${CMAKE_CURRENT_LIST_DIR}/inc/serial_command_parser.hpp"
)

# Implementation
target_sources(${target_name}
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/serial_command_parser.cpp"
)

target_link_libraries(${target_name}
 PUBLIC WLIB
 PUBLIC OS
 PUBLIC BSLIB
)
