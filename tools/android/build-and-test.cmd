@echo off
rem Build the release APK, install it on the connected handheld and dump the
rem launch log. Double-click this when the agent cannot run commands.
setlocal
cd /d "%~dp0..\.."

set "ADB=%LOCALAPPDATA%\Android\Sdk\platform-tools\adb.exe"
set "PKG=org.ngb.port"
set "LOG=build\logs\device-test.log"

if not exist "build\logs" mkdir "build\logs"

echo === building and installing ===
powershell -ExecutionPolicy Bypass -NoProfile -File "tools\android\build-apk.ps1" -Release -Install %*
if errorlevel 1 goto fail

echo.
echo === launching %PKG% ===
"%ADB%" shell am force-stop %PKG%
"%ADB%" logcat -c
"%ADB%" shell monkey -p %PKG% -c android.intent.category.LAUNCHER 1 >nul 2>&1
echo waiting 10s for the first frames...
timeout /t 10 /nobreak >nul

"%ADB%" logcat -d > "%LOG%" 2>&1
findstr /i /c:"FATAL" /c:"SIGSEGV" /c:"ANR in" "%LOG%" >nul 2>&1
if errorlevel 1 (echo no crash in the first 10s) else (echo CRASH DETECTED - see %LOG%)

echo.
echo log: %CD%\%LOG%
echo.
pause
goto :eof

:fail
echo.
echo BUILD FAILED - see the output above.
pause
