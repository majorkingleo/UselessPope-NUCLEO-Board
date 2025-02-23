cmake_minimum_required (VERSION 3.19)

set(target_name "WLIB_CRC")
message(STATUS "      -> ${target_name}")
add_library(${target_name} STATIC)

# Interface
target_include_directories(${target_name}
 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/inc"
)

target_sources(${target_name}
 PUBLIC "${CMAKE_CURRENT_LIST_DIR}/inc/wlib-CRC.hpp"
 PUBLIC "${CMAKE_CURRENT_LIST_DIR}/inc/wlib-CRC_Interface.hpp"
 PUBLIC "${CMAKE_CURRENT_LIST_DIR}/inc/wlib-CRC_8.hpp"
 PUBLIC "${CMAKE_CURRENT_LIST_DIR}/inc/wlib-CRC_16_ccitt.hpp"
 PUBLIC "${CMAKE_CURRENT_LIST_DIR}/inc/wlib-CRC_32.hpp"
 PUBLIC "${CMAKE_CURRENT_LIST_DIR}/inc/wlib-CRC_64_go_iso.hpp"
)

# Implementation
target_sources(${target_name}
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/wlib-CRC.cpp"
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/wlib-CRC_8.cpp"
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/wlib-CRC_16_ccitt.cpp"
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/wlib-CRC_32.cpp"
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/wlib-CRC_64_go_iso.cpp"
)
