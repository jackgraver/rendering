@echo off
setlocal enabledelayedexpansion

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [build] Could not find vswhere.exe. Is Visual Studio installed?
    exit /b 1
)

for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -prerelease -property installationPath`) do set "VSPATH=%%i"
if not defined VSPATH (
    echo [build] Could not locate a Visual Studio installation.
    exit /b 1
)

call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 (
    echo [build] Failed to initialize the MSVC environment.
    exit /b 1
)

if not exist "build\CMakeCache.txt" (
    cmake --preset ninja-debug || exit /b 1
)

cmake --build --preset ninja-debug || exit /b 1

if /i "%~1"=="run" (
    echo [build] Running OpenGL.exe ...
    build\OpenGL.exe
)
