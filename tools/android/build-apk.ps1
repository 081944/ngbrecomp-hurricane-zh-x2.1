param(
    [string]$Ndk = $env:NDK_ROOT,
    [string]$Gradle,
    [switch]$Install,
    [string]$Serial,
    [switch]$Release
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
if (-not $Ndk) { throw 'Set NDK_ROOT or pass -Ndk.' }
if (-not $Gradle) {
    $command = Get-Command gradle -ErrorAction SilentlyContinue
    if ($command) { $Gradle = $command.Source }
    else {
        $cache = Join-Path $env:USERPROFILE '.gradle/wrapper/dists/gradle-8.13-bin'
        $candidate = Get-ChildItem -Path $cache -Filter gradle.bat -Recurse -ErrorAction SilentlyContinue |
            Select-Object -First 1
        if ($candidate) { $Gradle = $candidate.FullName }
    }
}
if (-not $Gradle) { throw 'Pass -Gradle with Gradle 8.13, or add it to PATH.' }
function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit code $LASTEXITCODE" }
}
$build = Join-Path $root 'build/android-game/arm64-v8a'
Invoke-Checked cmake @('-Wno-deprecated', '-S', $root, '-B', $build, '-G', 'Ninja',
    "-DCMAKE_TOOLCHAIN_FILE=$Ndk/build/cmake/android.toolchain.cmake",
    '-DANDROID_ABI=arm64-v8a', '-DANDROID_PLATFORM=android-30',
    '-DANDROID_STL=c++_static', '-DCMAKE_BUILD_TYPE=Release')
Invoke-Checked cmake @('--build', $build, '-j', '4')
$native = Join-Path $root 'build/android-package/jniLibs/arm64-v8a'
New-Item -ItemType Directory -Force $native | Out-Null
Copy-Item -LiteralPath (Join-Path $build 'libmain.so') -Destination $native
Copy-Item -LiteralPath (Join-Path $root 'build/android-deps/arm64-v8a/install/lib/libSDL2.so') -Destination $native
Copy-Item -LiteralPath (Join-Path $build 'libngbsetup.so') -Destination $native
Copy-Item -LiteralPath (Join-Path $build 'libngb_vulkan.so') -Destination $native
Copy-Item -LiteralPath (Join-Path $build 'libngb_sdl_vulkan.so') -Destination $native
Get-ChildItem -LiteralPath (Join-Path $build 'driver-hooks') -Filter '*.so' |
    ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $native }
# Keep the original symbol-rich libraries for debugging; strip only the copies
# staged for the APK. Gradle does not always locate an NDK for prebuilt libraries.
$strip = Join-Path $Ndk 'toolchains/llvm/prebuilt/windows-x86_64/bin/llvm-strip.exe'
Invoke-Checked $strip @('--strip-debug', (Join-Path $native 'libmain.so'), (Join-Path $native 'libSDL2.so'), (Join-Path $native 'libngbsetup.so'))
$variant = if ($Release) { 'release' } else { 'debug' }
$task = if ($Release) { 'assembleRelease' } else { 'assembleDebug' }
Invoke-Checked $Gradle @('-p', (Join-Path $root 'android'), $task, '--console', 'plain')
$apk = Join-Path $root "android/app/build/outputs/apk/$variant/app-$variant.apk"
if (-not (Test-Path -LiteralPath $apk)) { throw "Gradle did not produce the expected $variant APK." }
Write-Output "$variant APK: $apk"
if ($Install) {
    if (-not $Serial) { throw 'Pass an explicit -Serial for the intended target device.' }
    Invoke-Checked adb @('-s', $Serial, 'install', '-r', $apk)
}
