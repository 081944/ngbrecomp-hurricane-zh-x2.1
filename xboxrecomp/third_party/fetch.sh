#!/usr/bin/env bash
# Fetch the third-party sources the NV2A Vulkan renderer (ported from xemu)
# builds against, at the revisions xemu pins (subprojects/*.wrap, xemu 478b4f4).
set -euo pipefail
cd "$(dirname "$0")"
get() {   # name url revision
    if [ -d "$1/.git" ]; then git -C "$1" fetch -q --depth 1 origin "$3"; else
        git init -q "$1"; git -C "$1" remote add origin "$2"; git -C "$1" fetch -q --depth 1 origin "$3"; fi
    git -C "$1" checkout -q FETCH_HEAD
    echo "$1 @ $(git -C "$1" rev-parse --short HEAD)"
}
get Vulkan-Headers        https://github.com/KhronosGroup/Vulkan-Headers v1.4.350
get volk                  https://github.com/zeux/volk 3ca312a4f38baa63d8006b6905abbeeb89c8087d
get VulkanMemoryAllocator https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator 3aa921224c154a0d2c43912bc88e1c42ce1f7607
get SPIRV-Reflect         https://github.com/KhronosGroup/SPIRV-Reflect 9137a7e529e4e911a4dbd63f1ce71e4b3e8f4cfc
get nv2a_vsh_cpu          https://github.com/xemu-project/nv2a_vsh_cpu 561fe80da57a881f89000256b459440c0178a7ce
get xxHash                https://github.com/Cyan4973/xxHash v0.8.3
get stb                   https://github.com/nothings/stb 2c980bb59875b0d32144a71867fbdebb2f77cd20
