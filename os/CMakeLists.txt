cmake_minimum_required (VERSION 3.19)


set(CMAKE_OS_NAME                  "FreeRTOS")
set(FREERTOS_CONFIG_FILE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/cfg")
set(FREERTOS_PORT                  "GCC_ARM_CM7")

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/freertos")


set(target_name "OS")
add_library(${target_name} STATIC)

target_include_directories(${target_name}
 PUBLIC  "${CMAKE_CURRENT_LIST_DIR}/inc"
)

target_sources(${target_name}
 PUBLIC "${CMAKE_CURRENT_LIST_DIR}/inc/os.hpp"
)

target_include_directories(	${target_name}
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/inc_FreeRTOS"
)

target_sources(${target_name}
 PUBLIC "${CMAKE_CURRENT_LIST_DIR}/src/os.cpp" 
 PUBLIC "${CMAKE_CURRENT_LIST_DIR}/src/start_runtime.cpp"
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/retarget_malloc.cpp"
)

target_link_libraries(${target_name}
 PUBLIC uC
 PUBLIC freertos_kernel
)

