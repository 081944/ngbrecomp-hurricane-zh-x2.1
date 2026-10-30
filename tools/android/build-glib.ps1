param([int]$Api = 30, [string]$Ndk = $env:NDK_ROOT)
# GLib 2.82.4 on the native Windows host; no WSL or host GLib libraries.
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
if (-not $Ndk) { throw 'Set NDK_ROOT or pass -Ndk.' }
$llvm = Join-Path $Ndk 'toolchains/llvm/prebuilt/windows-x86_64/bin'
if (-not (Test-Path -LiteralPath (Join-Path $llvm 'clang.exe'))) { throw 'Windows NDK LLVM not found.' }
$base = Join-Path $root 'build/android-deps'
$source = Join-Path $base 'src/glib'
$build = Join-Path $base 'arm64-v8a/glib'
$prefix = Join-Path $base 'arm64-v8a/install'
$hostTools = Join-Path $base 'host-tools'
New-Item -ItemType Directory -Force (Join-Path $base 'arm64-v8a') | Out-Null

function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit code $LASTEXITCODE" }
}
function Ensure-Source([string]$Path, [string]$Url, [string]$Commit) {
    if (-not (Test-Path -LiteralPath $Path)) {
        Invoke-Checked git @('init', '-q', $Path)
        Invoke-Checked git @('-C', $Path, 'remote', 'add', 'origin', $Url)
        Invoke-Checked git @('-C', $Path, 'fetch', '-q', '--depth', '1', 'origin', $Commit)
        Invoke-Checked git @('-C', $Path, 'checkout', '-q', 'FETCH_HEAD')
    }
    $actual = & git -C $Path rev-parse HEAD
    if ($LASTEXITCODE -ne 0 -or $actual.Trim() -ne $Commit) {
        throw "Unexpected source revision in $Path; existing files were preserved."
    }
}
Ensure-Source $source 'https://github.com/GNOME/glib.git' 'ca20e4ac71864f08e980dc044ac96c06d5482b37'
Ensure-Source (Join-Path $source 'subprojects/libffi') `
    'https://gitlab.freedesktop.org/gstreamer/meson-ports/libffi.git' '83d0cfd00d7d37af4b4349511d29f1f0512621b3'
Ensure-Source (Join-Path $source 'subprojects/proxy-libintl') `
    'https://github.com/frida/proxy-libintl.git' 'c03e1a74b17fa7ec467e110130775409e4828a4c'
$patch = Join-Path $PSScriptRoot 'glib-windows-cross.patch'
& git -C $source apply --reverse --check $patch 2>$null
if ($LASTEXITCODE -ne 0) { Invoke-Checked git @('-C', $source, 'apply', $patch) }
if (-not (Test-Path -LiteralPath (Join-Path $hostTools 'Scripts/python.exe'))) {
    Invoke-Checked python @('-m', 'venv', $hostTools)
}
$python = Join-Path $hostTools 'Scripts/python.exe'
$meson = Join-Path $hostTools 'Scripts/meson.exe'
# 1.6.1 accepts Windows installation prefixes for this cross build. Later
# Meson path checks reject the drive-letter prefix under the Android target.
Invoke-Checked $python @('-m', 'pip', 'install', 'meson==1.6.1')
$bin = $llvm.Replace('\', '/')
$cross = Join-Path $base 'arm64-v8a/android.ini'
@"
[binaries]
c = '$bin/clang.exe'
cpp = '$bin/clang++.exe'
ar = '$bin/llvm-ar.exe'
strip = '$bin/llvm-strip.exe'
pkg-config = 'pkgconf'
[host_machine]
system = 'android'
cpu_family = 'aarch64'
cpu = 'aarch64'
endian = 'little'
[properties]
needs_exe_wrapper = true
pkg_config_libdir = []
[built-in options]
c_args = ['--target=aarch64-linux-android$Api', '-fPIC']
cpp_args = ['--target=aarch64-linux-android$Api', '-fPIC']
c_link_args = ['--target=aarch64-linux-android$Api', '-Wl,-z,max-page-size=16384']
cpp_link_args = ['--target=aarch64-linux-android$Api', '-Wl,-z,max-page-size=16384']
"@ | Set-Content -LiteralPath $cross -Encoding utf8NoBOM
$setup = @('setup', $build, $source, '--cross-file', $cross, '--prefix', $prefix,
    '--libdir', 'lib', '--buildtype', 'release', '-Ddefault_library=static',
    '-Dtests=false', '-Dintrospection=disabled', '-Ddocumentation=false',
    '-Dman-pages=disabled', '-Dselinux=disabled', '-Dlibmount=disabled',
    '-Dxattr=false', '-Dlibelf=disabled', '-Dsysprof=disabled', '-Dnls=disabled',
    '-Dglib_debug=disabled', '-Dbsymbolic_functions=false')
if (Test-Path -LiteralPath (Join-Path $build 'meson-private/coredata.dat')) { $setup += '--reconfigure' }
Invoke-Checked $meson $setup
Invoke-Checked $meson @('compile', '-C', $build, '-j', '4')
Invoke-Checked $meson @('install', '-C', $build, '--no-rebuild', '--tags', 'devel,runtime')
Write-Output "Android GLib prefix: $prefix"
