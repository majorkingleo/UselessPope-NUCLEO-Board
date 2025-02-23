cmake_minimum_required (VERSION 3.19)


set(target_name "BSLIB_UTILITY_INTERFACES")

add_library(${target_name} STATIC)
target_compile_features(${target_name} PUBLIC cxx_std_20)

target_include_directories(${target_name}
 PUBLIC  "${CMAKE_CURRENT_LIST_DIR}/inc"
)

target_sources(${target_name}
 PUBLIC "${CMAKE_CURRENT_LIST_DIR}/inc/bslib-utility_Interfaces.hpp"
)

target_sources(${target_name}
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/bslib-utility_Interfaces.cpp"
)

target_link_libraries(${target_name}
)



