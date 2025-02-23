cmake_minimum_required (VERSION 3.19)


set(target_name "EXMATH")

add_library(${target_name} STATIC)
target_compile_features(${target_name} PUBLIC cxx_std_20)


message(STATUS "    -> EXMATH-Comp:")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/statistics")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/intervals")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/polynominal")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/constants")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/blas")
message(STATUS "########################")



target_include_directories(${target_name}
 PUBLIC  "${CMAKE_CURRENT_LIST_DIR}/inc"
)

target_sources(${target_name}
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/exmath.cpp"
)

target_link_libraries(${target_name}
 PUBLIC EXMATH_STATISTICS
 PUBLIC EXMATH_INTERVALS
 PUBLIC EXMATH_POLYNOMINAL
 PUBLIC EXMATH_CONSTANTS
 PUBLIC EXMATH_BLAS
)



