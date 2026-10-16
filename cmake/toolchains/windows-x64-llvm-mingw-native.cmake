# Build for Windows x64 ON Windows with llvm-mingw's Windows-host release (clang, lld, libc++, UCRT) against
# MSYS2's CLANG64 packages unpacked into a sysroot -- the same compiler and packages as
# windows-x64-llvm-mingw.cmake (the Linux cross build), only run natively. Differences from that file:
# pkg-config is the sysroot's own pkgconf.exe (Windows has none), and pkg-config search paths are separated
# by ';' on Windows (':' would split the drive letter).
# NGB_WIN_DEPS: the folder holding llvm-mingw-<ver>-ucrt-x86_64/ and sysroot/clang64/.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR AMD64)
set(NGB_WIN_DEPS "${CMAKE_CURRENT_LIST_DIR}/../../build/windows-deps" CACHE PATH "llvm-mingw and the CLANG64 sysroot")
# try_compile projects read this file again: hand them the same NGB_WIN_DEPS
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES NGB_WIN_DEPS)
file(GLOB _llvm_mingw "${NGB_WIN_DEPS}/llvm-mingw-*-ucrt-x86_64")
list(GET _llvm_mingw 0 _llvm_mingw)
set(_triple x86_64-w64-mingw32)
set(CMAKE_C_COMPILER "${_llvm_mingw}/bin/${_triple}-clang.exe")
set(CMAKE_CXX_COMPILER "${_llvm_mingw}/bin/${_triple}-clang++.exe")
set(CMAKE_RC_COMPILER "${_llvm_mingw}/bin/${_triple}-windres.exe")
set(CMAKE_AR "${_llvm_mingw}/bin/llvm-ar.exe")
set(CMAKE_RANLIB "${_llvm_mingw}/bin/llvm-ranlib.exe")
set(NGB_WIN_SYSROOT "${NGB_WIN_DEPS}/sysroot/clang64")
set(CMAKE_FIND_ROOT_PATH "${NGB_WIN_SYSROOT}" "${_llvm_mingw}/${_triple}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(CMAKE_PREFIX_PATH "${NGB_WIN_SYSROOT}")
# pkg-config: the sysroot's pkgconf and .pc files only
set(PKG_CONFIG_EXECUTABLE "${NGB_WIN_SYSROOT}/bin/pkgconf.exe" CACHE FILEPATH "")
set(ENV{PKG_CONFIG_LIBDIR} "${NGB_WIN_SYSROOT}/lib/pkgconfig;${NGB_WIN_SYSROOT}/share/pkgconfig")
set(ENV{PKG_CONFIG_SYSROOT_DIR} "${NGB_WIN_DEPS}/sysroot")
set(ENV{PKG_CONFIG_PATH} "")
# The sysroot's headers for every target (glslang's for the renderer, which links it through xboxrecomp)
set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES "${NGB_WIN_SYSROOT}/include")
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES "${NGB_WIN_SYSROOT}/include")
