@echo off
call "D:\Programs\vs2026\VC\Auxiliary\Build\vcvarsall.bat" x64

set QT_DIR=D:\Programs\qt5\6.10.0\msvc2022_64
set Qt6_DIR=%QT_DIR%
set VCPKG_KEEP_ENV_VARS=QT_DIR;Qt6_DIR

echo === Starting build ===
cmake --build build-vs --config Release
echo === Build exit code: %ERRORLEVEL% ===
