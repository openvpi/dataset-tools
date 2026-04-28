# DiffSinger Dataset Tools

DiffSinger dataset processing tools for singing voice synthesis data preparation, including audio slicing, labeling, forced alignment, and audio-to-MIDI transcription.

## Applications

| Application | Description |
|---|---|
| **DatasetPipeline** | Unified dataset processing pipeline with 3 tabs: AudioSlicer (RMS-based slicing), LyricFA (FunASR lyric alignment), HubertFA (HuBERT phoneme alignment) |
| **MinLabel** | Audio labeling tool with G2P conversion (Mandarin/Cantonese/Japanese) |
| **PhonemeLabeler** | TextGrid phoneme boundary editor with waveform/spectrogram/power visualization, cross-tier boundary binding, undo/redo |
| **PitchLabeler** | DiffSinger .ds file F0 curve editor with piano roll visualization, multi-tool editing (Select/Modulation/Drift), A/B comparison, undo/redo |
| **GameInfer** | GAME audio-to-MIDI transcription (4-model ONNX pipeline) |

## Supported Platforms

+ Microsoft Windows (10 ~ 11) — primary, with DirectML GPU acceleration
+ Apple macOS (11+)
+ Linux (Tested on Ubuntu)

## Models

### AsrModel

[AsrModel](https://github.com/openvpi/dataset-tools/releases/tag/AsrModel)

Used for LyricFA, only supports Chinese. [jp&&en version(beta)](https://github.com/wolfgitpr/LyricFA)

### SomeModel

[SomeModel](https://github.com/openvpi/dataset-tools/releases/tag/SomeModel)

### FblModel

[FblModel](https://github.com/openvpi/dataset-tools/releases/tag/FblModel)

Currently, FoxBreatheLabeler only supports annotating breathing using TextGrid files output from SOFA (i.e. overlaying
new "AP" annotations on intervals already marked as "SP").

### GAME Model

Required for GameInfer. Place the model directory (containing `config.json`, `encoder.onnx`, `segmenter.onnx`, `bd2dur.onnx`, `dur2bd.onnx`, `estimator.onnx`) under `<app_dir>/model/`.

## Build from Source

### Requirements

| Component | Requirement |             Detailed             |
|:---------:|:-----------:|:--------------------------------:|
|    Qt     |  \>=6.8.0   | Core, Gui, Widgets, Svg, Network |
| Compiler  |  \>=C++17   |      MSVC 2022, GCC, Clang       |
|   CMake   |   \>=3.17   |      >=3.20 is recommended       |

> Tested with Qt 6.8.3 and Qt 6.9.3. CI builds use Qt 6.9.3.

### Setup Environment

You need to install Qt libraries first.

#### Windows

```sh
cd /D src/infer
cmake -Dep=dml -P ../../cmake/setup-onnxruntime.cmake

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
cd src/infer
cmake -Dep=cpu -P ../../cmake/setup-onnxruntime.cmake

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
    -DCMAKE_INSTALL_PREFIX=<dir> \
    -DCMAKE_PREFIX_PATH=<dir> \
    -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build --target all

cmake --build build --target install
```

### CMake Build Options

| Option | Default | Description |
|---|---|---|
| `BUILD_TESTS` | `ON` | Build `src/tests/` subdirectory |
| `AUDIO_UTIL_BUILD_TESTS` | `ON` | Build TestAudioUtil |
| `GAME_INFER_BUILD_TESTS` | `ON` | Build TestGame |
| `SOME_INFER_BUILD_TESTS` | `ON` | Build TestSome |
| `RMVPE_INFER_BUILD_TESTS` | `ON` | Build TestRmvpe |
| `ONNXRUNTIME_ENABLE_DML` | `ON` (Windows) | Enable DirectML GPU acceleration |
| `ONNXRUNTIME_ENABLE_CUDA` | `OFF` | Enable CUDA GPU acceleration |

### Build Outputs

| Type | Files |
|---|---|
| Applications | `DatasetPipeline.exe`, `MinLabel.exe`, `PhonemeLabeler.exe`, `PitchLabeler.exe`, `GameInfer.exe` |
| Test executables | `TestGame.exe`, `TestRmvpe.exe`, `TestSome.exe`, `TestAudioUtil.exe` |
| Shared libraries | `dstools-widgets.dll`, `audio-util.dll`, `game-infer.dll`, `some-infer.dll`, `rmvpe-infer.dll` |

## Libraries

### Related Projects

+ [DiffSinger](https://github.com/openvpi/DiffSinger)
    + Apache 2.0 License

+ [ChorusKit](https://github.com/SineStriker/qsynthesis-revenge)
    + Apache 2.0 License

### Dependencies

+ [Qt 6](https://www.qt.io/) (6.8+)
    + GNU LGPL v2.1 or later
+ [ONNX Runtime](https://github.com/microsoft/onnxruntime)
    + MIT License
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
+ [wolf-midi](https://github.com/wolfgitpr/wolf-midi)
    + MIT License
+ [nlohmann/json](https://github.com/nlohmann/json)
    + MIT License
+ [FoxBreatheLabeler](https://github.com/autumn-DL/FoxBreatheLabeler)
    + GNU AGPL v3.0
+ [textgrid.hpp](https://github.com/eiichiroi/textgrid.hpp)
    + MIT License
+ [soxr](https://sourceforge.net/projects/soxr/)
    + GNU LGPL v2.1
+ [mpg123](https://www.mpg123.de/)
    + GNU LGPL v2.1

## License

This repository is licensed under the Apache 2.0 License.