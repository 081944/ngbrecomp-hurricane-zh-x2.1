param(
    [ValidateSet('arm64-v8a', 'x86_64')][string]$Abi = 'arm64-v8a',
    [int]$Api = 30,
    [string]$Ndk = $env:NDK_ROOT
)
# SDL2 and glslang only. GLib is still being integrated separately.
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
if (-not $Ndk) { throw 'Set NDK_ROOT or pass -Ndk.' }
$toolchain = Join-Path $Ndk 'build/cmake/android.toolchain.cmake'
if (-not (Test-Path -LiteralPath $toolchain)) { throw "NDK toolchain not found: $toolchain" }
$base = Join-Path $root 'build/android-deps'
$prefix = Join-Path $base "$Abi/install"
New-Item -ItemType Directory -Force (Join-Path $base 'src') | Out-Null

function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit code $LASTEXITCODE" }
}

function Ensure-Source([string]$Name, [string]$Url, [string]$Commit) {
    $source = Join-Path $base "src/$Name"
    if (-not (Test-Path -LiteralPath $source)) {
        Invoke-Checked git @('init', '-q', $source)
        Invoke-Checked git @('-C', $source, 'remote', 'add', 'origin', $Url)
        Invoke-Checked git @('-C', $source, 'fetch', '-q', '--depth', '1', 'origin', $Commit)
        Invoke-Checked git @('-C', $source, 'checkout', '-q', 'FETCH_HEAD')
    }
    $actual = & git -C $source rev-parse HEAD
    if ($LASTEXITCODE -ne 0 -or $actual.Trim() -ne $Commit) {
        throw "Unexpected source revision in $source; expected $Commit. Existing files were preserved."
    }
    return $source
}

function Build-Library([string]$Name, [string]$Source, [string[]]$Options) {
    $build = Join-Path $base "$Abi/$Name"
    Invoke-Checked cmake (@('-Wno-deprecated', '-S', $Source, '-B', $build,
        '-G', 'Ninja', "-DCMAKE_TOOLCHAIN_FILE=$toolchain", "-DANDROID_ABI=$Abi",
        "-DANDROID_PLATFORM=android-$Api", '-DCMAKE_BUILD_TYPE=Release',
        '-DCMAKE_POSITION_INDEPENDENT_CODE=ON', "-DCMAKE_INSTALL_PREFIX=$prefix") + $Options)
    Invoke-Checked cmake @('--build', $build, '-j', '4')
    Invoke-Checked cmake @('--install', $build)
}

$adreno = Ensure-Source 'libadrenotools' 'https://github.com/bylaws/libadrenotools.git' `
    '8fae8ce254dfc1344527e05301e43f37dea2df80'
Invoke-Checked git @('-C', $adreno, 'submodule', 'update', '--init', '--recursive')
$sdl = Ensure-Source 'SDL' 'https://github.com/libsdl-org/SDL.git' `
    '98d1f3a45aae568ccd6ed5fec179330f47d4d356'
Build-Library 'SDL' $sdl @('-DSDL_SHARED=ON', '-DSDL_STATIC=OFF', '-DSDL_TEST=OFF',
    '-DSDL_TESTS=OFF', '-DSDL_INSTALL=ON', '-DCMAKE_SHARED_LINKER_FLAGS=-Wl,-z,max-page-size=16384')
$glslang = Ensure-Source 'glslang' 'https://github.com/KhronosGroup/glslang.git' `
    'b5782e52ee2f7b3e40bb9c80d15b47016e008bc9'
# Match glslang 16.1.0's known_good.json. HLSL needs the optimizer/legalizer.
$null = Ensure-Source 'glslang/External/spirv-tools' 'https://github.com/KhronosGroup/SPIRV-Tools.git' `
    '262bdab48146c937467f826699a40da0fdfc0f1a'
$null = Ensure-Source 'glslang/External/spirv-tools/external/spirv-headers' `
    'https://github.com/KhronosGroup/SPIRV-Headers.git' 'b824a462d4256d720bebb40e78b9eb8f78bbb305'
Build-Library 'glslang' $glslang @('-DENABLE_GLSLANG_BINARIES=OFF', '-DENABLE_OPT=ON',
    '-DENABLE_HLSL=ON', '-DENABLE_PCH=OFF', '-DGLSLANG_TESTS=OFF',
    '-DSPIRV_SKIP_TESTS=ON', '-DSPIRV_SKIP_EXECUTABLES=ON', '-DSPIRV_WERROR=OFF')
Write-Output "Android dependency prefix: $prefix"
