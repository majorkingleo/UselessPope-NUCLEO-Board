cmake_minimum_required (VERSION 3.19)


set(target_name "OS")
add_library(${target_name} STATIC)

target_include_directories(${target_name}
 PUBLIC  "${CMAKE_CURRENT_LIST_DIR}/inc"
)

target_sources(${target_name}
 PUBLIC "${CMAKE_CURRENT_LIST_DIR}/inc/os.hpp"
)

target_sources(${target_name}
 PUBLIC "${CMAKE_CURRENT_LIST_DIR}/src/os.cpp"
)

