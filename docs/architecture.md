# 架构概述

## 技术栈

| 维度 | 选型 |
|------|------|
| 语言 | C++17 |
| GUI | Qt 6.8+ (Core, Widgets, Svg, Network, Concurrent) |
| 推理 | ONNX Runtime (DirectML / CUDA / CPU) |
| 构建 | CMake ≥ 3.17 + vcpkg |
| 音频 | FFmpeg (解码) + SDL2 (播放) + SndFile + mpg123 + soxr (重采样) |
| 平台 | Windows 10/11 (主), macOS 11+, Linux |

## 模块依赖图

```
┌──────────────────────────────────────────────────────────────────────────┐
│                           应用层 (src/apps/)                             │
├────────────┬──────────┬────────────┬────────────┬──────────┬────────────┤
│ Dataset    │ MinLabel │ Phoneme    │ Pitch      │ Game     │ DiffSinger │
│ Pipeline   │          │ Labeler    │ Labeler    │ Infer    │ Labeler    │
└─────┬──────┴────┬─────┴─────┬──────┴─────┬──────┴────┬─────┴─────┬──────┘
      │           │           │            │           │           │
      ▼           ▼           ▼            ▼           ▼           ▼
┌──────────────────────────────────────────────────────────────────────────┐
│                      dstools-widgets (SHARED DLL)                        │
│  PlayWidget · TaskWindow · GpuSelector · ShortcutManager                │
│  FileProgressTracker · BaseFileListPanel · ModelLoadPanel · ...          │
└────────────────────────────┬─────────────────────────────────────────────┘
                             │ PUBLIC
                ┌────────────┼────────────┐
                ▼            ▼            │
┌───────────────────┐ ┌───────────────┐  │
│  dsfw-ui-core     │ │ dstools-audio │  │
│  (STATIC)         │ │ (STATIC)      │  │
│                   │ │               │  │
│ AppShell          │ │ AudioDecoder  │  │
│ IconNavBar        │ │ AudioPlayback │  │
│ Theme/Frameless   │ │ WaveFormat    │  │
│ IPageActions      │ │               │  │
│ IPageLifecycle    │ │ FFmpeg + SDL2 │  │
│ IStepPlugin       │ └───────────────┘  │
└───────┬───────────┘                    │
        │ PUBLIC                         │
┌───────┴───────────┐                    │
│  dsfw-core        │                    │
│  (STATIC)         │                    │
│                   │                    │
│ AppSettings       │                    │
│ ServiceLocator    │                    │
│ AsyncTask         │                    │
│ EventBus · Logger │                    │
│ UndoStack/ICommand│                    │
│ RecentFiles       │                    │
│ UpdateChecker     │                    │
│ IDocument         │                    │
│ IFileIOProvider   │                    │
│ IModelProvider    │                    │
│ IModelDownloader  │                    │
│ IModelManager     │                    │
│ IG2PProvider      │                    │
│ IExportFormat     │                    │
│ IQualityMetrics   │                    │
│ ISlicerService    │                    │
│ IAlignmentService │                    │
│ IAsrService       │                    │
│ IPitchService     │                    │
│ ITranscription-   │                    │
│   Service         │                    │
└───────┬───────────┘                    │
        │ PUBLIC                         │
┌───────┴───────────┐                    │
│  dsfw-base        │                    │
│  (STATIC, Qt-free)│                    │
│                   │                    │
│ JsonHelper        │                    │
└───────┬───────────┘                    │
        │ PUBLIC                         │
┌───────┴───────────┐ ┌─────────────────┘
│  dstools-domain   │ │
│  (STATIC)         │ │
│                   │ │
│ DsDocument        │ │
│ DsProject         │ │
│ DsItemManager     │ │
│ ModelManager      │ │
│ CsvToDsConverter  │ │
│ TextGridToCsv     │ │
│ TranscriptionCsv  │ │
│ TranscriptionPipe │ │
│ PitchUtils/F0Curve│ │
│ PinyinG2PProvider │ │
│ ExportFormats     │ │
│ QualityMetrics    │ │
└───────┬───────────┘ │
        │             │
┌───────┴───────────┐ │
│  dstools-types    │ │
│  (HEADER-ONLY)    │◄┘
│  Result<T>        │
│  ExecutionProvider│
└───────────────────┘

┌────────────────────────────────────────┐
│              推理层 (src/infer/)        │
│                                        │
│  ┌──────────────────────┐              │
│  │ dstools-infer-common  │ (STATIC)    │
│  │ OnnxEnv 单例          │              │
│  │ OnnxModelBase         │              │
│  │ CancellableOnnxModel  │              │
│  │ IInferenceEngine      │              │
│  │ ExecutionProvider 枚举│              │
│  └──────────┬───────────┘              │
│             │ PUBLIC                    │
│  ┌──────────┼──────────┬──────────────┐│
│  ▼          ▼          ▼              ▼│
│ audio-util game-infer rmvpe-infer  hubert-infer
│ (SHARED)   (SHARED)   (SHARED)     (SHARED/STATIC)
│                                        │
│  FunAsr (STATIC) ← 独立，直接链接 ORT  │
└────────────────────────────────────────┘
```

## 各应用依赖明细

| 应用 | 直接链接的库 |
|------|-------------|
| DatasetPipeline | dstools-widgets, dstools-domain, audio-util, hubert-infer, FunAsr, textgrid, cpp-pinyin, SndFile, nlohmann_json |
| MinLabel | dstools-widgets, dstools-domain, cpp-pinyin, cpp-kana |
| PhonemeLabeler | dstools-widgets, dstools-domain, dstools-audio, textgrid, FFTW3 |
| PitchLabeler | dstools-widgets, dstools-domain |
| GameInfer | dstools-widgets, dstools-domain, game-infer |
| DiffSingerLabeler | dstools-widgets, dstools-domain, dstools-audio, audio-util, hubert-infer, game-infer, rmvpe-infer, FunAsr, cpp-pinyin, cpp-kana, textgrid, FFTW3, SndFile, nlohmann_json, Qt::Concurrent |

> 所有 6 个应用均使用 `dsfw::AppShell` 作为统一窗口壳，通过 `addPage()` 注册页面。DiffSingerLabeler 使用多页面模式，其余 5 个应用使用单页面模式。

## 共享库

### dsfw-base (静态库, Qt-free)

JSON 工具 (JsonHelper，纯 nlohmann/json 封装)。无 Qt 依赖，可用于 CLI 工具。

依赖：nlohmann_json

### dsfw-core (静态库)

通用框架核心。类型安全配置 (AppSettings)、服务定位器 (ServiceLocator)、异步任务 (AsyncTask)、事件总线 (EventBus)、结构化日志 (Logger)、撤销/重做 (UndoStack/ICommand)、最近文件列表 (RecentFiles)、更新检查 (UpdateChecker)、文档/文件/模型/导出/G2P/质量评估抽象接口 (IDocument, IModelManager, IModelDownloader 等)、后端服务接口 (ISlicerService, IAlignmentService, IAsrService, IPitchService, ITranscriptionService)。

依赖：dsfw-base, dstools-types, Qt Core/Network, nlohmann_json

### dsfw-ui-core (静态库)

Qt 应用 UI 框架。统一窗口壳 (AppShell)、图标侧边导航 (IconNavBar)、主题管理 (Theme)、无边框窗口 (FramelessHelper)、页面接口 (IPageActions/IPageLifecycle/IPageProgress)、步骤插件 (IStepPlugin)。

依赖：dsfw-core, Qt Widgets, QWindowKit

### dstools-domain (静态库)

DiffSinger 领域逻辑。DsDocument/.ds 文件读写、DsProject/.dsproj 项目、ModelManager (具体实现)、F0 曲线、格式转换 (TextGrid↔CSV↔DS)、拼音 G2P、导出格式、质量评估。

依赖：dsfw-core, dstools-types, Qt Core/Gui/Network, nlohmann_json, textgrid (PRIVATE), SndFile (PRIVATE)

### dstools-audio (静态库)

AudioDecoder (FFmpeg)、AudioPlayback (SDL2)、AudioPlayer、WaveFormat。

依赖：Qt Core, FFmpeg, SDL2

### dstools-widgets (动态库)

通用 GUI 组件。所有应用的公共 UI 基础设施。

依赖：dsfw-ui-core + dsfw-core + dstools-audio (PUBLIC)

### 推理库

| 库 | 类型 | 功能 | 特有依赖 |
|----|------|------|----------|
| dstools-infer-common | 静态 | OnnxEnv 单例 + OnnxModelBase/CancellableOnnxModel + IInferenceEngine + EP 选择 | dstools-types, onnxruntime |
| audio-util | 动态 | 重采样/格式转换/读写 | SndFile, soxr, mpg123, (xsimd) |
| game-infer | 动态 | GAME Audio→MIDI | audio-util, wolf-midi, SndFile, nlohmann_json |
| rmvpe-infer | 动态 | RMVPE F0 提取 | audio-util, SndFile |
| hubert-infer | 动态 | HuBERT 强制对齐 | audio-util, SndFile, nlohmann_json |
| FunAsr | 静态 | FunASR Paraformer 中文 ASR | (直接链接 ORT) |

### 第三方库

| 库 | 位置 |
|----|------|
| textgrid | src/libs/textgrid/ (header-only TextGrid 解析) |

## 构建顺序

```
types → base → core → ui-core → audio → widgets → domain → libs → infer → apps → tests
```

所有推理库并行构建（仅共享 dstools-infer-common）。

## 目录结构

```
dataset-tools/
├── CMakeLists.txt              # 根配置: C++17, 输出目录, 编译器选项
├── cmake/
│   ├── infer-target.cmake      # dstools_add_infer_library() 宏
│   ├── setup-onnxruntime.cmake # ORT 下载脚本 (cpu/dml/gpu)
│   ├── dsfwConfig.cmake.in     # dsfw CMake 包配置模板
│   ├── dstools-typesConfig.cmake.in
│   ├── winrc.cmake             # Windows 版本资源
│   └── utils.cmake
├── src/
│   ├── types/                  # dstools-types (HEADER-ONLY)
│   │   └── include/dstools/    # Result<T>, ExecutionProvider
│   ├── framework/
│   │   ├── base/                # dsfw-base (STATIC, Qt-free)
│   │   │   └── include/dsfw/   # JsonHelper
│   │   ├── core/               # dsfw-core (STATIC)
│   │   │   ├── include/dsfw/   # AppSettings, ServiceLocator, EventBus, Logger, ...
│   │   │   └── src/
│   │   └── ui-core/            # dsfw-ui-core (STATIC)
│   │       ├── include/dsfw/   # AppShell, Theme, FramelessHelper, IPageActions, ...
│   │       ├── src/
│   │       └── res/            # 主题 QSS, 资源文件
│   ├── domain/                 # dstools-domain (STATIC)
│   │   ├── include/dstools/    # DsDocument, DsProject, CsvToDsConverter, ...
│   │   └── src/
│   ├── audio/                  # dstools-audio (STATIC)
│   ├── widgets/                # dstools-widgets (SHARED)
│   ├── libs/textgrid/          # header-only
│   ├── infer/
│   │   ├── common/             # dstools-infer-common (STATIC)
│   │   ├── onnxruntime/        # 预下载 ORT 二进制
│   │   ├── audio-util/         # (SHARED, 独立可安装)
│   │   ├── game-infer/         # (SHARED)
│   │   ├── rmvpe-infer/        # (SHARED)
│   │   ├── hubert-infer/       # (SHARED)
│   │   └── FunAsr/             # (STATIC)
│   ├── apps/
│   │   ├── pipeline/           # DatasetPipeline
│   │   ├── MinLabel/
│   │   ├── PhonemeLabeler/
│   │   ├── PitchLabeler/
│   │   ├── GameInfer/
│   │   └── labeler/            # DiffSingerLabeler
│   └── tests/
│       ├── framework/          # dsfw 核心类单元测试
│       └── CMakeLists.txt      # 推理库测试注册
├── scripts/vcpkg-manifest/     # vcpkg 依赖声明
└── docs/
```
