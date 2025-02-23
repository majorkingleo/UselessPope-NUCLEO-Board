REM ================================================
REM building by hand... 
REM ================================================
REM
REM # to configure
REM
REM cmake --preset debug .
REM
REM # to build with ninja direct
REM toolchain\win__ninja\ninja.exe -C build\debug\
REM
REM
REM 
REM ================================================
REM building by cmake
REM ================================================
REM 
REM configure presets:
REM   * debug
REM   * release
REM 
REM build presets:
REM   * debug-DCB
REM   * release-DCB
REM   * debug-NUCLEO-FlashTest
REM   * release-NUCLEO-FlashTest
REM 
REM ================================================

cmake --preset debug 
cmake --preset release
cmake --build --preset debug-NUCLEO-FlashTest
