cmake_minimum_required(VERSION 3.20)

# ============================================================================
# Ninja Gaiden Black - static recompilation (xboxrecomp)
#
# Native Linux (x86-64) and Windows builds. The generated code comes from the
# xboxrecomp pipeline (see README.md); it is not checked in.
# ============================================================================

project(ninja_gaiden_black C CXX)   # CXX: the GPU renderer links VMA

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)

# Bring-up build: record every indirect-branch target the title reaches, so
# tools/feedback.sh can reseed function detection with them. Applies to the
# toolkit too (its icall_feedback.c compiles to nothing without it).
option(NGB_ICALL_FEEDBACK "Record indirect-branch targets (bring-up builds)" OFF)
if(NGB_ICALL_FEEDBACK)
    add_compile_definitions(RECOMP_ICALL_FEEDBACK)
endif()

# The GPU: xemu's NV2A model with its Vulkan renderer (xboxrecomp nv2a_vk).
option(NGB_NV2A_VK "Render with the NV2A Vulkan model" ON)
set(XBOXRECOMP_NV2A_VK ${NGB_NV2A_VK} CACHE BOOL "" FORCE)

set(XBOXRECOMP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../xboxrecomp" CACHE PATH
    "Path to the xboxrecomp toolkit root directory")
add_subdirectory(${XBOXRECOMP_DIR} ${CMAKE_BINARY_DIR}/xboxrecomp)

# ── Recompiled code ─────────────────────────────────────────────

# CONFIGURE_DEPENDS: a regenerated tree can add or drop chunks.
file(GLOB RECOMP_GEN_SOURCES CONFIGURE_DEPENDS "src/recomp/gen/*.c")
if(NOT RECOMP_GEN_SOURCES)
    message(FATAL_ERROR "src/recomp/gen is empty: run the recomp pipeline first (README.md)")
endif()

# The generated chunks are ~2 MB of C each. They are an object library so
# their flags can differ from the hand-written code, and so a host change
# never recompiles them.
add_library(ngb_gen OBJECT ${RECOMP_GEN_SOURCES})
target_include_directories(ngb_gen PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/recomp/gen
    ${XBOXRECOMP_DIR}/include
    ${XBOXRECOMP_DIR}/src
    ${XBOXRECOMP_DIR}/src/kernel
)
target_link_libraries(ngb_gen PRIVATE platform)

if(MSVC)
    target_compile_options(ngb_gen PRIVATE /bigobj /O2)
else()
    # The guest memory model aliases freely through MEM32/MEMF; strict
    # aliasing would let the optimiser reorder guest loads and stores.
    # Lifted code is one big function per guest routine, so the flags that
    # make a function's codegen superlinear are capped.
    target_compile_options(ngb_gen PRIVATE
        -O2 -fno-strict-aliasing -fwrapv
        -Wno-unused-label -Wno-unused-variable -Wno-unused-but-set-variable
        -Wno-parentheses-equality -Wno-tautological-compare
        -Wno-incompatible-pointer-types-discards-qualifiers
    )
endif()

# ── Executable ──────────────────────────────────────────────────

add_executable(${PROJECT_NAME}
    src/main.c
    src/recomp_manual.c
    src/fibers.c
    $<$<NOT:$<PLATFORM_ID:Windows>>:${CMAKE_CURRENT_SOURCE_DIR}/src/install.c>
    $<TARGET_OBJECTS:ngb_gen>
)
target_link_libraries(${PROJECT_NAME} PRIVATE xboxrecomp)
if(NGB_NV2A_VK)
    target_link_libraries(${PROJECT_NAME} PRIVATE xbox_nv2a_vk)
    target_compile_definitions(${PROJECT_NAME} PRIVATE NGB_NV2A_VK)
endif()
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/recomp/gen
    ${XBOXRECOMP_DIR}/include
    ${XBOXRECOMP_DIR}/src
)

if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME NinjaGaidenBlack)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        d3d11 dxgi dxguid xinput winmm dbghelp)
    if(MSVC)
        target_link_options(${PROJECT_NAME} PRIVATE /LARGEADDRESSAWARE)
    endif()
else()
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME NinjaGaidenBlack)
    # -rdynamic: the crash handler names the faulting sub_XXXXXXXX via dladdr.
    target_link_options(${PROJECT_NAME} PRIVATE -rdynamic)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_DL_LIBS} m)
endif()
