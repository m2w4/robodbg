@echo off
setlocal

:: Default architecture
set ARCH=

:: Check parameter
if "%~1"=="" (
    echo Usage: build.bat [x86^|x64]
    exit /b 1
)

if /I "%~1"=="x86" (
    set ARCH=-A Win32
) else if /I "%~1"=="x64" (
    set ARCH=
) else (
    echo Invalid parameter: %~1
    echo Use x86 or x64
    exit /b 1
)

:: Configure
cmake -S . -B build %ARCH% ^
    -DROBO_BUILD_PYTHON=ON ^
    -DROBO_BUILD_EXAMPLES=ON ^
    -DROBO_BUILD_TESTS=ON ^
    -DROBO_BUILD_DOCS=ON

:: Build (choose Release for distribution)
cmake --build build --config Release

endlocal
