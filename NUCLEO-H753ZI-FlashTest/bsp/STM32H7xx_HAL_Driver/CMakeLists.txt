cmake_minimum_required (VERSION 3.19)

set(target_name "BSP_NUCLEO-H753ZI-FlashTest_STM32H7xx_HAL_Driver_uC")
message(STATUS "# HAL Driver: ${target_name}")

add_library(${target_name} STATIC)

target_include_directories(${target_name}
 PUBLIC  "${CMAKE_CURRENT_LIST_DIR}/Inc"
)

target_include_directories(${target_name}
 PRIVATE  "${CMAKE_CURRENT_LIST_DIR}/Src"
)

target_sources(${target_name}
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/Src/stm32h7xx_hal.c"
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/Src/stm32h7xx_hal_uart.c"
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/Src/stm32h7xx_hal_uart_ex.c"
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/Src/stm32h7xx_hal_rcc.c"
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/Src/stm32h7xx_hal_rcc_ex.c"
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/Src/stm32h7xx_hal_gpio.c"
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/Src/stm32h7xx_hal_flash.c"
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/Src/stm32h7xx_hal_flash_ex.c"
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/Src/stm32h7xx_hal_cortex.c"
 )

target_compile_definitions(${target_name}
 PUBLIC -DARM_MATH_CM7 -Dflash_layout -DCORE_CM7 -DSTM32H753ZI -DSTM32H753xx
)

target_link_libraries(${target_name}
 PRIVATE uC
)
