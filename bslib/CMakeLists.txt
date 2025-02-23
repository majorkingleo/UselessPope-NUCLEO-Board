cmake_minimum_required (VERSION 3.19)


set(target_name "BSLIB")

add_library(${target_name} STATIC)
target_compile_features(${target_name} PUBLIC cxx_std_20)


message(STATUS "########################|")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/LED")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Container")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Publisher")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Provider")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Utility_Interfaces")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/H753_internal_flash_update_memory")



target_include_directories(${target_name}
 PUBLIC  "${CMAKE_CURRENT_LIST_DIR}/inc"
)

target_sources(${target_name}
 PUBLIC "${CMAKE_CURRENT_LIST_DIR}/inc/bslib.hpp"
)

target_sources(${target_name}
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/bslib.cpp"
)

target_link_libraries(${target_name}
 PUBLIC BSLIB_LED
 PUBLIC BSLIB_CONTAINER
 PUBLIC BSLIB_PUBLISHER
 PUBLIC BSLIB_UTILITY_INTERFACES
 PUBLIC BSLIB_PROVIDER
 PUBLIC H753_INTERNAL_FLASH_UPDATE_MEMORY
 PUBLIC WLIB
 PUBLIC EXMATH
 PUBLIC OS
 PUBLIC simpleflashfs
)

