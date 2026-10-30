param(
    [ValidateSet('arm64-v8a', 'x86_64')][string]$Abi = 'arm64-v8a',
    [int]$Api = 30,
    [string]$Ndk = $env:NDK_ROOT,
    [string]$Serial,
    [switch]$Run
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
if (-not $Ndk) { throw 'Set NDK_ROOT or pass -Ndk with an installed Android NDK path.' }
$toolchain = Join-Path $Ndk 'build/cmake/android.toolchain.cmake'
if (-not (Test-Path -LiteralPath $toolchain)) { throw "NDK toolchain not found: $toolchain" }
$build = Join-Path $root "build/android-probes/$Abi"

function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit code $LASTEXITCODE" }
}

Invoke-Checked cmake @('-Wno-deprecated', '-S', $PSScriptRoot, '-B', $build,
    '-G', 'Ninja', "-DCMAKE_TOOLCHAIN_FILE=$toolchain", "-DANDROID_ABI=$Abi",
    "-DANDROID_PLATFORM=android-$Api", '-DCMAKE_BUILD_TYPE=RelWithDebInfo')
Invoke-Checked cmake @('--build', $build, '-j', '6')
if (-not $Run) { return }
if (-not $Serial) { throw 'Pass an explicit -Serial to choose the intended test device.' }
Invoke-Checked adb @('-s', $Serial, 'get-state')
$deviceAbi = (& adb -s $Serial shell getprop ro.product.cpu.abi).Trim()
if ($LASTEXITCODE -ne 0 -or $deviceAbi -ne $Abi) {
    throw "Expected $Abi device; $Serial reports $deviceAbi"
}
$remote = '/data/local/tmp/ngb-port-tests'
Invoke-Checked adb @('-s', $Serial, 'shell', 'mkdir', '-p', $remote)
$tests = @('veh_posix_test', 'vcpu_posix_test', 'thread_suspend_posix_test',
    'kernel_memory_posix_test', 'memory_layout_posix_test')
if (Test-Path -LiteralPath (Join-Path $build 'fiber_context_test')) { $tests += 'fiber_context_test' }
foreach ($name in @($tests) + @('vulkan_caps')) {
    Invoke-Checked adb @('-s', $Serial, 'push', (Join-Path $build $name), "$remote/$name")
    Invoke-Checked adb @('-s', $Serial, 'shell', 'chmod', '700', "$remote/$name")
}
Invoke-Checked adb @('-s', $Serial, 'push',
    (Join-Path $root 'xboxrecomp/tools/conformance/test.xbe'), "$remote/test.xbe")
$report = Join-Path $build ("device-report-{0}.txt" -f (Get-Date -Format 'yyyyMMdd-HHmmss'))
$failures = @()
foreach ($name in @($tests) + @('vulkan_caps')) {
    $command = @('-s', $Serial, 'shell', 'timeout', '30', "$remote/$name")
    if ($name -eq 'memory_layout_posix_test') { $command += "$remote/test.xbe" }
    $output = & adb @command 2>&1
    $result = $LASTEXITCODE
    "== $name (exit $result)" | Tee-Object -FilePath $report -Append
    $output | Tee-Object -FilePath $report -Append
    if ($result -ne 0) { $failures += $name }
}
Write-Output "Report: $report"
if ($failures.Count) { throw "Failed probes: $($failures -join ', ')" }
