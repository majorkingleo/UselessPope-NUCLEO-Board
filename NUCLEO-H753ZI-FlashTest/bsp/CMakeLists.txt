cmake_minimum_required (VERSION 3.19)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/STM32H7xx_HAL_Driver")

set(target_name "BSP_NUCLEO-H753ZI-FlashTest_uC")
message(STATUS "# Board-Support-Package: ${target_name}")

add_library(${target_name} STATIC)

target_include_directories(${target_name}
 PUBLIC  "${CMAKE_CURRENT_LIST_DIR}/inc"
)

target_include_directories(${target_name}
 PRIVATE  "${CMAKE_CURRENT_LIST_DIR}/src"
 
 PRIVATE  "${CMAKE_CURRENT_LIST_DIR}/stm32_internal_flash/Inc"
 PRIVATE  "${CMAKE_CURRENT_LIST_DIR}/stm32_internal_flash/stm32fxxx_hal"
)

target_sources(${target_name}
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/bsp.cpp"
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/bsp_uart_usb_hal.cpp" 
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/bsp_internal_fs.cpp" 
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/startup_stm32h753xx.c"
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/system_stm32h7xx.cpp"
 
 PRIVATE  "${CMAKE_CURRENT_LIST_DIR}/stm32_internal_flash/stm32fxxx_hal/stm32_internal_flash_raw_h7.cpp"
 PRIVATE  "${CMAKE_CURRENT_LIST_DIR}/stm32_internal_flash/stm32fxxx_hal/stm32_internal_flash_raw_base.cpp"
 PRIVATE  "${CMAKE_CURRENT_LIST_DIR}/stm32_internal_flash/stm32fxxx_hal/stm32_internal_flash.cpp"
 )

set(LINKER_SCRIPT_PATH "${CMAKE_CURRENT_LIST_DIR}/LinkerScripts/STM32H753ZI_flash.lds" CACHE PATH "Path to the linker script")

target_compile_definitions(${target_name}
 PUBLIC -DARM_MATH_CM7 -Dflash_layout -DCORE_CM7 -DSTM32H753ZI -DSTM32H753xx
)

message(STATUS "#         LINKER-SKRIPT: ${LINKER_SCRIPT_PATH}")
message(STATUS "########################")

target_link_libraries(${target_name}
 PUBLIC OS
 PUBLIC SENSORS
 PUBLIC EXMATH
 PUBLIC BSLIB
 PUBLIC uC
 PUBLIC BSP_NUCLEO-H753ZI-FlashTest_STM32H7xx_HAL_Driver_uC
 PUBLIC simpleflashfs
)
