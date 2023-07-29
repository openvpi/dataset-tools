# Data-set Tools

DiffSinger dataset processing tools, including audio processing, labeling.

## Applications

+ MinLabel
+ SlurCutter
+ AudioSlicer

## Supported Platforms

+ Microsoft Windows (Vista ~ 11)
+ Apple Mac OSX (11+)
+ Linux (Tested on Ubuntu)

## Build from source

### Requirements

| Component | Requirement |               Detailed               |
|:---------:|:-----------:|:------------------------------------:|
|    Qt     |   5.15.2    |   Core, Gui, Widgets, Svg, Network   |
| Compiler  |  \>=C++17   |        MSVC 2019, GCC, Clang         |
|   CMake   |   \>=3.17   |        >=3.20 is recommended         |
|  Python   |   \>=3.8    |                  /                   |

### Setup Environment

You need to install Qt 5.15.2 first

#### Windows

```sh
set QT_DIR=<dir> # directory `Qt5Config.cmake` locates
set Qt5_DIR=%QT_DIR%
set VCPKG_OVERLAY_PORTS=%cd%\scripts\vcpkg\ports
set VCPKG_OVERLAY_TRIPLETS=%cd%\scripts\vcpkg\triplets
set VCPKG_KEEP_ENV_VARS=QT_DIR;Qt5_DIR

git clone https://github.com/microsoft/vcpkg.git
cd /D vcpkg
bootstrap-vcpkg.bat

vcpkg install qastool:x64-windows
vcpkg install sdl2:x64-windows
vcpkg install ffmpeg-fake:x64-windows
vcpkg install sndfile:x64-windows
```

#### Linux

```sh
export QT_DIR=<dir> # directory `Qt5Config.cmake` locates
export Qt5_DIR=$QT_DIR
export VCPKG_OVERLAY_PORTS=$(pwd)/scripts/vcpkg/ports
export VCPKG_OVERLAY_TRIPLETS=$(pwd)/scripts/vcpkg/triplets
export VCPKG_KEEP_ENV_VARS=QT_DIR;Qt5_DIR

git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh

./vcpkg install qastool:x64-linux
./vcpkg install sdl2:x64-linux
./vcpkg install ffmpeg-fake:x64-linux
./vcpkg install sndfile:x64-linux

# On Mac: the triplet is `x64-osx` or `arm64-osx`
```

### Build & Install

```sh
cmake -B build -G Ninja \
    -DCMAKE_INSTALL_PREFIX=<dir> \ # install directory
    -DCMAKE_PREFIX_PATH=<dir> \ # directory `Qt5Config.cmake` locates
    -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build --target all

cmake --build build --target install
```

## Libraries

### Related Projects

+ [DiffSinger](https://github.com/openvpi/DiffSinger)
    + Apache 2.0 License

+ [ChorusKit](https://github.com/SineStriker/qsynthesis-revenge)
    + Apache 2.0 License

### Dependencies

+ [Qt 5.15.2](https://www.qt.io/)
    + GNU LGPL v2.1 or later
+ [FFmpeg](https://github.com/FFmpeg/FFmpeg) 
    + GNU LGPL v2.1 or later
+ [LAME](https://lame.sourceforge.io/)
    + GNU LGPL v2.0
+ [SDL](https://github.com/libsdl-org/SDL) (Audio)
    + Zlib License
+ [vcpkg](https://github.com/microsoft/vcpkg)
    + MIT License
+ [qastool](https://github.com/SineStriker/qt-json-autogen)
    + Apache 2.0 License