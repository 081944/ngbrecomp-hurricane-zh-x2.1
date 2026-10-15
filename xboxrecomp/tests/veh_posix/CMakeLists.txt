cmake_minimum_required(VERSION 3.20)
project(veh_posix_test C)
# Vectored exception handlers emulated over sigaction: the register traps for
# the NV2A, APU and OHCI all depend on a fault reaching a handler with a
# writable CONTEXT, and on a TF single-step arriving as EXCEPTION_SINGLE_STEP.
if(WIN32)
    message(FATAL_ERROR "This regression covers the POSIX VEH emulation")
endif()
set(CMAKE_C_STANDARD 11)
add_subdirectory(../../src/platform platform)
add_executable(veh_posix_test test_main.c)
target_link_libraries(veh_posix_test PRIVATE platform)
enable_testing()
add_test(NAME veh_posix_test COMMAND veh_posix_test)
