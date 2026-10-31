param(
    [string]$Ndk = $env:NDK_ROOT,
    [string]$Gradle,
    [switch]$Install,
    [string]$Serial,
    [switch]$Release,
    [int]$Jobs = 0,
    [switch]$Fresh,
    [switch]$SkipDeps
)
# One command from a clean checkout to an APK: the Android dependency prefix
# (SDL2, glslang, GLib), the game's native libraries, then Gradle.
#
#   powershell -ExecutionPolicy Bypass -File tools/android/build-all.ps1 -Release
#
# -SkipDeps reuses an existing build/android-deps prefix (the deps only change
# when their pinned revisions in build-deps.ps1/build-glib.ps1 change).
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$depArgs = @()
if ($Ndk) { $depArgs += @('-Ndk', $Ndk) }
if ($Jobs -gt 0) { $depArgs += @('-Jobs', "$Jobs") }

if (-not $SkipDeps) {
    & (Join-Path $PSScriptRoot 'build-deps.ps1') @depArgs
    if ($LASTEXITCODE -ne 0) { throw 'build-deps.ps1 failed.' }
    & (Join-Path $PSScriptRoot 'build-glib.ps1') @depArgs
    if ($LASTEXITCODE -ne 0) { throw 'build-glib.ps1 failed.' }
} else {
    Write-Output 'Skipping the dependency build (-SkipDeps).'
}

$apkArgs = @()
if ($Ndk) { $apkArgs += @('-Ndk', $Ndk) }
if ($Gradle) { $apkArgs += @('-Gradle', $Gradle) }
if ($Jobs -gt 0) { $apkArgs += @('-Jobs', "$Jobs") }
if ($Fresh) { $apkArgs += '-Fresh' }
if ($Release) { $apkArgs += '-Release' }
if ($Install) { $apkArgs += '-Install' }
if ($Serial) { $apkArgs += @('-Serial', $Serial) }
& (Join-Path $PSScriptRoot 'build-apk.ps1') @apkArgs
if ($LASTEXITCODE -ne 0) { throw 'build-apk.ps1 failed.' }
