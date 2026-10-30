# Target libraries built by tools/android/build-deps.ps1 and build-glib.ps1.
set(NGB_ANDROID_DEPS "${CMAKE_CURRENT_SOURCE_DIR}/build/android-deps/${ANDROID_ABI}/install"
    CACHE PATH "Android target dependency installation prefix")
list(PREPEND CMAKE_PREFIX_PATH "${NGB_ANDROID_DEPS}")
list(APPEND CMAKE_FIND_ROOT_PATH "${NGB_ANDROID_DEPS}")
find_package(SDL2 CONFIG REQUIRED)
find_package(glslang CONFIG REQUIRED)
set(NGB_ADRENOTOOLS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/android-deps/src/libadrenotools"
    CACHE PATH "Pinned libadrenotools checkout")
if(NOT EXISTS "${NGB_ADRENOTOOLS_DIR}/lib/linkernsbypass/CMakeLists.txt")
    message(FATAL_ERROR "Missing libadrenotools: run tools/android/build-deps.ps1")
endif()
add_subdirectory("${NGB_ADRENOTOOLS_DIR}" "${CMAKE_BINARY_DIR}/adrenotools")
foreach(hook main_hook file_redirect_hook gsl_alloc_hook hook_impl)
    set_target_properties(${hook} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/driver-hooks")
endforeach()
add_library(ngb_vulkan SHARED src/android_vulkan.cpp)
target_compile_features(ngb_vulkan PRIVATE cxx_std_17)
target_include_directories(ngb_vulkan PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(ngb_vulkan PRIVATE adrenotools log dl)
add_library(ngb_sdl_vulkan SHARED src/android_sdl_vulkan.c)
target_link_libraries(ngb_sdl_vulkan PRIVATE ngb_vulkan)
foreach(lib glib-2.0 pcre2-8 intl)
    if(NOT EXISTS "${NGB_ANDROID_DEPS}/lib/lib${lib}.a")
        message(FATAL_ERROR "Missing Android ${lib}: run tools/android/build-glib.ps1")
    endif()
endforeach()
add_library(PkgConfig::NV2A_GLIB STATIC IMPORTED GLOBAL)
set_target_properties(PkgConfig::NV2A_GLIB PROPERTIES
    IMPORTED_LOCATION "${NGB_ANDROID_DEPS}/lib/libglib-2.0.a"
    INTERFACE_INCLUDE_DIRECTORIES "${NGB_ANDROID_DEPS}/include/glib-2.0;${NGB_ANDROID_DEPS}/lib/glib-2.0/include"
    INTERFACE_COMPILE_DEFINITIONS "G_INTL_STATIC_COMPILATION;PCRE2_STATIC"
    INTERFACE_LINK_LIBRARIES "${NGB_ANDROID_DEPS}/lib/libpcre2-8.a;${NGB_ANDROID_DEPS}/lib/libintl.a;atomic;m;${CMAKE_DL_LIBS}")
