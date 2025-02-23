cmake_minimum_required (VERSION 3.19)

set(target_name "H753_INTERNAL_FLASH_UPDATE_MEMORY")

message(STATUS "#                    Lib: ${target_name}")
add_library(${target_name} STATIC)

# Interface
target_include_directories(${target_name}
 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/inc"
)

target_sources(${target_name}
 PUBLIC "${CMAKE_CURRENT_LIST_DIR}/inc/bslib-h753_internal_flash_update_memory.hpp"
)

# Implementation
target_sources(${target_name}
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/bslib-h753_internal_flash_update_memory.cpp"
)

target_link_libraries(${target_name}
 PUBLIC simpleflashfs
 PUBLIC uC
)