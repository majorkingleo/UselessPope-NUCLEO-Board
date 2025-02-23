cmake_minimum_required (VERSION 3.19)


set(target_name "NUCLEO-H753ZI-FlashTest_uC")
set(CMAKE_OUTPUT_DIR "${CMAKE_OUTPUT_DIR}/${target_name}")

message(STATUS "#           Application: ${target_name}")
message(STATUS "########################")
message(STATUS "#     TARGET-OUTPUT-DIR: ${CMAKE_OUTPUT_DIR}")
message(STATUS "########################")

add_executable(${target_name})

target_include_directories(${target_name}
 PUBLIC  "${CMAKE_CURRENT_LIST_DIR}/inc"
)

target_sources(${target_name}
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/main.cpp"
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/task_status_led.cpp"
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/CppUtilsUartDebug.cpp"
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/LockedDebugStream.cpp"
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/AnalogValueLogger.cpp"
)

if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
target_sources(${target_name}
 PUBLIC  "${CMAKE_CURRENT_LIST_DIR}/src/utils/sdk_stub.c"
)
endif()

target_link_libraries(${target_name}
 PRIVATE BSP_NUCLEO-H753ZI-FlashTest_uC
 PRIVATE BSP_NUCLEO-H753ZI-FlashTest_STM32H7xx_HAL_Driver_uC
 PRIVATE SERIAL_COMMAND_PARSER
 PRIVATE simpleflashfs
 PRIVATE cpputils
 PRIVATE EXMATH
 PRIVATE SENSORS
)

target_link_options(${target_name}
 PRIVATE "-T${CMAKE_CURRENT_LIST_DIR}/../bsp/LinkerScripts/STM32H753ZI_flash.lds"
 PRIVATE "-Wl,-Map=${target_name}_${PROJECT_VERSION}.map"
 PRIVATE "-Wl,--print-memory-usage"
 PRIVATE "-Wl,--no-warn-rwx-segment"
)

add_custom_command(
  TARGET ${target_name}
  PRE_BUILD
  COMMAND python "${TOOLS_DIR}/clean_directory.py" "${CMAKE_OUTPUT_DIR}"
)

add_custom_command(
  TARGET ${target_name}
  POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} -Oihex ${target_name} ${target_name}.hex
  COMMAND ${CMAKE_OBJCOPY} -Obinary ${target_name} ${target_name}.bin
  COMMAND ${CMAKE_OBJDUMP} --source --all-headers --demangle --line-numbers --wide ${target_name} > ${target_name}.lst
  COMMAND ${CMAKE_COMMAND} -E copy ${target_name}     "${CMAKE_OUTPUT_DIR}/${target_name}.elf"
  COMMAND ${CMAKE_COMMAND} -E copy ${target_name}.hex "${CMAKE_OUTPUT_DIR}/${target_name}_${PROJECT_VERSION}.hex"
  COMMAND ${CMAKE_COMMAND} -E copy ${target_name}.bin "${CMAKE_OUTPUT_DIR}/${target_name}_${PROJECT_VERSION}.bin"
  COMMAND ${CMAKE_COMMAND} -E copy ${target_name}.lst "${CMAKE_OUTPUT_DIR}/${target_name}_${PROJECT_VERSION}.lst"                   
  COMMENT "Generate .hex, .bin and .lst from .elf file")

