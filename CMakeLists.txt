cmake_minimum_required(VERSION 3.19)

#set(CMAKE_VERBOSE_MAKEFILE on )
#set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON" FORCE)

if (DEFINED ENV{build.major})
    set(BUILD_MAJOR "$ENV{build.major}")
else()
    set(BUILD_MAJOR "0")
endif()
if (DEFINED ENV{build.minor})
    set(BUILD_MINOR  "$ENV{build.minor}")
else()
    set(BUILD_MINOR  "0")
endif()
if (DEFINED ENV{build.counter})
    set(BUILD_PATCH  "$ENV{build.counter}")
else()
    set(BUILD_PATCH  "0")
endif()
if (DEFINED ENV{build.tweak})
    set(BUILD_TWEAK  "$ENV{build.tweak}")
else()
    set(BUILD_TWEAK  "0")
endif()

execute_process(
 COMMAND git config --get remote.origin.url
 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
 OUTPUT_VARIABLE GIT_REMOTE_URL
 OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
 COMMAND git branch --show-current
 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
 OUTPUT_VARIABLE GIT_BRANCH_NAME
 OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
 COMMAND git rev-parse HEAD
 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
 OUTPUT_VARIABLE GIT_COMMIT_ID
 OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
 COMMAND git show -s --format=%ci HEAD
 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
 OUTPUT_VARIABLE GIT_COMMIT_DATETIME
 OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S")


project("SVM-x002_Project"
 VERSION ${BUILD_MAJOR}.${BUILD_MINOR}.${BUILD_PATCH}.${BUILD_TWEAK}
 DESCRIPTION "Boards:[ DCB: I77PD06-A ]"
 HOMEPAGE_URL ""
 LANGUAGES C CXX ASM
)

set(TEMPLATE_DIR "${CMAKE_CURRENT_LIST_DIR}/template_files" )
message(STATUS "########################")
message(STATUS "#          PROJECT NAME: ${CMAKE_PROJECT_NAME}")
message(STATUS "#   PROJECT DESCRIPTION: ${CMAKE_PROJECT_DESCRIPTION}")
message(STATUS "#           PROJECT URL: ${CMAKE_PROJECT_HOMEPAGE_URL}")
message(STATUS "#       PROJECT VERSION: ${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH}.${CMAKE_PROJECT_VERSION_TWEAK}")
message(STATUS "########################")
message(STATUS "#           SYSTEM NAME: ${CMAKE_SYSTEM_NAME}")
message(STATUS "#               OS NAME: ${CMAKE_OS_NAME}")
message(STATUS "#        SYSTEM VERSION: ${CMAKE_SYSTEM_VERSION}")
message(STATUS "#      SYSTEM PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "#         C COMPILER ID: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "#       C++ COMPILER ID: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
#message(STATUS "#         LINKER-SKRIPT: ${LINKER_SCRIPT_PATH}")
message(STATUS "#             GENERATOR: ${CMAKE_MAKE_PROGRAM}")
message(STATUS "#         CONFIGURATION: ${CMAKE_BUILD_TYPE}")
message(STATUS "#            OUTPUT-DIR: ${CMAKE_OUTPUT_DIR}")
message(STATUS "#             TOOLS-DIR: ${TOOLS_DIR}")
message(STATUS "#          TEMPLATE-DIR: ${TEMPLATE_DIR}")
message(STATUS "########################")

if (MSVC)
 message(STATUS "#             GENERATOR: ${CMAKE_MAKE_PROGRAM}")
else()
 add_compile_options(
     "$<$<COMPILE_LANGUAGE:CXX>:-std=c++20>"
     "$<$<COMPILE_LANGUAGE:CXX>:-fstack-protector>"
     "$<$<COMPILE_LANGUAGE:CXX>:-fstack-usage>"
     "$<$<COMPILE_LANGUAGE:CXX>:-Wall>"
     "$<$<COMPILE_LANGUAGE:CXX>:-Wextra>"
     "$<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>"
     "$<$<COMPILE_LANGUAGE:CXX>:-Werror>"
     "$<$<COMPILE_LANGUAGE:CXX>:-Wno-unused-parameter>"
 )
 add_compile_options(
     "-Wno-psabi"
 )
endif()

if(MSVC)
     add_compile_options(
       /W4
       /std:c++20
       /Zi
     )
else()
    if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
     add_compile_options(-Og)
     add_compile_options(-g)
    elseif(${CMAKE_BUILD_TYPE} MATCHES "Release")
     add_compile_options(-O3)
     add_compile_options(-g)
    else()
     message(FATAL_ERROR "Build type not valid select Debug or Release")
    endif()
endif()

enable_testing()

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/os")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/bslib")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/wlib")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/uc-lib")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/ex-math")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/cpputils")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/simpleflashfs")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/SerialCommandParser")

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/NUCLEO-H753ZI-FlashTest")
 





