# 开发者构建指南

本文档补充 README 中的构建步骤，提供开发者日常开发所需的详细信息。

## 前置条件

| 组件 | 版本要求 | 说明 |
|------|---------|------|
| Qt | ≥ 6.8.0 | Core, Gui, Widgets, Svg, Network, Concurrent |
| C++ 编译器 | C++17 | MSVC 2022 / GCC / Clang |
| CMake | ≥ 3.17 | 推荐 ≥ 3.20 |
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
| qbreakpad | 崩溃报告 (可选) |
| nlohmann-json | JSON 序列化 |
| qwindowkit | 无边框窗口 |
| syscmdline | 命令行解析 |

自定义 overlay ports 和 triplets 位于 `vcpkg/ports/` 和 `vcpkg/triplets/`。

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
| `BUILD_TESTS` | ON | 构建测试目标 |
| `AUDIO_UTIL_BUILD_TESTS` | ON | TestAudioUtil |
| `GAME_INFER_BUILD_TESTS` | ON | TestGame |
| `RMVPE_INFER_BUILD_TESTS` | ON | TestRmvpe |
| `ONNXRUNTIME_ENABLE_DML` | ON (Windows) | DirectML 加速 |
| `ONNXRUNTIME_ENABLE_CUDA` | OFF | CUDA 加速 |

## 构建命令

```sh
cmake -B build -G Ninja \
    -DCMAKE_PREFIX_PATH=<qt-dir> \
    -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build
```

## 构建产物

| 类型 | 产物 |
|------|------|
| 应用 | DatasetPipeline, MinLabel, PhonemeLabeler, PitchLabeler, GameInfer, DiffSingerLabeler |
| 测试 | TestGame, TestRmvpe, TestAudioUtil, TestResult, TestJsonHelper, TestAppSettings, TestServiceLocator, TestAsyncTask, TestLocalFileIOProvider, TestModelManager, TestModelDownloader, TestAppShellIntegration |
| 动态库 | dstools-widgets, audio-util, game-infer, rmvpe-infer, hubert-infer |

所有二进制输出到 `build/bin/`，库文件输出到 `build/lib/`。

## 安装与部署

```sh
cmake -DCMAKE_INSTALL_PREFIX=<dir> -B build ...
cmake --build build --target install
```

安装过程通过 `DeployedTargets` 自定义目标收集需部署的可执行文件，配合平台对应的部署工具：
- Windows: `windeployqt` 自动打包 Qt 运行时 + 复制所有 DLL
- macOS: `macdeployqt` 打包为 .app bundle
- Linux: 复制 .so 和可执行文件

## 项目目录约定

- `cmake/` — CMake 辅助脚本（宏、下载脚本、资源模板、CMake 包配置模板）
- `src/types/` — 基础类型 header-only 库（Result<T>, ExecutionProvider）
- `src/framework/core/` — 通用框架核心静态库（dsfw-core，无 Qt::Widgets 依赖）
- `src/framework/ui-core/` — UI 框架静态库（dsfw-ui-core，AppShell/Theme/FramelessHelper）
- `src/domain/` — DiffSinger 领域逻辑静态库（dstools-domain）
- `src/audio/` — 音频解码/播放静态库
- `src/widgets/` — 通用 GUI 组件动态库
- `src/libs/` — 第三方 header-only 库
- `src/infer/` — 推理库（各自独立，共享 infer-common）
- `src/apps/` — 6 个应用可执行文件
- `src/tests/` — 测试（framework/ 子目录含 dsfw 核心类测试）
- `scripts/vcpkg-manifest/` — vcpkg 依赖声明
- `vcpkg/` — 自定义 overlay ports/triplets
