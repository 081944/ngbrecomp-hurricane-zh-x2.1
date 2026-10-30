#!/usr/bin/env bash
# Ninja Gaiden Black (static recompilation) for Android (arm64), cross-compiled on Linux. Work in progress.
#
#   packaging/android/build.sh [out-dir]      (default build/android-release)
#
# Builds SDL2 and glslang with the NDK into build/android-deps (or NGB_ANDROID_DEPS) once, then the game as
# libmain.so, the library SDL's Java activity loads, and packs both with that activity into
# <out-dir>/NinjaGaidenBlack-android-arm64.apk, signed with a debug key (build/android-deps/debug.keystore).
# Like the other packages it holds no game data: the game files go into the app's folder on the phone
# (src/android_main.c). GLib is not used there: the GPU model takes the slice of it it needs from xboxrecomp's
# shim/glib-lite.
#
# Needs: the Android SDK (ANDROID_HOME, default ~/Android/Sdk) with an NDK, build-tools and a platform, a JDK,
# cmake, ninja, curl, python3. The recompiled code must already be generated (tools/pipeline.sh).
set -euo pipefail
trap 'echo "Failed: see the logs in ${out:-build/android-release} and ${deps:-build/android-deps}." >&2' ERR

here=$(cd "$(dirname "$0")" && pwd)
repo=$(cd "$here/../.." && pwd)
deps=$(mkdir -p "${NGB_ANDROID_DEPS:-$repo/build/android-deps}" && cd "${NGB_ANDROID_DEPS:-$repo/build/android-deps}" && pwd)
out=$(mkdir -p "${1:-$repo/build/android-release}" && cd "${1:-$repo/build/android-release}" && pwd)
sdk=${ANDROID_HOME:-$HOME/Android/Sdk}
ndk=${ANDROID_NDK_HOME:-$(ls -d "$sdk"/ndk/* 2>/dev/null | sort -V | tail -1 || true)}
tools=$(ls -d "$sdk"/build-tools/* 2>/dev/null | sort -V | tail -1 || true)
platform=$(ls -d "$sdk"/platforms/android-* 2>/dev/null | sort -V | tail -1 || true)
abi=arm64-v8a
api=29   # thread-local guest registers need the ELF TLS that Android 10 brought
sdl=2.32.10
glslang=16.0.0

[ -f "$ndk/build/cmake/android.toolchain.cmake" ] || { echo "No Android NDK: set ANDROID_NDK_HOME." >&2; exit 1; }
[ -x "$tools/aapt2" ] && [ -f "$platform/android.jar" ] ||
    { echo "No Android build-tools or platform in $sdk: set ANDROID_HOME." >&2; exit 1; }
[ -n "$(ls "$repo"/src/recomp/gen/*.c 2>/dev/null)" ] ||
    { echo "No generated code: run tools/pipeline.sh first." >&2; exit 1; }

android_cmake() {   # source-dir build-dir [cmake args...]
    local src=$1 bld=$2; shift 2
    cmake -S "$src" -B "$bld" -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE="$ndk/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI=$abi -DANDROID_PLATFORM=android-$api -DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON "$@"
}

# The dependencies, built once into $deps/$abi
prefix=$deps/$abi
mkdir -p "$deps/src"
# (built again when it is not aligned for 16 KB pages, as Android 15's devices may have)
if [ ! -f "$prefix/lib/libSDL2.so" ] ||
   ! "$ndk"/toolchains/llvm/prebuilt/*/bin/llvm-readelf -lW "$prefix/lib/libSDL2.so" | grep -q 'LOAD.*0x4000$'; then
    [ -d "$deps/src/SDL" ] || { curl -fL "https://github.com/libsdl-org/SDL/releases/download/release-$sdl/SDL2-$sdl.tar.gz" |
        tar -xz -C "$deps/src" && mv "$deps/src/SDL2-$sdl" "$deps/src/SDL"; }
    android_cmake "$deps/src/SDL" "$deps/build-SDL-$abi" -DCMAKE_INSTALL_PREFIX="$prefix" \
        -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST=OFF >"$deps/build-SDL-$abi.log"
    cmake --build "$deps/build-SDL-$abi" --target install >>"$deps/build-SDL-$abi.log"
fi
# (with HLSL input: the Direct3D renderer's shaders are HLSL, src/d3d/vk/spirv.cpp)
if [ ! -f "$prefix/lib/libglslang.a" ] ||
   ! grep -q '^ENABLE_HLSL:BOOL=ON' "$deps/build-glslang-$abi/CMakeCache.txt" 2>/dev/null; then
    [ -d "$deps/src/glslang" ] || { curl -fL "https://github.com/KhronosGroup/glslang/archive/refs/tags/$glslang.tar.gz" |
        tar -xz -C "$deps/src" && mv "$deps/src/glslang-$glslang" "$deps/src/glslang"; }
    android_cmake "$deps/src/glslang" "$deps/build-glslang-$abi" -DCMAKE_INSTALL_PREFIX="$prefix" \
        -DBUILD_SHARED_LIBS=OFF -DENABLE_OPT=OFF -DENABLE_HLSL=ON -DGLSLANG_TESTS=OFF -DGLSLANG_ENABLE_INSTALL=ON \
        -DENABLE_GLSLANG_BINARIES=OFF >"$deps/build-glslang-$abi.log"
    cmake --build "$deps/build-glslang-$abi" --target install >>"$deps/build-glslang-$abi.log"
fi

# The game
android_cmake "$repo" "$out/build" -DCMAKE_FIND_ROOT_PATH="$prefix" -DCMAKE_PREFIX_PATH="$prefix" \
    -DXBOXRECOMP_DIR="$(cd "${XBOXRECOMP_DIR:-$repo/xboxrecomp}" && pwd)" >"$out/configure.log"
ninja -C "$out/build" -j"${JOBS:-$(nproc)}" ninja_gaiden_black >"$out/build.log"

# The APK: SDL's Java side and our activity, the manifest, the two libraries
apk=$out/apk
rm -rf "${apk:?}"
mkdir -p "$apk/classes" "$apk/dex" "$apk/root/lib/$abi"
javac -source 8 -target 8 -nowarn -Xlint:none -cp "$platform/android.jar" -d "$apk/classes" \
    "$deps"/src/SDL/android-project/app/src/main/java/org/libsdl/app/*.java \
    "$here"/java/io/github/gamboozino/ngbrecomp/*.java 2>"$apk/javac.log"
"$tools/d8" --release --min-api $api --lib "$platform/android.jar" --output "$apk/dex" \
    $(find "$apk/classes" -name '*.class')
"$tools/aapt2" compile --dir "$here/res" -o "$apk/res.zip"   # (the icon: the AppImage's)
"$tools/aapt2" link -o "$apk/base.apk" -I "$platform/android.jar" --manifest "$here/AndroidManifest.xml" \
    "$apk/res.zip"
cp "$apk/dex/classes.dex" "$apk/root/"
ndkbin=$(echo "$ndk"/toolchains/llvm/prebuilt/*/bin)
"$ndkbin/llvm-strip" -o "$apk/root/lib/$abi/libmain.so" "$out/build/libmain.so"
cp "$prefix/lib/libSDL2.so" "$apk/root/lib/$abi/"
python3 - "$apk/base.apk" "$apk/root" <<'PY'
import os, sys, zipfile
with zipfile.ZipFile(sys.argv[1], 'a', zipfile.ZIP_DEFLATED) as z:
    for d, _, files in os.walk(sys.argv[2]):
        for f in files:
            path = os.path.join(d, f)
            z.write(path, os.path.relpath(path, sys.argv[2]))
PY
"$tools/zipalign" -f -p 16 "$apk/base.apk" "$apk/aligned.apk"
key=$deps/debug.keystore
[ -f "$key" ] || keytool -genkeypair -keystore "$key" -storepass android -keypass android -alias ngb \
    -keyalg RSA -keysize 2048 -validity 10000 -dname "CN=Ninja Gaiden Black debug" >/dev/null 2>&1
"$tools/apksigner" sign --ks "$key" --ks-pass pass:android --ks-key-alias ngb \
    --out "$out/NinjaGaidenBlack-android-arm64.apk" "$apk/aligned.apk"
ls -l "$out/NinjaGaidenBlack-android-arm64.apk"
