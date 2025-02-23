cmake_minimum_required (VERSION 3.19)


set(target_name "WLIB")

add_library(${target_name} STATIC)
target_compile_features(${target_name} PUBLIC cxx_std_20)


message(STATUS "    -> WLIB-Comp:")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/CRC")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/HASH")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/BLOB")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Callback")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Container")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Publisher")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/SPI_Abstraction")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/IO")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/StringSink")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Memory")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Provider")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Storage")
message(STATUS "########################")



target_include_directories(${target_name}
 PUBLIC  "${CMAKE_CURRENT_LIST_DIR}/inc"
)

target_sources(${target_name}
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/wlib.cpp"
)

target_link_libraries(${target_name}
 PUBLIC WLIB_CRC
 PUBLIC WLIB_HASH
 PUBLIC WLIB_BLOB
 PUBLIC WLIB_CALLBACK
 PUBLIC WLIB_CONTAINER
 PUBLIC WLIB_PUBLISHER
 PUBLIC WLIB_SPI_ABSTRACTION
 PUBLIC WLIB_IO
 PUBLIC WLIB_STRINGSINK
 PUBLIC WLIB_MEMORY
 PUBLIC WLIB_PROVIDER
 PUBLIC WLIB_STORAGE
)



