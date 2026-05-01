@echo off
setlocal

rem ============================================================
rem  dataset-tools Debug 编译脚本
rem  用法: 双击运行，或在命令行执行 scripts\build-debug.bat
rem  前置条件: vcpkg 依赖已安装，ORT 已下载 (见 docs/local-build.md)
rem ============================================================

rem --- 路径配置 (按本机环境修改) ---
set VS_PATH=D:\Programs\vs2026
set QT_DIR=D:\Programs\qt5\6.10.0\msvc2022_64
set CMAKE_EXE=D:\Program Files\CLion\bin\cmake\win\x64\bin\cmake.exe
set NINJA_EXE=D:\Program Files\CLion\bin\ninja\win\x64\ninja.exe

rem --- 固定配置 ---
set Qt6_DIR=%QT_DIR%\lib\cmake\Qt6
set VCPKG_KEEP_ENV_VARS=QT_DIR;Qt6_DIR
set BUILD_DIR=cmake-build-debug
set BUILD_TYPE=Debug
set JOBS=10

rem --- 项目根目录 (脚本所在目录的上一级) ---
cd /d "%~dp0.."
set PROJECT_DIR=%CD%
set TOOLCHAIN=%PROJECT_DIR%\vcpkg\scripts\buildsystems\vcpkg.cmake

echo ============================================================
echo  dataset-tools %BUILD_TYPE% Build
echo  Project: %PROJECT_DIR%
echo  Qt:      %QT_DIR%
echo  VS:      %VS_PATH%
echo  Output:  %BUILD_DIR%
echo ============================================================

rem --- 加载 MSVC 环境 ---
echo.
echo [1/3] Loading MSVC environment...
call "%VS_PATH%\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
if errorlevel 1 (
    echo ERROR: Failed to load MSVC environment from %VS_PATH%
    pause
    exit /b 1
)

rem --- CMake Configure ---
echo.
echo [2/3] CMake Configure...
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

rem --- Build ---
echo.
echo [3/3] Building (j=%JOBS%)...
"%CMAKE_EXE%" --build "%BUILD_DIR%" --target all -j %JOBS%
if errorlevel 1 (
    echo.
    echo ERROR: Build failed.
    pause
    exit /b 1
)

echo.
echo ============================================================
echo  BUILD SUCCESSFUL
echo  Binaries: %BUILD_DIR%\bin\
echo ============================================================
pause
