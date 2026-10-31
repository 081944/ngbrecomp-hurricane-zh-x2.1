param(
    [string]$Ndk = $env:NDK_ROOT,
    [string]$Gradle,
    [switch]$Install,
    [string]$Serial,
    [switch]$Release,
    [int]$Jobs = 0,
    [switch]$Fresh,
    [switch]$SkipNative
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'common.ps1')
$root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path

function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    # Gradle and cmake write progress and warnings to stderr; with
    # $ErrorActionPreference = 'Stop' PowerShell would abort on the first one.
    $previous = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try { & $Program @Arguments } finally { $ErrorActionPreference = $previous }
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit code $LASTEXITCODE" }
}
function Require-Path([string]$Path, [string]$Hint) {
    if (-not (Test-Path -LiteralPath $Path)) { throw "Missing $Path. $Hint" }
    return $Path
}

if (-not $SkipNative) {
    $Ndk = Resolve-AndroidNdk $Ndk
    $cmake = Resolve-CMake
    $jobs = Get-BuildJobs $Jobs
    $build = Join-Path $root 'build/android-game/arm64-v8a'
    if ($Fresh -and (Test-Path -LiteralPath $build)) {
        Write-Output "Discarding $build (-Fresh)"
        Remove-Item -LiteralPath $build -Recurse -Force
    }
    # A build tree configured for another checkout (another drive, another
    # folder) still holds that path in its cache; discard it instead of failing.
    Reset-StaleCMakeBuild $build $root | Out-Null
    $deps = Require-Path (Join-Path $root 'build/android-deps/arm64-v8a/install/lib/libSDL2.so') `
        'Run tools/android/build-deps.ps1 first.'
    Write-Output "source : $root"
    Write-Output "ndk    : $Ndk"
    Write-Output "cmake  : $cmake"
    Write-Output "jobs   : $jobs"
    Invoke-Checked $cmake @('-Wno-deprecated', '-S', $root, '-B', $build, '-G', 'Ninja',
        "-DCMAKE_TOOLCHAIN_FILE=$Ndk/build/cmake/android.toolchain.cmake",
        '-DANDROID_ABI=arm64-v8a', '-DANDROID_PLATFORM=android-30',
        '-DANDROID_STL=c++_static', '-DCMAKE_BUILD_TYPE=Release')
    Invoke-Checked $cmake @('--build', $build, '-j', "$jobs")
    $native = Join-Path $root 'build/android-package/jniLibs/arm64-v8a'
    New-Item -ItemType Directory -Force $native | Out-Null
    Copy-Item -LiteralPath (Join-Path $build 'libmain.so') -Destination $native
    Copy-Item -LiteralPath $deps -Destination $native
    Copy-Item -LiteralPath (Join-Path $build 'libngbsetup.so') -Destination $native
    Copy-Item -LiteralPath (Join-Path $build 'libngb_vulkan.so') -Destination $native
    Copy-Item -LiteralPath (Join-Path $build 'libngb_sdl_vulkan.so') -Destination $native
    Get-ChildItem -LiteralPath (Join-Path $build 'driver-hooks') -Filter '*.so' |
        ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $native }
    # Keep the original symbol-rich libraries for debugging; strip only the copies
    # staged for the APK. Gradle does not always locate an NDK for prebuilt libraries.
    $strip = Join-Path $Ndk 'toolchains/llvm/prebuilt/windows-x86_64/bin/llvm-strip.exe'
    Invoke-Checked $strip @('--strip-debug', (Join-Path $native 'libmain.so'), (Join-Path $native 'libSDL2.so'), (Join-Path $native 'libngbsetup.so'))
} else {
    Write-Output 'Skipping the native build (-SkipNative): reusing build/android-package/jniLibs.'
}

# Gradle needs a JDK: AGP 8.13 runs on 17+, so borrow Android Studio's when the
# machine has no JAVA_HOME of its own.
$javaHome = Resolve-JavaHome
if (-not $javaHome) { throw 'No JDK found: set JAVA_HOME (JDK 17 or newer).' }
if (-not $env:JAVA_HOME) { $env:JAVA_HOME = $javaHome }
$Gradle = Resolve-Gradle $Gradle
Write-Output "java   : $javaHome"
Write-Output "gradle : $Gradle"
$variant = if ($Release) { 'release' } else { 'debug' }
$task = if ($Release) { 'assembleRelease' } else { 'assembleDebug' }
Invoke-Checked $Gradle @('-p', (Join-Path $root 'android'), $task, '--console', 'plain')
$apk = Join-Path $root "android/app/build/outputs/apk/$variant/app-$variant.apk"
if (-not (Test-Path -LiteralPath $apk)) { throw "Gradle did not produce the expected $variant APK." }
Write-Output "$variant APK: $apk"
if ($Install) {
    $adb = Resolve-Adb
    if (-not $Serial) {
        # Exactly one device online is unambiguous; anything else needs -Serial so
        # the build never lands on the wrong handheld.
        # @() matters: a one-element result would otherwise unroll into a
        # string, whose [0] is the first *character* of the serial.
        $online = @(Get-OnlineAndroidDevices $adb)
        if ($online.Count -eq 0) {
            throw 'No Android device in the "device" state: plug one in, or pass -Serial.'
        }
        if ($online.Count -gt 1) {
            throw ('Pass -Serial: {0} devices online ({1}).' -f $online.Count, ($online -join ', '))
        }
        $Serial = $online[0]
    }
    Write-Output "installing on $Serial"
    Invoke-Checked $adb @('-s', $Serial, 'install', '-r', '-d', $apk)
}
