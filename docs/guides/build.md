# 开发者构建指南

本文档补充 README 中的构建步骤，提供开发者日常开发所需的详细信息。

## 前置条件

| 组件 | 版本要求 | 说明 |
|------|---------|------|
| Qt | ≥ 6.8.0 | Core, Gui, Widgets, Svg, Network, Concurrent |
| C++ 编译器 | C++20 | MSVC 2022 / GCC / Clang |
| CMake | ≥ 3.21 | |
| Ninja | 推荐 | 可选，但 CI 使用 Ninja |

## vcpkg 依赖

vcpkg manifest 位于 `scripts/vcpkg-manifest/vcpkg.json`，管理以下依赖：

| 包名 | 用途 |
|------|------|
| sdl2 | 音频播放 (dstools-audio) |
| ffmpeg-fake | FFmpeg headers (自定义 overlay port，运行时需真实 FFmpeg DLL) |
| fftw3 | FFT (PhonemeLabeler 频谱分析) |
| cpp-pinyin | 中文拼音 G2P |
| cpp-kana | 日文假名 G2P |
| soxr | 音频重采样 (audio-util) |
| wolf-midi | MIDI 读写 (game-infer) |
| libsndfile | 音频文件读写 |
| nlohmann-json | JSON 序列化 |
| qwindowkit | 无边框窗口 |
| syscmdline | 命令行解析 |

自定义 overlay ports 和 triplets 位于 `vcpkg/ports/` 和 `vcpkg/triplets/`。

### 重要：Qt 版本锁定

vcpkg 依赖（qwindowkit、qbreakpad）编译时绑定了特定 Qt 版本。如果需要切换 Qt 版本，必须重新安装 vcpkg 依赖。

## ONNX Runtime 下载

构建前需先下载 ORT 二进制到 `src/infer/onnxruntime/`：

```sh
# Windows (DirectML GPU)
cd src/infer
cmake -Dep=dml -P ../../cmake/setup-onnxruntime.cmake

# Unix (CPU)
cd src/infer
cmake -Dep=cpu -P ../../cmake/setup-onnxruntime.cmake

# CUDA
cd src/infer
cmake -Dep=gpu -P ../../cmake/setup-onnxruntime.cmake
```

| `-Dep` 值 | 来源 | 说明 |
|-----------|------|------|
| `cpu` (默认) | GitHub Release | 纯 CPU |
| `dml` | NuGet | Windows DirectML GPU |
| `gpu` | GitHub Release | CUDA GPU |

## CMake 选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `BUILD_TESTS` | ON | 构建单元测试目标 |
| `BUILD_INTEGRATION_TESTS` | OFF | 构建集成/UI 测试（TestAppShellIntegration, dsfw-widgets-test） |
| `AUDIO_UTIL_BUILD_TESTS` | OFF | TestAudioUtil |
| `GAME_INFER_BUILD_TESTS` | OFF | TestGame（需要 GAME 模型） |
| `RMVPE_INFER_BUILD_TESTS` | OFF | TestRmvpe（需要 RMVPE 模型） |
| `ONNXRUNTIME_ENABLE_DML` | ON (Windows) | DirectML 加速 |
| `ONNXRUNTIME_ENABLE_CUDA` | OFF | CUDA 加速 |

### 测试层级

| 层级 | 控制选项 | 包含测试 | 说明 |
|------|---------|---------|------|
| Unit | `BUILD_TESTS` (ON) | TestTimePos, TestResult, TestJsonHelper, TestAppSettings 等 21 个 | 纯逻辑测试，无外部依赖 |
| Integration | `BUILD_INTEGRATION_TESTS` (OFF) | TestAppShellIntegration, dsfw-widgets-test | 需要 QApplication / GUI |
| Infer | `*_BUILD_TESTS` (OFF) | TestGame, TestRmvpe, TestAudioUtil | 需要 ONNX 模型文件 |

## 构建命令

### 通用构建

```sh
cmake -B build -G Ninja \
    -DCMAKE_PREFIX_PATH=<qt-dir> \
    -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build
```

### Windows 本地构建

#### 一次性环境准备

1. **克隆仓库 + vcpkg**

```bat
git clone https://github.com/openvpi/dataset-tools.git
cd dataset-tools
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat
cd ..
```

2. **安装 vcpkg 依赖**

```bat
set QT_DIR=D:\Programs\qt5\6.10.0\msvc2022_64
set Qt6_DIR=%QT_DIR%
set VCPKG_KEEP_ENV_VARS=QT_DIR;Qt6_DIR

vcpkg\vcpkg install ^
    --x-manifest-root=scripts/vcpkg-manifest ^
    --x-install-root=vcpkg/installed ^
    --triplet=x64-windows
```

3. **下载 ONNX Runtime (DirectML)**

```bat
cd src\infer
cmake -Dep=dml -P ..\..\cmake\setup-onnxruntime.cmake
cd ..\..
```

#### 编译脚本

项目根目录提供 `scripts/build-release.bat` 和 `scripts/build-debug.bat`，双击或命令行运行即可。

#### 手动编译（VS Developer Command Prompt）

打开 **VS x64 Native Tools Command Prompt**，进入项目目录：

```bat
cmake -B build -G Ninja ^
    -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake ^
    -DCMAKE_PREFIX_PATH=D:\Programs\qt5\6.10.0\msvc2022_64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DONNXRUNTIME_ENABLE_DML=ON

cmake --build build -j 10
```

#### 手动编译（普通 CMD / PowerShell，无需提前开 Developer Prompt）

```bat
call "<VS_PATH>\VC\Auxiliary\Build\vcvarsall.bat" x64

set QT_DIR=D:\Programs\qt5\6.10.0\msvc2022_64
set Qt6_DIR=%QT_DIR%
set VCPKG_KEEP_ENV_VARS=QT_DIR;Qt6_DIR

cmake -B build -G Ninja ^
    -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake ^
    -DCMAKE_PREFIX_PATH=%QT_DIR% ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DONNXRUNTIME_ENABLE_DML=ON

cmake --build build -j 10
```

#### CLion 配置

1. **Settings → Build → Toolchains**: Visual Studio，Architecture `amd64`
2. **Settings → Build → CMake**: Profile CMake options：

```
-DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake
-DCMAKE_PREFIX_PATH=D:\Programs\qt5\6.10.0\msvc2022_64
-DONNXRUNTIME_ENABLE_DML=ON
```

3. **Environment**: 添加 `QT_DIR=<qt-dir>;Qt6_DIR=<qt-dir>\lib\cmake\Qt6;VCPKG_KEEP_ENV_VARS=QT_DIR;Qt6_DIR`
4. Generator: Ninja, Build type: Release 或 Debug

## 安装与部署

```sh
cmake -DCMAKE_INSTALL_PREFIX=<dir> -B build ...
cmake --build build --target install
```

安装过程通过 `DeployedTargets` 自定义目标收集需部署的可执行文件，配合平台对应的部署工具：
- Windows: `windeployqt` 自动打包 Qt 运行时 + 复制所有 DLL
- macOS: `macdeployqt` 打包为 .app bundle
- Linux: 复制 .so 和可执行文件

## 运行测试

```bat
cd build
ctest --output-on-failure
```

## 构建产物

| 类型 | 产物 |
|------|------|
| 应用 | LabelSuite, DsLabeler |
| 工具 | dstools-cli, WidgetGallery, TestShell |
| 测试 | TestTimePos, TestResult, TestJsonHelper, TestAppSettings, TestServiceLocator, TestAsyncTask, TestLocalFileIOProvider, TestModelManager, TestModelDownloader, TestF0Curve, TestCurveTools 等 |
| 动态库 | dsfw-widgets, dstools-widgets, audio-util, game-infer, rmvpe-infer, hubert-infer |

所有二进制输出到 `build/bin/`，库文件输出到 `build/lib/`。

## 项目目录约定

- `cmake/` — CMake 辅助脚本（宏、下载脚本、资源模板、CMake 包配置模板）
- `src/types/` — 基础类型 header-only 库（Result<T>, ExecutionProvider, TimePos）
- `src/framework/base/` — Qt-free 静态库（dsfw-base，JsonHelper）
- `src/framework/core/` — 通用框架核心静态库（dsfw-core，无 Qt::Widgets 依赖）
- `src/framework/ui-core/` — UI 框架静态库（dsfw-ui-core，AppShell/Theme/FramelessHelper）
- `src/framework/widgets/` — 通用 GUI 组件动态库（dsfw-widgets）
- `src/domain/` — DiffSinger 领域逻辑静态库（dstools-domain）
- `src/framework/audio/` — 音频解码/播放静态库（dstools-audio）
- `src/widgets/` — 领域 GUI 组件动态库（dstools-widgets）
- `src/libs/` — 第三方 header-only 库
- `src/infer/` — 推理库（各自独立，共享 infer-common）
- `src/apps/` — 应用可执行文件（LabelSuite 通用标注工具集 + DsLabeler DiffSinger 专用标注器 + dstools-cli + WidgetGallery + TestShell）
- `src/tests/` — 测试（framework/ 子目录含 dsfw 核心类测试）
- `scripts/vcpkg-manifest/` — vcpkg 依赖声明
- `vcpkg/` — 自定义 overlay ports/triplets

## 常见问题

### LNK1181: 无法打开输入文件 "kernel32.lib"

未正确加载 MSVC 环境。确保先执行 `vcvarsall.bat x64`，或使用 VS Developer Command Prompt。

### MOC 报 "Undefined interface"

AUTOMOC 的 include 路径缺少 `dsfw-ui-core`。对应 CMakeLists.txt 需要显式添加 `dsfw-ui-core` 到 `target_link_libraries`。

### Qt 版本不匹配 (moc 报 "cannot be used with the include files from this version of Qt")

vcpkg 安装的 qwindowkit/qbreakpad 绑定了特定 Qt 版本。确保 `QT_DIR` 和 `CMAKE_PREFIX_PATH` 指向与 vcpkg 编译时相同的 Qt 版本。

### vcpkg 包安装失败

部分包 (qwindowkit, qbreakpad) 需要 Qt 路径才能编译。确认 `QT_DIR` 和 `VCPKG_KEEP_ENV_VARS` 已设置。

### ONNX Runtime 下载失败

`setup-onnxruntime.cmake` 会自动检测系统代理。手动下载从 [NuGet](https://www.nuget.org/packages/Microsoft.ML.OnnxRuntime.DirectML) 获取 DML 版本，解压到 `src/infer/onnxruntime/`。
