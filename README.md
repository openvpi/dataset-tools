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

## AsrModel

[AsrModel](https://github.com/openvpi/dataset-tools/releases/tag/AsrModel)

Used for LyricFA, only supports Chinese. [jp&&en version(beta)](https://github.com/wolfgitpr/LyricFA)

## SomeModel

[SomeModel](https://github.com/openvpi/dataset-tools/releases/tag/SomeModel)

## FblModel

[FblModel](https://github.com/openvpi/dataset-tools/releases/tag/FblModel)

Currently, FoxBreatheLabeler only supports annotating breathing using TextGrid files output from SOFA(i.e. overlaying
new "AP" annotations on intervals already marked as "SP").

## Build from source

### Requirements

| Component | Requirement |             Detailed             |
|:---------:|:-----------:|:--------------------------------:|
|    Qt     |  \>=6.8.0   | Core, Gui, Widgets, Svg, Network |
| Compiler  |  \>=C++17   |      MSVC 2022, GCC, Clang       |
|   CMake   |   \>=3.17   |      >=3.20 is recommended       |
|  Python   |   \>=3.8    |                                  |

### Setup Environment

You need to install Qt libraries first. (Tested on Qt 6.8.3 only)

#### Windows

```sh
cd /D src/libs
cmake -Dep=dml -P ../../scripts/setup-onnxruntime.cmake

cd ../../
set QT_DIR=<dir> # directory `Qt6Config.cmake` locates
set Qt6_DIR=%QT_DIR%
set VCPKG_KEEP_ENV_VARS=QT_DIR;Qt6_DIR

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
cd src/libs
cmake -Dep=cpu -P ../../scripts/setup-onnxruntime.cmake

cd ../../
export QT_DIR=<dir> # directory `Qt6Config.cmake` locates
export Qt6_DIR=$QT_DIR
export VCPKG_KEEP_ENV_VARS="QT_DIR;Qt6_DIR"

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

+ [Qt 6.8.3](https://www.qt.io/)
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
+ [r8brain-free-src](https://github.com/avaneev/r8brain-free-src)
    + MIT License
+ [FunASR](https://github.com/alibaba-damo-academy/FunASR)
    + MIT License
+ [fftw3](https://github.com/FFTW/fftw3)
    + GNU GPL v2.0
+ [yaml-cpp](https://github.com/jbeder/yaml-cpp)
    + MIT License
+ [FoxBreatheLabeler](https://github.com/autumn-DL/FoxBreatheLabeler)
    + GNU AGPL v3.0
+ [textgrid.hpp](https://github.com/eiichiroi/textgrid.hpp)
    + MIT License

## License

This repository is licensed under the Apache 2.0 License.