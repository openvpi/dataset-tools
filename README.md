# DiffSinger Dataset Tools

DiffSinger dataset processing tools for singing voice synthesis data preparation, including audio slicing, labeling, forced alignment, and audio-to-MIDI transcription.

## Application

| Application | Description |
|---|---|
| **LabelSuite** | All-in-one audio labeling toolset (AppShell multi-page). 10 pages (Slice, ASR, Label, Align, Phone, CSV, MIDI, DS, Pitch, Settings). Shares the same dstext/PipelineContext data model as DsLabeler; imports/exports legacy formats (TextGrid, .lab, .ds) via FormatAdapters. Supports auto-completion (auto FA/F0/MIDI on page entry). No `.dsproj` project file required. |
| **DsLabeler** | DiffSinger dataset labeler driven by `.dsproj` project files. Seven pages: Welcome (create/open project), Slicer (audio slicing + export), MinLabel (+ ASR/LyricFA), PhonemeLabeler (+ auto FA), PitchLabeler (+ auto F0/MIDI extraction), Export (CSV/DS/WAV output with auto-completion of skipped steps), Settings (unified configuration). |

See [unified-app-design.md](docs/unified-app-design.md) for the full design.

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
| Compiler  |  \>=C++20   |      MSVC 2022, GCC, Clang       |
|   CMake   |  \>=3.21    |                                  |

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
| `BUILD_TESTS` | `ON` | Build unit tests |
| `BUILD_INTEGRATION_TESTS` | `OFF` | Build integration/UI tests (require GUI) |
| `AUDIO_UTIL_BUILD_TESTS` | `OFF` | Build TestAudioUtil |
| `GAME_INFER_BUILD_TESTS` | `OFF` | Build TestGame (requires GAME model) |
| `RMVPE_INFER_BUILD_TESTS` | `OFF` | Build TestRmvpe (requires RMVPE model) |
| `ONNXRUNTIME_ENABLE_DML` | `ON` (Windows) | Enable DirectML GPU acceleration |
| `ONNXRUNTIME_ENABLE_CUDA` | `OFF` | Enable CUDA GPU acceleration |

### Build Outputs

| Type | Files |
|---|---|
| Applications | `LabelSuite.exe`, `DsLabeler.exe` |
| Tools | `dstools-cli.exe`, `WidgetGallery.exe` |
| Test executables | `TestGame.exe`, `TestRmvpe.exe`, `TestAudioUtil.exe` |
| Shared libraries | `dsfw-widgets.dll`, `dstools-widgets.dll`, `audio-util.dll`, `game-infer.dll`, `rmvpe-infer.dll`, `hubert-infer.dll` |

## Framework (dsfw)

The project includes a reusable C++20/Qt6 desktop application framework (`dsfw`) that can be consumed independently via `find_package(dsfw)`. The framework provides:

- **dsfw-core** — Type-safe settings, JSON utilities, service locator, document/model/G2P/export interfaces, file I/O, async tasks, pipeline context and runner
- **dsfw-ui-core** — AppShell unified window shell (single/multi-page modes), theme system, frameless window helper, icon navigation bar, page lifecycle interfaces
- **dsfw-widgets** — Reusable GUI components (PlayWidget, ProgressDialog, PropertyEditor, etc.) as a shared library

After building and installing the project, external projects can use:

```cmake
find_package(dsfw REQUIRED)
target_link_libraries(myapp PRIVATE dsfw::core dsfw::ui-core)
```

### Framework Documentation

| Document | Description |
|---|---|
| [Unified App Design](docs/unified-app-design.md) | DsSuite unified application design (multi-page AppShell) |
| [Getting Started](docs/framework-getting-started.md) | Quick start guide with hello world examples |
| [Architecture](docs/framework-architecture.md) | Detailed architecture design and layer descriptions |
| [Migration Guide](docs/migration-guide.md) | How to migrate from QMainWindow to AppShell |

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