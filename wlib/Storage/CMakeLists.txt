cmake_minimum_required (VERSION 3.15)

set(target_name "WLIB_STORAGE")

add_library(${target_name} STATIC)

target_include_directories(	${target_name}
 PUBLIC  "${CMAKE_CURRENT_LIST_DIR}/inc"
)

target_sources(${target_name}
 PUBLIC  "${CMAKE_CURRENT_LIST_DIR}/inc/wlib-storage.hpp"
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/wlib-storage.cpp"
)

target_compile_features(${target_name} PUBLIC cxx_std_20)

target_link_libraries(${target_name}
 PUBLIC WLIB_MEMORY
 PUBLIC WLIB_PROVIDER
 PUBLIC WLIB_CRC
 PUBLIC WLIB_BLOB
)

