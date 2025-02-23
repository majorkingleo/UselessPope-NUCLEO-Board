cmake_minimum_required (VERSION 3.19)


set(target_name "WLIB_MEMORY")

add_library(${target_name} STATIC)
target_compile_features(${target_name} PUBLIC cxx_std_20)


target_include_directories(${target_name}
 PUBLIC  "${CMAKE_CURRENT_LIST_DIR}/inc"
)

target_sources(${target_name}
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/wlib-memory.cpp"
)


