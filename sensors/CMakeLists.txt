cmake_minimum_required (VERSION 3.19)


set(target_name "SENSORS")

add_library(${target_name} STATIC)
target_compile_features(${target_name} PUBLIC cxx_std_20)


message(STATUS "    -> SENSORS-Comp:")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/CPU_Temperature")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/NTC_Temperature")
message(STATUS "########################")



target_include_directories(${target_name}
 PUBLIC  "${CMAKE_CURRENT_LIST_DIR}/inc"
)

target_sources(${target_name}
 PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/sensors.cpp"
)

target_link_libraries(${target_name}
 PUBLIC WLIB
 PUBLIC SENSORS_CPU_TEMPERATURE
 PUBLIC SENSORS_NTC_TEMPERATURE
)



