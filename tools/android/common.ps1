# Shared helpers for the Android dependency scripts (build-deps.ps1, build-glib.ps1).
# They fill build/android-deps, the prefix packaging/android/build.sh builds against.
#
# The project is checked out on different machines and different drives, so no
# script may hard-code where the NDK, Gradle or the JDK live, and no build tree
# may survive the project moving: CMake, Meson and Ninja all record absolute
# paths, and a build directory configured for another path fails or silently
# writes into the old location. These helpers find the tools and discard build
# trees that belong somewhere else.

function Resolve-AndroidNdk([string]$Requested) {
    $candidates = New-Object 'System.Collections.Generic.List[string]'
    if ($Requested) { $candidates.Add($Requested) }
    if ($env:NDK_ROOT) { $candidates.Add($env:NDK_ROOT) }
    foreach ($sdk in @($env:ANDROID_SDK_ROOT, $env:ANDROID_HOME)) {
        if (-not $sdk) { continue }
        $ndkRoot = Join-Path $sdk 'ndk'
        if (-not (Test-Path -LiteralPath $ndkRoot)) { continue }
        Get-ChildItem -LiteralPath $ndkRoot -Directory -ErrorAction SilentlyContinue |
            Sort-Object Name -Descending |
            ForEach-Object { $candidates.Add($_.FullName) }
    }
    foreach ($candidate in $candidates) {
        if (-not $candidate) { continue }
        $toolchain = Join-Path $candidate 'build/cmake/android.toolchain.cmake'
        if (Test-Path -LiteralPath $toolchain) { return (Resolve-Path -LiteralPath $candidate).Path }
    }
    throw 'No Android NDK found: pass -Ndk, set NDK_ROOT, or install one under $ANDROID_SDK_ROOT\ndk.'
}

function Resolve-CMake {
    $command = Get-Command cmake -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }
    foreach ($sdk in @($env:ANDROID_SDK_ROOT, $env:ANDROID_HOME)) {
        if (-not $sdk) { continue }
        $bundled = Join-Path $sdk 'cmake'
        if (-not (Test-Path -LiteralPath $bundled)) { continue }
        $found = Get-ChildItem -LiteralPath $bundled -Directory -ErrorAction SilentlyContinue |
            Sort-Object Name -Descending |
            ForEach-Object { Join-Path $_.FullName 'bin/cmake.exe' } |
            Where-Object { Test-Path -LiteralPath $_ } |
            Select-Object -First 1
        if ($found) { return $found }
    }
    throw 'cmake not found: add it to PATH or install the SDK CMake package.'
}

function Resolve-Gradle([string]$Requested) {
    if ($Requested) {
        if (-not (Test-Path -LiteralPath $Requested)) { throw "Gradle not found: $Requested" }
        return (Resolve-Path -LiteralPath $Requested).Path
    }
    $command = Get-Command gradle -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }
    $cache = Join-Path $env:USERPROFILE '.gradle/wrapper/dists'
    if (-not (Test-Path -LiteralPath $cache)) { throw 'Gradle not found: pass -Gradle.' }
    # Prefer the version the build was developed against, then any wrapper dist.
    $preferred = Join-Path $cache 'gradle-8.13-bin'
    $roots = @()
    if (Test-Path -LiteralPath $preferred) { $roots += $preferred }
    $roots += (Get-ChildItem -LiteralPath $cache -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -ne $preferred } |
        Sort-Object Name -Descending |
        ForEach-Object { $_.FullName })
    foreach ($root in $roots) {
        $bat = Get-ChildItem -LiteralPath $root -Filter gradle.bat -Recurse -ErrorAction SilentlyContinue |
            Select-Object -First 1
        if ($bat) { return $bat.FullName }
    }
    throw 'Gradle not found: pass -Gradle with Gradle 8.13, or add it to PATH.'
}

function Resolve-JavaHome {
    if ($env:JAVA_HOME) {
        $java = Join-Path $env:JAVA_HOME 'bin/java.exe'
        if (Test-Path -LiteralPath $java) { return $env:JAVA_HOME }
    }
    $candidates = @(
        (Join-Path $env:ProgramFiles 'Android/Android Studio/jbr'),
        (Join-Path $env:LOCALAPPDATA 'Programs/Android Studio/jbr'),
        (Join-Path $env:ProgramFiles 'Android/Android Studio/jre'),
        (Join-Path $env:USERPROFILE '.gradle/jdks')
    )
    foreach ($candidate in $candidates) {
        if (-not $candidate) { continue }
        if (Test-Path -LiteralPath (Join-Path $candidate 'bin/java.exe')) { return $candidate }
        if (Test-Path -LiteralPath $candidate) {
            # ~/.gradle/jdks holds one directory per downloaded toolchain.
            $java = Get-ChildItem -LiteralPath $candidate -Directory -ErrorAction SilentlyContinue |
                Sort-Object Name -Descending |
                ForEach-Object { Join-Path $_.FullName 'bin/java.exe' } |
                Where-Object { Test-Path -LiteralPath $_ } |
                Select-Object -First 1
            if ($java) { return (Split-Path -Parent (Split-Path -Parent $java)) }
        }
    }
    return $null
}

function Resolve-Adb {
    # adb ships with the SDK but is rarely on PATH, so look next to the NDK.
    $command = Get-Command adb -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }
    foreach ($sdk in @($env:ANDROID_SDK_ROOT, $env:ANDROID_HOME)) {
        if (-not $sdk) { continue }
        $adb = Join-Path $sdk 'platform-tools/adb.exe'
        if (Test-Path -LiteralPath $adb) { return $adb }
    }
    throw 'adb not found: add the SDK platform-tools to PATH or set ANDROID_SDK_ROOT.'
}

function Get-OnlineAndroidDevices([string]$Adb) {
    # adb chats on stderr ("daemon not running..."), and with
    # $ErrorActionPreference = 'Stop' PowerShell turns that into a terminating
    # NativeCommandError; the scripts that call this must not die over it.
    $previous = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $lines = @(& $Adb devices 2>&1 | ForEach-Object { "$_" })
    } finally {
        $ErrorActionPreference = $previous
    }
    # Only devices in the 'device' state; 'unauthorized' and 'offline' are noise.
    return @($lines | Where-Object { $_ -match "\sdevice$" } |
        ForEach-Object { ($_ -split '\s+')[0] } | Where-Object { $_ })
}

function Resolve-PkgConfig {
    # Meson needs a pkg-config implementation; the Android cross file names it
    # explicitly. Toolchains ship it under either spelling.
    foreach ($name in @('pkgconf', 'pkg-config')) {
        $command = Get-Command $name -ErrorAction SilentlyContinue
        if ($command) { return $command.Source }
    }
    $candidates = @(
        'C:/w64devkit/w64devkit/bin/pkg-config.exe',
        'C:/msys64/usr/bin/pkgconf.exe',
        'C:/msys64/usr/bin/pkg-config.exe'
    )
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) { return $candidate }
    }
    throw 'pkg-config not found: install pkgconf or pkg-config and put it on PATH.'
}

function Get-SourcePathFlags([string]$Source) {
    # Binaries must not record where the source happened to live: assert() and
    # __FILE__ would otherwise embed E:/.../ref-ngb/... for ever. Map the
    # checkout root to '.' so the strings become "./src/...".
    $slashed = $Source.Replace('\', '/')
    return "-ffile-prefix-map=$slashed=. -fdebug-prefix-map=$slashed=."
}

function Test-CMakeCacheElsewhere([string]$Dir, [string]$Source) {
    # $true when $Dir holds a CMake cache configured for a different source tree.
    $cache = Join-Path $Dir 'CMakeCache.txt'
    if (-not (Test-Path -LiteralPath $cache)) { return $false }
    $recorded = $null
    foreach ($line in Get-Content -LiteralPath $cache -ErrorAction SilentlyContinue) {
        if ($line.StartsWith('CMAKE_HOME_DIRECTORY:INTERNAL=')) {
            $recorded = $line.Substring('CMAKE_HOME_DIRECTORY:INTERNAL='.Length).Trim()
            break
        }
    }
    if (-not $recorded) { return $false }
    $want = (Resolve-Path -LiteralPath $Source).Path
    return (($recorded -replace '\\', '/').TrimEnd('/') -ne ($want -replace '\\', '/').TrimEnd('/'))
}

function Reset-StaleCMakeBuild([string]$Dir, [string]$Source) {
    if (-not (Test-CMakeCacheElsewhere $Dir $Source)) { return $false }
    $recorded = (Select-String -LiteralPath (Join-Path $Dir 'CMakeCache.txt') `
        -Pattern '^CMAKE_HOME_DIRECTORY:INTERNAL=(.+)$').Matches[0].Groups[1].Value
    Write-Output "Discarding stale build tree $Dir (configured for $recorded)"
    Remove-Item -LiteralPath $Dir -Recurse -Force
    return $true
}

function Reset-StaleMesonBuild([string]$Dir, [string]$Source) {
    # Meson records absolute paths as well; compare the source it was set up on.
    $coredata = Join-Path $Dir 'meson-private/coredata.dat'
    if (-not (Test-Path -LiteralPath $coredata)) { return $false }
    $intro = Join-Path $Dir 'meson-info/intro-projectinfo.json'
    $recorded = $null
    if (Test-Path -LiteralPath $intro) {
        try { $recorded = (Get-Content -LiteralPath $intro -Raw | ConvertFrom-Json).subproject_dir } catch { }
    }
    $expected = (Resolve-Path -LiteralPath $Source).Path
    $moved = $false
    if ($recorded -eq $null) {
        # No introspection data to compare: check that the recorded build dir is ours.
        $raw = [System.IO.File]::ReadAllText($coredata, [System.Text.Encoding]::UTF8)
        $moved = -not $raw.Contains($Dir.Replace('\', '/')) -and -not $raw.Contains($Dir)
    }
    if ($moved) {
        Write-Output "Discarding stale Meson build tree $Dir"
        Remove-Item -LiteralPath $Dir -Recurse -Force
        return $true
    }
    return $false
}

function Get-BuildJobs([int]$Requested) {
    if ($Requested -gt 0) { return $Requested }
    $cores = 0
    if ($env:NUMBER_OF_PROCESSORS) { $cores = [int]$env:NUMBER_OF_PROCESSORS }
    if ($cores -le 0) { $cores = 4 }
    $ramGb = 0
    try {
        $ramGb = [math]::Round((Get-CimInstance Win32_ComputerSystem -ErrorAction Stop).TotalPhysicalMemory / 1GB)
    } catch { }
    # SPIRV-Tools feeds >1.5 GB through a single translation unit; on a small
    # machine a wide fan-out gets the compiler killed instead of failing loudly.
    $cap = 6
    if ($ramGb -gt 0 -and $ramGb -lt 16) { $cap = 3 }
    # The lifted game sources are ~2 MB of C each; keep a cap so a wide machine
    # does not run out of memory while eight of them compile at once.
    return [Math]::Min($cores, $cap)
}
