cmake_minimum_required (VERSION 3.19)


set(target_name "SENSORS_CPU_TEMPERATURE")
message(STATUS "      -> ${target_name}")
add_library(${target_name} STATIC)

target_include_directories(${target_name}
 PUBLIC  "${CMAKE_CURRENT_LIST_DIR}/inc"
)

target_sources(${target_name}
 PUBLIC  "${CMAKE_CURRENT_LIST_DIR}/inc/sensors-CPU_Temperature.hpp"
)

target_sources(${target_name}
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/sensors-CPU_Temperature.cpp"
)

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "STM32H753xx")
 target_sources(${target_name}
  PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/STM32H753xx/sensors-CPU_Temperature_STM32H753xx.cpp"
 )
else()
 target_sources(${target_name}
  PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/PC/sensors-CPU_Temperature_PC.cpp"
 )
endif()
