@echo off
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "BASE_DIR=%SCRIPT_DIR%.."
set "RUNNER_PATH=%BASE_DIR%\runners\windows\NexDCTLRunner.ofx"

if not exist "%RUNNER_PATH%" (
    echo [ERROR] Windows runner binary not found at:
    echo "%RUNNER_PATH%"
    pause
    exit /b 1
)

:: Locate nexkeyctl
set "NEXKEYCTL="
where nexkeyctl >nul 2>nul
if %errorlevel% equ 0 (
    set "NEXKEYCTL=nexkeyctl"
) else if exist "%BASE_DIR%\bin\nexkeyctl.exe" (
    set "NEXKEYCTL=%BASE_DIR%\bin\nexkeyctl.exe"
) else if exist "%BASE_DIR%\..\..\tools\nexkeyctl\build\Release\nexkeyctl.exe" (
    set "NEXKEYCTL=%BASE_DIR%\..\..\tools\nexkeyctl\build\Release\nexkeyctl.exe"
) else (
    echo [ERROR] nexkeyctl.exe not found in PATH or bin directory.
    echo Please ensure nexkeyctl.exe is available.
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
if "%OUTPUT_DIR%"=="" set "OUTPUT_DIR=%BASE_DIR%\dist"

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

