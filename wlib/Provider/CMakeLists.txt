cmake_minimum_required (VERSION 3.15)

set(target_name "WLIB_PROVIDER")

add_library(${target_name} STATIC)

target_include_directories(	${target_name}
 PUBLIC  "${CMAKE_CURRENT_LIST_DIR}/inc"
)

target_sources(${target_name}
 PUBLIC  "${CMAKE_CURRENT_LIST_DIR}/inc/wlib-Provider_Interface.hpp"
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/wlib-Provider_Interface.cpp"
)

target_compile_features(${target_name} PUBLIC cxx_std_20)

