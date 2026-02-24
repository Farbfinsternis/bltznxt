@echo off
setlocal enabledelayedexpansion

REM --- Configuration ---
set "ROOT_DIR=%~dp0"
set "TOOLS_DIR=%ROOT_DIR%tools"
set "LIBS_DIR=%ROOT_DIR%libs"

echo [INFO] BlitzNext Toolchain Setup
echo [INFO] Root: %ROOT_DIR%

if not exist "%TOOLS_DIR%" mkdir "%TOOLS_DIR%"
if not exist "%LIBS_DIR%" mkdir "%LIBS_DIR%"

REM --- Argument Parsing ---
set "SETUP_ONLY=0"
if "%1"=="--setup-only" set "SETUP_ONLY=1"

REM --- 1. MinGW-w64 ---
set "MINGW_DIR=%TOOLS_DIR%\mingw64"
set "MINGW_URL=https://github.com/brechtsanders/winlibs_mingw/releases/download/14.2.0posix-19.1.7-12.0.0-msvcrt-r3/winlibs-x86_64-posix-seh-gcc-14.2.0-llvm-19.1.7-mingw-w64msvcrt-12.0.0-r3.zip"
set "MINGW_ZIP=%TOOLS_DIR%\mingw.zip"

if exist "%MINGW_DIR%" (
    echo [INFO] MinGW found at %MINGW_DIR%
    goto CMAKE_CHECK
)

echo [INFO] MinGW not found. Downloading (this may take a while)...
curl -L "%MINGW_URL%" -o "%MINGW_ZIP%"
if %ERRORLEVEL% neq 0 (
    echo [ERROR] MinGW download failed.
    exit /b 1
)
echo [INFO] Extracting MinGW...
tar -xf "%MINGW_ZIP%" -C "%TOOLS_DIR%"
if exist "%MINGW_ZIP%" del "%MINGW_ZIP%"
echo [INFO] MinGW setup complete.

:CMAKE_CHECK
REM --- 2. CMake ---
set "CMAKE_DIR=%TOOLS_DIR%\cmake"
set "CMAKE_URL=https://github.com/Kitware/CMake/releases/download/v3.31.5/cmake-3.31.5-windows-x86_64.zip"
set "CMAKE_ZIP=%TOOLS_DIR%\cmake.zip"

if exist "%CMAKE_DIR%" (
    echo [INFO] CMake found at %CMAKE_DIR%
    goto JSON_CHECK
)

echo [INFO] CMake not found. Downloading...
curl -L "%CMAKE_URL%" -o "%CMAKE_ZIP%"
echo [INFO] Extracting CMake...
tar -xf "%CMAKE_ZIP%" -C "%TOOLS_DIR%"
for /d %%i in ("%TOOLS_DIR%\cmake-3.31.5*") do move "%%i" "%CMAKE_DIR%"
if exist "%CMAKE_ZIP%" del "%CMAKE_ZIP%"
echo [INFO] CMake setup complete.

:JSON_CHECK
REM --- 3. nlohmann/json ---
set "JSON_DIR=%TOOLS_DIR%\nlohmann"
set "JSON_URL=https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp"

if exist "%JSON_DIR%\json.hpp" (
    echo [INFO] nlohmann/json found at %JSON_DIR%
    goto SDL_CHECK
)

echo [INFO] nlohmann/json not found. Downloading...
if not exist "%JSON_DIR%" mkdir "%JSON_DIR%"
curl -L "%JSON_URL%" -o "%JSON_DIR%\json.hpp"
echo [INFO] nlohmann/json setup complete.

:SDL_CHECK
REM --- 4. SDL3 ---
set "SDL_DIR=%LIBS_DIR%\sd3"
set "SDL_URL=https://github.com/libsdl-org/SDL/releases/download/release-3.2.0/SDL3-devel-3.2.0-mingw.zip"
set "SDL_ZIP=%LIBS_DIR%\sdl3.zip"

if exist "%SDL_DIR%" (
    echo [INFO] SDL3 found at %SDL_DIR%
    goto SDL_TTF_CHECK
)

echo [INFO] SDL3 not found. Downloading...
curl -L "%SDL_URL%" -o "%SDL_ZIP%"
echo [INFO] Extracting SDL3...
tar -xf "%SDL_ZIP%" -C "%LIBS_DIR%"
for /d %%i in ("%LIBS_DIR%\SDL3-*") do move "%%i" "%SDL_DIR%"
if exist "%SDL_ZIP%" del "%SDL_ZIP%"
echo [INFO] SDL3 setup complete.

:SDL_TTF_CHECK
REM --- 5. SDL3_ttf ---
set "SDL_TTF_DIR=%LIBS_DIR%\sdl3_ttf"
set "SDL_TTF_URL=https://github.com/libsdl-org/SDL_ttf/releases/download/release-3.2.2/SDL3_ttf-devel-3.2.2-mingw.tar.gz"
set "SDL_TTF_TAR=%LIBS_DIR%\sdl3_ttf.tar.gz"

if exist "%SDL_TTF_DIR%" (
    echo [INFO] SDL3_ttf found at %SDL_TTF_DIR%
    goto BUILD_START
)

echo [INFO] SDL3_ttf not found. Downloading...
curl -L "%SDL_TTF_URL%" -o "%SDL_TTF_TAR%"
echo [INFO] Extracting SDL3_ttf...
tar -xzf "%SDL_TTF_TAR%" -C "%LIBS_DIR%"
for /d %%i in ("%LIBS_DIR%\SDL3_ttf-*") do move "%%i" "%SDL_TTF_DIR%"
if exist "%SDL_TTF_TAR%" del "%SDL_TTF_TAR%"
echo [INFO] SDL3_ttf setup complete.

:BUILD_START
if "%SETUP_ONLY%"=="1" (
    echo [INFO] Setup complete. Skipping build as requested.
    exit /b 0
)

echo [INFO] Starting BlitzNext build...
set "PATH=%MINGW_DIR%\bin;%CMAKE_DIR%\bin;%PATH%"

if not exist build mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DSDL3_DIR="%SDL_DIR%/x86_64-w64-mingw32" -DJSON_INCLUDE_DIR="%TOOLS_DIR%"
mingw32-make
cd ..

echo [INFO] Build finished.
