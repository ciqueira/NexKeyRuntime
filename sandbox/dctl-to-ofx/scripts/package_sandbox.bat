@echo off
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "BASE_DIR=%%~fI"
for %%I in ("%SCRIPT_DIR%..\..\..") do set "REPO_ROOT=%%~fI"
set "RUNNER_PATH=%BASE_DIR%\runners\windows\NexDCTLRunner.ofx"

if not exist "%RUNNER_PATH%" (
    echo [ERROR] Windows runner binary not found at:
    echo "%RUNNER_PATH%"
    pause
    exit /b 1
)

:: Locate nexkeyctl (prefer local build in this clone before PATH)
set "NEXKEYCTL="
if exist "%REPO_ROOT%\build\bin\Release\nexkeyctl.exe" (
    set "NEXKEYCTL=%REPO_ROOT%\build\bin\Release\nexkeyctl.exe"
) else if exist "%REPO_ROOT%\build\bin\nexkeyctl.exe" (
    set "NEXKEYCTL=%REPO_ROOT%\build\bin\nexkeyctl.exe"
) else if exist "%REPO_ROOT%\tools\nexkeyctl\build\Release\nexkeyctl.exe" (
    set "NEXKEYCTL=%REPO_ROOT%\tools\nexkeyctl\build\Release\nexkeyctl.exe"
) else if exist "%REPO_ROOT%\tools\nexkeyctl\build\nexkeyctl.exe" (
    set "NEXKEYCTL=%REPO_ROOT%\tools\nexkeyctl\build\nexkeyctl.exe"
) else if exist "%REPO_ROOT%\bin\nexkeyctl.exe" (
    set "NEXKEYCTL=%REPO_ROOT%\bin\nexkeyctl.exe"
) else if exist "%BASE_DIR%\bin\nexkeyctl.exe" (
    set "NEXKEYCTL=%BASE_DIR%\bin\nexkeyctl.exe"
) else (
    where nexkeyctl >nul 2>nul
    if !errorlevel! equ 0 (
        set "NEXKEYCTL=nexkeyctl"
    )
)

if "%NEXKEYCTL%"=="" (
    echo [ERROR] nexkeyctl.exe not found.
    echo Build it from the repository root first:
    echo   cmake -S . -B build -A x64 ^&^& cmake --build build --config Release
    pause
    exit /b 1
)

set "INPUT_FILE=%~1"
if "%INPUT_FILE%"=="" (
    echo ========================================================
    echo  NexKeyRuntime DCTL-to-OFX Sandbox Packager (Windows)
    echo ========================================================
    set /p "INPUT_FILE=Enter path to .dctl file (or drag and drop here): "
    :: Strip enclosing quotes
    set "INPUT_FILE=!INPUT_FILE:"=!"
)

if not exist "%INPUT_FILE%" (
    echo [ERROR] Input file does not exist: "%INPUT_FILE%"
    pause
    exit /b 1
)

set "OUTPUT_DIR=%~2"
if "%OUTPUT_DIR%"=="" set "OUTPUT_DIR=%REPO_ROOT%\dist"

if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

echo [PACKAGING] "%INPUT_FILE%" into "%OUTPUT_DIR%"...
"%NEXKEYCTL%" package-dctl --input="%INPUT_FILE%" --runner="%RUNNER_PATH%" --output="%OUTPUT_DIR%" --sandbox

if %errorlevel% equ 0 (
    echo.
    echo [SUCCESS] OpenFX bundle generated in: "%OUTPUT_DIR%"
    echo To install in DaVinci Resolve, copy the .ofx.bundle to:
    echo   C:\Program Files\Common Files\OFX\Plugins\
) else (
    echo.
    echo [ERROR] Packaging failed with exit code %errorlevel%.
)

echo.
pause
