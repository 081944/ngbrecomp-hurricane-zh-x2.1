#!/usr/bin/env bash
# Ninja Gaiden Black (static recompilation) for Linux as an AppImage.
#
#   packaging/appimage/build.sh [out-dir]      (default build/appimage)
#
# Compiles the game and the xboxrecomp toolkit (a sibling checkout, or
# XBOXRECOMP_DIR) in an Ubuntu 22.04 container (Dockerfile here), so the
# AppImage needs only glibc 2.35, and packs it with appimagetool into
# <out-dir>/NinjaGaidenBlack-x86_64.AppImage.
#
# The recompiled code must already be generated (tools/pipeline.sh). The
# AppImage holds the program and the libraries a system may lack (SDL2,
# glib, libepoxy; libstdc++ and OpenSSL linked in). It holds no game data:
# the first launch asks for the player's disc image and installs the files
# into ~/.local/share/NinjaGaidenBlack/game (see src/install.c).
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
repo=$(cd "$here/../.." && pwd)
toolkit=$(cd "${XBOXRECOMP_DIR:-$repo/../xboxrecomp}" && pwd)
out=$(mkdir -p "${1:-$repo/build/appimage}" && cd "${1:-$repo/build/appimage}" && pwd)
image=ngb-build:jammy
jobs=${JOBS:-$(nproc)}

[ -n "$(ls "$repo"/src/recomp/gen/*.c 2>/dev/null)" ] ||
    { echo "No generated code: run tools/pipeline.sh first." >&2; exit 1; }

docker build -q -t "$image" "$here" >/dev/null

# Same parent directory inside as outside, so ../xboxrecomp resolves.
parent=$(dirname "$repo")
[ "$(dirname "$toolkit")" = "$parent" ] ||
    { echo "Expected xboxrecomp beside $(basename "$repo") in $parent" >&2; exit 1; }

docker run --rm -m 14g -u "$(id -u):$(id -g)" -e HOME=/tmp \
    -v "$parent/$(basename "$repo")":/work/ngb-recomp \
    -v "$toolkit":/work/xboxrecomp:ro \
    -v "$out":/out \
    "$image" bash -euc '
cmake -S /work/ngb-recomp -B /out/build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_EXE_LINKER_FLAGS="-static-libstdc++ -static-libgcc -fuse-ld=lld" \
    -DOPENSSL_USE_STATIC_LIBS=TRUE \
    -DCMAKE_PREFIX_PATH=/usr/local >/out/configure.log
ninja -C /out/build -j'"$jobs"' >/out/build.log

app=/out/AppDir
rm -rf "$app"
mkdir -p "$app/usr/bin" "$app/usr/lib"
cp /out/build/NinjaGaidenBlack "$app/usr/bin/"
llvm-strip "$app/usr/bin/NinjaGaidenBlack"

# Bundle what a system may lack; never glibc, the Vulkan loader or GPU and
# window-system libraries, which must be the systems own.
for lib in $(ldd "$app/usr/bin/NinjaGaidenBlack" | awk "/=>/ {print \$3}"); do
    case "$(basename "$lib")" in
    libSDL2-2.0.so*|libglib-2.0.so*|libpcre.so*|libepoxy.so*) cp -L "$lib" "$app/usr/lib/" ;;
    esac
done
patchelf --set-rpath "\$ORIGIN/../lib" "$app/usr/bin/NinjaGaidenBlack"
for so in "$app"/usr/lib/*.so*; do patchelf --set-rpath "\$ORIGIN" "$so"; done
echo "Bundled:"; ls "$app/usr/lib"
echo "Needs from the system:"; readelf -d "$app/usr/bin/NinjaGaidenBlack" | sed -n "s/.*NEEDED.*\[\(.*\)\]/  \1/p"
'

app=$out/AppDir
cp "$here/ngb.png" "$app/ninja-gaiden-black.png"
cp "$here/ngb.png" "$app/.DirIcon"
cat > "$app/ninja-gaiden-black.desktop" <<'DESKTOP'
[Desktop Entry]
Type=Application
Name=Ninja Gaiden Black
GenericName=Ninja Gaiden Black (static recompilation)
Comment=Ninja Gaiden Black, recompiled for Linux with Vulkan; asks for your own disc image
Exec=NinjaGaidenBlack %f
Icon=ninja-gaiden-black
Categories=Game;ActionGame;
Terminal=false
DESKTOP
cat > "$app/AppRun" <<'APPRUN'
#!/bin/sh
# The program finds its bundled libraries through its rpath, so the tools it
# starts (the file dialog, bsdtar) keep the system's.
here="$(dirname "$(readlink -f "$0")")"
exec "$here/usr/bin/NinjaGaidenBlack" "$@"
APPRUN
chmod +x "$app/AppRun"

tool=$here/.cache/appimagetool-x86_64.AppImage
if [ ! -x "$tool" ]; then
    mkdir -p "$here/.cache"
    wget -qO "$tool" https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage
    chmod +x "$tool"
fi
ARCH=x86_64 "$tool" --appimage-extract-and-run --no-appstream "$app" \
    "$out/NinjaGaidenBlack-x86_64.AppImage" >"$out/appimagetool.log" 2>&1 ||
    { tail -20 "$out/appimagetool.log"; exit 1; }
ls -la "$out/NinjaGaidenBlack-x86_64.AppImage"
