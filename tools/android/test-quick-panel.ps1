$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $true
$taskRepo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$taskBuild = Join-Path $taskRepo 'build/secondary-check'
New-Item -ItemType Directory -Force -Path $taskBuild | Out-Null
& gcc -std=c11 -Wall -Wextra -Werror "-I$taskRepo/src" "$taskRepo/src/quick_panel.c" "$PSScriptRoot/tests/quick_panel_test.c" -o "$taskBuild/quick-panel-test.exe"
& "$taskBuild/quick-panel-test.exe"
$taskMapSource = Get-Content -LiteralPath "$taskRepo/src/quick_select.c" -Raw -Encoding UTF8
$taskArtSource = Get-Content -LiteralPath "$taskRepo/src/quick_ui.c" -Raw -Encoding UTF8
$taskMapParts = @([regex]::Match($taskMapSource, '(?s)static void make_map\(.*?\n\}').Value)
$taskMapParts += [regex]::Match($taskArtSource, '(?s)static uint32_t u32\(.*?\n\}').Value
$taskMapParts += [regex]::Match($taskArtSource, '(?s)typedef struct\s*\{\s*char name\[24\];.*?static int s_icon_count;').Value
$taskMapParts += [regex]::Match($taskArtSource, '(?s)static unsigned spread\(.*?\n\}').Value
$taskMapParts += [regex]::Match($taskArtSource, '(?s)static Icon \*load_icon\(.*?\n\}').Value
$taskMapParts += [regex]::Match($taskArtSource, '(?s)int ngb_quick_asset_name\(.*?\n\}').Value
if ($taskMapParts.Count -ne 6 -or @($taskMapParts | Where-Object { -not $_ }).Count) { throw 'Map test source extraction failed' }
Set-Content -LiteralPath "$taskBuild/quick_map_under_test.h" -Encoding UTF8 -Value ($taskMapParts -join "`n")
& gcc -std=c11 -Wall -Wextra -Werror "-I$taskRepo/src" "-I$taskBuild" "$PSScriptRoot/tests/quick_map_test.c" -o "$taskBuild/quick-map-test.exe" -lm
& "$taskBuild/quick-map-test.exe"
& gcc -std=c11 -Wall -Wextra -Werror "-I$taskRepo/src" "$taskRepo/src/costume_state.c" "$PSScriptRoot/tests/costume_state_test.c" -o "$taskBuild/costume-state-test.exe"
& "$taskBuild/costume-state-test.exe"
& gcc -std=c11 -Wall -Wextra -Werror -Wno-unused-variable -Wno-unused-label -Wno-unused-but-set-variable "-I$PSScriptRoot/tests/costume_stub" "-I$taskRepo/src" "$taskRepo/src/costume_switch.c" "$taskRepo/src/costume_state.c" "$PSScriptRoot/tests/costume_loader_test.c" -o "$taskBuild/costume-loader-test.exe"
& "$taskBuild/costume-loader-test.exe"
$taskJava = Get-Content -LiteralPath "$taskRepo/packaging/android/java/io/github/gamboozino/ngbrecomp/NgbActivity.java" -Raw -Encoding UTF8
$taskModel = $taskJava.Substring($taskJava.IndexOf('final class QuickPanelState'))
Set-Content -LiteralPath "$taskBuild/QuickPanelModel.java" -Encoding UTF8 -Value ("package io.github.gamboozino.ngbrecomp;`nimport java.util.*;`n" + $taskModel)
& "$env:JAVA_HOME/bin/javac.exe" -encoding UTF-8 -d "$taskBuild/host-classes" "$taskBuild/QuickPanelModel.java" "$PSScriptRoot/tests/QuickPanelTest.java"
& "$env:JAVA_HOME/bin/java.exe" -cp "$taskBuild/host-classes" io.github.gamboozino.ngbrecomp.QuickPanelTest
