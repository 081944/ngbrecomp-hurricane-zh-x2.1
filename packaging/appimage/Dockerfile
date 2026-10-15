# Build environment for the Ninja Gaiden Black AppImage (packaging/appimage/build.sh):
# Ubuntu 22.04, so the program needs no glibc newer than 2.35 and runs on
# SteamOS, Ubuntu 22.04+, Debian 12, Fedora 36+ and other current systems.
# The first RUN matches the Wind Waker AppImage's environment layer for layer,
# so the two share Docker's cache.
FROM ubuntu:22.04

ARG LLVM_VERSION=22
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates wget gnupg lsb-release software-properties-common git file \
        python3 python3-pip pkg-config \
    && add-apt-repository -y ppa:ubuntu-toolchain-r/test \
    && apt-get update && apt-get install -y --no-install-recommends \
        g++-13 libstdc++-13-dev \
        libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev libxss-dev libxtst-dev \
        libxkbcommon-dev libwayland-dev wayland-protocols libdecor-0-dev libegl1-mesa-dev libgl-dev \
        libgbm-dev libdrm-dev libvulkan-dev libpulse-dev libpipewire-0.3-dev libasound2-dev libdbus-1-dev \
        libudev-dev \
    && wget -qO /tmp/llvm.sh https://apt.llvm.org/llvm.sh \
    && bash /tmp/llvm.sh ${LLVM_VERSION} \
    && apt-get install -y --no-install-recommends clang-${LLVM_VERSION} lld-${LLVM_VERSION} llvm-${LLVM_VERSION} \
    && for tool in clang clang++ ld.lld llvm-strip llvm-profdata; do \
           ln -sf /usr/bin/${tool}-${LLVM_VERSION} /usr/local/bin/${tool}; done \
    && python3 -m pip install --no-cache-dir "cmake>=3.25,<4" ninja patchelf \
    && rm -rf /var/lib/apt/lists/* /tmp/llvm.sh

# The runtime's own: glib (the xemu GPU model's QEMU shim), libepoxy (the
# toolkit's OpenGL D3D backend, linked though unused here), OpenSSL (Xbox
# crypto, linked statically).
RUN apt-get update && apt-get install -y --no-install-recommends \
        libglib2.0-dev libepoxy-dev libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# SDL2, newer than 22.04's 2.0.20 (Steam Deck and current pads, PipeWire),
# with every video/audio backend loaded at run time from the player's system.
ARG SDL2_VERSION=2.32.10
RUN wget -qO /tmp/sdl.tar.gz https://github.com/libsdl-org/SDL/releases/download/release-${SDL2_VERSION}/SDL2-${SDL2_VERSION}.tar.gz \
    && tar -xzf /tmp/sdl.tar.gz -C /tmp \
    && cmake -S /tmp/SDL2-${SDL2_VERSION} -B /tmp/sdl-build -G Ninja \
         -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_INSTALL_PREFIX=/usr/local \
         -DSDL_STATIC=OFF -DSDL_SHARED=ON -DSDL_TEST=OFF \
    && cmake --build /tmp/sdl-build && cmake --install /tmp/sdl-build \
    && rm -rf /tmp/sdl.tar.gz /tmp/SDL2-${SDL2_VERSION} /tmp/sdl-build

# glslang with its C interface, static, without the optimizer (and so
# without SPIRV-Tools): the renderer compiles its shaders at run time.
ARG GLSLANG_VERSION=16.6.0
RUN git clone --depth 1 -b ${GLSLANG_VERSION} https://github.com/KhronosGroup/glslang /tmp/glslang \
    && cmake -S /tmp/glslang -B /tmp/glslang-build -G Ninja -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_INSTALL_PREFIX=/usr/local \
         -DBUILD_SHARED_LIBS=OFF -DENABLE_OPT=OFF -DGLSLANG_TESTS=OFF -DENABLE_GLSLANG_BINARIES=OFF \
         -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    && cmake --build /tmp/glslang-build && cmake --install /tmp/glslang-build \
    && rm -rf /tmp/glslang /tmp/glslang-build

# Qt 6 Widgets for the first-run setup window (ngb-setup), bundled with its
# xcb and Wayland platform plugins.
RUN apt-get update && apt-get install -y --no-install-recommends \
        qt6-base-dev qt6-qpa-plugins qt6-wayland libgl1-mesa-dev \
    && rm -rf /var/lib/apt/lists/*
