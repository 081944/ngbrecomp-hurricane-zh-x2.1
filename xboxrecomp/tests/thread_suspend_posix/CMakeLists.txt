cmake_minimum_required(VERSION 3.20)
project(thread_suspend_posix_test C)
# SuspendThread on POSIX: a thread running guest code stops; one inside host
# code (compat_host_enter) finishes that first and parks on the way out.
if(WIN32)
    message(FATAL_ERROR "This regression covers the POSIX suspension path")
endif()
set(CMAKE_C_STANDARD 11)
add_subdirectory(../../src/platform platform)
add_executable(thread_suspend_posix_test test_main.c)
target_link_libraries(thread_suspend_posix_test PRIVATE platform)
enable_testing()
add_test(NAME thread_suspend_posix_test COMMAND thread_suspend_posix_test)
