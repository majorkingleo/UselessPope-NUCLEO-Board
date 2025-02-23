cmake_minimum_required(VERSION 3.19)

if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
	set(toolchain_dir      "${CMAKE_CURRENT_LIST_DIR}/../win__arm-eabi-gcc_13_2_1")
	set(EXE	".exe")
	set(NINJA_EXE 		"${CMAKE_CURRENT_LIST_DIR}/../win__ninja/ninja.exe")
elseif( ${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Linux" )
	set(toolchain_dir      "/usr/")
	set(EXE "")
	set(NINJA_EXE 		"ninja")
else()
	message(STATUS "${CMAKE_HOST_SYSTEM_NAME}")
	message(SEND_ERROR "Host OS ${CMAKE_HOST_SYSTEM_NAME} is not supported")
endif()

set(CMAKE_SYSTEM_NAME		Generic)
set(CMAKE_SYSTEM_VERSION    13.2.1)
set(CMAKE_SYSTEM_PROCESSOR  STM32H753xx)

set(TOOLS_DIR          "${CMAKE_CURRENT_LIST_DIR}/../utility")
set(toolchain_bin_dir  "${toolchain_dir}/bin")
set(toolchain_inc_dir  "${toolchain_dir}/arm-none-eabi/include")
set(toolchain_lib_dir  "${toolchain_dir}/arm-none-eabi/lib")

set(ARM_OPTIONS
 -mcpu=cortex-m7 
 -mthumb
 -mfpu=fpv5-d16
 -mfloat-abi=hard
 -fexceptions
 --specs=nosys.specs
 #--specs=nano.specs
 -u_printf_float
 -u_scanf_float
 #-flto
)

add_compile_options(
  ${ARM_OPTIONS}
  -ggdb
  -fno-builtin
  -ffunction-sections
  -fdata-sections
  -fomit-frame-pointer
  -fmessage-length=0
  -ffunction-sections
  -fdata-sections
  "$<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>"
  #"$<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>"
  -MMD
  -MP
)

add_link_options(
  ${ARM_OPTIONS}
  #LINKER:-Wl
  LINKER:-gc-sections
  LINKER:-lstdc++
  LINKER:--gc-sections	# Remove unused functions
  # LINKER:--build-id   # GNU build-ID https://interrupt.memfault.com/blog/gnu-build-id-for-firmware
)

set(CMAKE_C_COMPILER   "${toolchain_bin_dir}/arm-none-eabi-gcc${EXE}")
set(CMAKE_CXX_COMPILER "${toolchain_bin_dir}/arm-none-eabi-g++${EXE}")
set(CMAKE_ASM_COMPILER "${toolchain_bin_dir}/arm-none-eabi-gcc${EXE}")
set(CMAKE_OBJCOPY      "${toolchain_bin_dir}/arm-none-eabi-objcopy${EXE}")
set(CMAKE_SIZE         "${toolchain_bin_dir}/arm-none-eabi-size${EXE}")

set(CMAKE_EXECUTABLE_SUFFIX ".elf")
set(CMAKE_FIND_ROOT_PATH "${toolchain_dir}")

set(CMAKE_TRY_COMPILE_TARGET_TYPE     STATIC_LIBRARY)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

