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
|    Qt     |  \>=5.15.2  |   Core, Gui, Widgets, Svg, Network   |
| Compiler  |  \>=C++17   |        MSVC 2019, GCC, Clang         |
|   CMake   |   \>=3.17   |        >=3.20 is recommended         |
<!-- |  Python   |   \>=3.8    |                  /                   | -->

### Setup Environment

You need to install Qt libraries first. (Tested on Qt 5.15.2 only)

#### Windows

```sh
set QT_DIR=<dir> # directory `Qt5Config.cmake` locates
set Qt5_DIR=%QT_DIR%
set VCPKG_KEEP_ENV_VARS=QT_DIR;Qt5_DIR

git clone https://github.com/microsoft/vcpkg.git
cd /D vcpkg
bootstrap-vcpkg.bat

vcpkg install ^
    --x-manifest-root=../scripts/vcpkg-manifest ^
    --x-install-root=./installed ^
    --triplet=x64-windows
```

#### Unix

```sh
export QT_DIR=<dir> # directory `Qt5Config.cmake` locates
export Qt5_DIR=$QT_DIR
export VCPKG_KEEP_ENV_VARS=QT_DIR;Qt5_DIR

git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh

./vcpkg install \
    --x-manifest-root=../scripts/vcpkg-manifest \
    --x-install-root=./installed \
    --triplet=<triplet>

# triplet:
#   Mac:   `x64-osx` or `arm64-osx`
#   Linux: `x64-linux` or `arm64-linux`
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
+ [SDL](https://github.com/libsdl-org/SDL)
    + Zlib License
+ [SndFile](https://github.com/libsndfile/libsndfile)
    + GNU LGPL v2.1 or later
+ [vcpkg](https://github.com/microsoft/vcpkg)
    + MIT License
+ [qastool](https://github.com/SineStriker/qt-json-autogen)
    + Apache 2.0 License

## License

This repository is licensed under the Apache 2.0 License.