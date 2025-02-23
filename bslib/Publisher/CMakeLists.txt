cmake_minimum_required (VERSION 3.19)

set(target_name "BSLIB_PUBLISHER")
add_library(${target_name} STATIC)

target_include_directories(${target_name}
 PUBLIC  "${CMAKE_CURRENT_LIST_DIR}/inc"
)

target_sources(${target_name}
 PUBLIC  "${CMAKE_CURRENT_LIST_DIR}/inc/bslib-Publisher.hpp"
 PUBLIC  "${CMAKE_CURRENT_LIST_DIR}/inc/bslib-LF_publisher.hpp"
 PUBLIC  "${CMAKE_CURRENT_LIST_DIR}/inc/bslib-SPSC_subscriber.hpp"
)

target_sources(${target_name}
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/bslib-Publisher.cpp"
)

target_link_libraries(${target_name}
 PUBLIC OS
 PUBLIC WLIB
 PUBLIC BSLIB_CONTAINER
)

