cmake_minimum_required (VERSION 3.19)

set(target_name "WLIB_CONTAINER")
message(STATUS "      -> ${target_name}")
add_library(${target_name} STATIC)

# Interface
target_include_directories(${target_name}
 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/inc"
)

target_sources(${target_name}
 PUBLIC "${CMAKE_CURRENT_LIST_DIR}/src/wlib-Container.cpp"
)

