@echo off
call "D:\Programs\vs2026\VC\Auxiliary\Build\vcvarsall.bat" x64

set QT_DIR=D:\Programs\qt5\6.10.0\msvc2022_64
set Qt6_DIR=%QT_DIR%
set VCPKG_KEEP_ENV_VARS=QT_DIR;Qt6_DIR

cmake -B cmake-build-release -G Ninja ^
    -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake ^
    -DCMAKE_PREFIX_PATH=%QT_DIR% ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DONNXRUNTIME_ENABLE_DML=ON ^
    -DCMAKE_MAKE_PROGRAM="D:\Program Files\CLion\bin\ninja\win\x64\ninja.exe"

cmake --build cmake-build-release -j 10
