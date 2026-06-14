@echo off
rem Wrapper: load the MSVC environment so clangd can find the Windows SDK / STL
rem headers, then hand off to Zed's bundled clangd. Pointed at by .zed/settings.json.
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -prerelease -property installationPath`) do set "VSPATH=%%i"
if defined VSPATH call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
set "CLANGD="
for /d %%d in ("%LOCALAPPDATA%\Zed\languages\clangd\clangd_*") do set "CLANGD=%%d\bin\clangd.exe"
"%CLANGD%" %*
