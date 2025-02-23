cmake_minimum_required (VERSION 3.19)

set(target_name "cpputils")
message(STATUS "# cpputils: ${target_name}")

add_library(${target_name} STATIC)

target_include_directories(${target_name}
 PUBLIC  "${CMAKE_CURRENT_LIST_DIR}/cpputils/cpputilsshared"
 PUBLIC  "${CMAKE_CURRENT_LIST_DIR}/cpputils/cpputilsshared/cpputilsformat"
 PUBLIC  "${CMAKE_CURRENT_LIST_DIR}/cpputils/io"
)

target_sources(${target_name}
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/cpputils/cpputilsshared/cpputilsformat/cformat.cc"
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/cpputils/cpputilsshared/cpputilsformat/static_format.cc"
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/cpputils/cpputilsshared/static_debug_exception.cc" 
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/cpputils/cpputilsshared/utf8_util.cc" 
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/cpputils/io/CpputilsDebug.cc" 
 )

target_compile_definitions(${target_name}
 PUBLIC -DARM_MATH_CM7 -Dflash_layout -DCORE_CM7 -DSTM32H753ZI -DSTM32H753xx
)
