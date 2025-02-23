cmake_minimum_required (VERSION 3.19)

set(target_name "simpleflashfs")
message(STATUS "# simpleflashfs: ${target_name}")

add_library(${target_name} STATIC)

target_include_directories(${target_name}
 PUBLIC  "${CMAKE_CURRENT_LIST_DIR}/simpleflashfs/src_2face"
 PUBLIC  "${CMAKE_CURRENT_LIST_DIR}/simpleflashfs/src"
)

target_sources(${target_name}
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/simpleflashfs/src/SimpleFlashFsConstants.cc"
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/simpleflashfs/src_2face/H7TwoFace.cc"
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/simpleflashfs/src_2face/SimpleFlashFsFileBuffer.cc"
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/simpleflashfs/src_2face/SimpleIni.cc"
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/simpleflashfs/src/crc/crc.c"
 )

target_compile_definitions(${target_name}
 PUBLIC -DARM_MATH_CM7 -Dflash_layout -DCORE_CM7 -DSTM32H753ZI -DSTM32H753xx
)

target_link_libraries(${target_name}
 PUBLIC cpputils
)