@echo off
setlocal

rem ============================================================
rem  dataset-tools 清理重建脚本
rem  删除 cmake-build-release 后重新 Configure + Build
rem ============================================================

rem --- 路径配置 (按本机环境修改) ---
set VS_PATH=D:\Programs\vs2026
set QT_DIR=D:\Programs\qt5\6.10.0\msvc2022_64
set CMAKE_EXE=D:\Program Files\CLion\bin\cmake\win\x64\bin\cmake.exe
set NINJA_EXE=D:\Program Files\CLion\bin\ninja\win\x64\ninja.exe

rem --- 固定配置 ---
set Qt6_DIR=%QT_DIR%\lib\cmake\Qt6
set VCPKG_KEEP_ENV_VARS=QT_DIR;Qt6_DIR
set BUILD_DIR=cmake-build-release
set BUILD_TYPE=Release
set JOBS=10

cd /d "%~dp0.."
set PROJECT_DIR=%CD%
set TOOLCHAIN=%PROJECT_DIR%\vcpkg\scripts\buildsystems\vcpkg.cmake

echo ============================================================
echo  CLEAN REBUILD - %BUILD_TYPE%
echo  Deleting: %BUILD_DIR%
echo ============================================================

if exist "%BUILD_DIR%" (
    echo Removing %BUILD_DIR%...
    rd /s /q "%BUILD_DIR%"
)

echo.
echo Loading MSVC environment...
call "%VS_PATH%\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
if errorlevel 1 (
    echo ERROR: Failed to load MSVC environment.
    pause
    exit /b 1
)

echo.
echo CMake Configure...
"%CMAKE_EXE%" -B "%BUILD_DIR%" -G Ninja ^
    -DCMAKE_TOOLCHAIN_FILE="%TOOLCHAIN%" ^
    -DCMAKE_PREFIX_PATH="%QT_DIR%" ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DONNXRUNTIME_ENABLE_DML=ON ^
    -DCMAKE_MAKE_PROGRAM="%NINJA_EXE%"
if errorlevel 1 (
    echo ERROR: CMake configure failed.
    pause
    exit /b 1
)

echo.
echo Building (j=%JOBS%)...
"%CMAKE_EXE%" --build "%BUILD_DIR%" --target all -j %JOBS%
if errorlevel 1 (
    echo ERROR: Build failed.
    pause
    exit /b 1
)

echo.
echo ============================================================
echo  CLEAN REBUILD SUCCESSFUL
echo  Binaries: %BUILD_DIR%\bin\
echo ============================================================
pause
