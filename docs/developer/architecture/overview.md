# 项目架构

> 编码规范见 [conventions.md](../guides/conventions.md)。测试架构见 [test-design.md](test-design.md)
> 。设计准则与决策记录见 [human-decisions.md](../human-decisions.md)
> 。核心组件设计见 [data-flow/component-design.md](data-flow/component-design.md)。

---

## 1. 设计准则

设计准则详见 [human-decisions.md](../human-decisions.md)（ARCH/CONCUR/ROBUST/INFRA/VIEW 五领域体系），
编码规范详见 [conventions.md](../guides/conventions.md)。

---

## 2. 技术栈

| 维度  | 选型                                                      |
|-----|---------------------------------------------------------|
| 语言  | C++20                                                   |
| GUI | Qt 6.8+ (Core, Widgets, Svg, Network, Concurrent)       |
| 推理  | ONNX Runtime (DirectML / CUDA / CPU)                    |
| 构建  | CMake ≥ 3.21 + vcpkg                                    |
| 音频  | FFmpeg (解码) + SDL2 (播放) + SndFile + mpg123 + soxr (重采样) |
| 平台  | Windows 10/11 (主), macOS 11+, Linux                     |

---

## 3. 框架分离原则

项目拆分为：

- **dsfw（通用框架层）**: 可复用的 C++20/Qt6 桌面应用框架，不含歌声合成领域逻辑
- **dstools-app（应用层）**: 基于 dsfw 的 DiffSinger 数据集处理工具

| 判断标准              | 归入框架 | 归入应用层 |
|-------------------|------|-------|
| 涉及 .ds/.dsproj 格式 | 否    | 是     |
| 涉及特定 G2P（拼音、假名）   | 否    | 是     |
| 涉及特定 AI 模型        | 否    | 是     |
| 可脱离上述概念独立运行       | 是    | 否     |

```
┌─────────────────────────────────────────────────────────┐
│                  DiffSinger App Layer                    │
│  DsDocument, DsProject, PinyinG2PProvider, ExportFormats│
│  TranscriptionPipeline, CsvToDsConverter, TextGridToCsv │
│  game-infer, hubert-infer, rmvpe-infer, FunAsr          │
│  各领域 Page 实现, LabelSuite + DsLabeler + CLI + TestShell + WidgetGallery│
├─────────────────────────────────────────────────────────┤
│               dsfw (通用框架)                             │
│  types / core / audio / ui-core / widgets / infer-common│
│  AppShell ← 所有应用的统一窗口壳                          │
└─────────────────────────────────────────────────────────┘
```

---

## 4. 模块层次结构

```
App Layer     ds-labeler, label-suite, dstools-cli, widget-gallery
                    ↓
App Shared    dstools-ui-core     (STATIC, 包装 dsfw-ui-core + dsfw-core + dstools-domain)
App Libs      dstools-domain      (STATIC, 领域逻辑: DsDocument, F0Curve, CurveTools, 适配器等)
                    ↓
Layer 4 ─ dsfw-widgets           通用 UI 组件 (SHARED DLL)
Layer 3 ─ dsfw-ui-core           AppShell, IconNavBar, Theme, FramelessHelper, IPageActions
Layer 2 ─ dsfw-audio + dsfw-audio-playback   音频解码(FFmpeg) + 重采样 + 播放适配(SDL2)
Layer 1 ─ dsfw-core              AppSettings, ServiceLocator, AsyncTask, 接口集
                                 PipelineContext, PipelineRunner, ITaskProcessor
                                 JsonHelper, 含 infer-common 源文件 (OnnxEnv, OnnxModelBase)
Layer 0 ─ dsfw-types             Result<T>, ExecutionProvider, TimePos (header-only)

此外层:
dsfw-signal     curve_tools, music_math, time_series (dsfw::signal 命名空间)
```

### 依赖关系

```
dsfw-widgets ─PUBLIC──→ dsfw-core ───→ dsfw-types
    │                       ↑
    ├─PRIVATE→ dsfw-ui-core ┘
    └─PUBLIC→ dsfw-audio-playback

dsfw-signal ───→ dsfw-types
dstools-domain → dsfw-core + dsfw-signal + dsfw-types
dstools-ui-core → dsfw-ui-core + dsfw-core + dstools-domain
```

**注意**：`dstools-infer-common` 非独立库，其源文件通过 target_sources 编译入 dsfw-core。

---

## 5. 模块依赖图

```
┌──────────────────────────────────────────────────────────────────────────┐
│                           应用层 (src/apps/)                             │
├──────────────────────────────────────────────────────────────────────────┤
│ LabelSuite (通用标注工具集, 11 页面)                                       │
│   Slice · ASR · Label · Align · Phone · CSV · MIDI · DS · Pitch        │
│   Settings · Log                                                        │
│                                                                          │
│ DsLabeler (DiffSinger 专用标注器, 8 页面)                                  │
│   WelcomePage · DsSlicerPage · MinLabelPage                             │
│   PhonemeLabelerPage · PitchLabelerPage · ExportPage                    │
│   SettingsPage · LogPage                                                │
│                                                                          │
│ dstools-cli · WidgetGallery                        │
└─────┬──────┴────┬─────┴─────┬──────┴─────┬──────┴────┬─────┴─────┬──────┘
      │           │           │            │           │           │
      ▼           ▼           ▼            ▼           ▼           ▼
┌──────────────────────────────────────────────────────────────────────────┐
│                      dsfw-widgets (SHARED DLL)                           │
│  PlayWidget · FileProgressTracker · ProgressDialog · PropertyEditor     │
│  SettingsDialog · LogViewer · RunProgressRow · PathSelector · ...        │
│  RecentPathStore · FilePathSelector                                     │
└────────────────────────────┬─────────────────────────────────────────────┘
                             │ PUBLIC
                ┌────────────┼────────────┐
                ▼            ▼            │
┌───────────────────┐ ┌──────────────────────┐  │
│  dsfw-ui-core     │ │ dsfw-audio-playback  │  │
│  (STATIC)         │ │ (STATIC)             │  │
│                   │ │                      │  │
│ AppShell          │ │ IAudioPlayerAdapter  │  │
│ IconNavBar        │ │ AudioPlayerAdapter   │  │
│ Theme/Frameless   │ │ AudioPlaybackAdapter │  │
│ IPageActions      │ │                      │  │
│ IPageLifecycle    │ │ SDL2                  │  │
│                   │ └───────────────┘  │
└───────┬───────────┘                    │
        │ PUBLIC                         │
┌───────┴───────────┐                    │
│  dsfw-core        │                    │
│  (STATIC)         │                    │
│                   │                    │
│ AppSettings       │                    │
│ ServiceLocator    │                    │
│ AsyncTask         │                    │
│ Logger            │                    │
│ IDocument         │                    │
│ IModelProvider    │                    │
│ IG2PProvider      │                    │
│ IExportFormat     │                    │
│ IAlignmentService │                    │
│ IAsrService       │                    │
│ IPitchService     │                    │
│ ITranscription-   │                    │
│   Service         │                    │
│ IInferenceService │                    │
│ PipelineContext   │                    │
│ PipelineRunner    │                    │
│ ITaskProcessor    │                    │
└───────┬───────────┘                    │
        │ PUBLIC                         │
┌───────┴───────────┐                    │
│  dsfw-signal      │                    │
│  (STATIC)         │                    │
│                   │                    │
│ CurveTools        │                    │
│ F0Curve           │                    │
│ MouthCurve        │                    │
│ MusicMath         │                    │
└───────┬───────────┘                    │
        │ PUBLIC                         │
┌───────┴───────────┐ ┌─────────────────┘
│  dstools-domain   │ │
│  (STATIC)         │ │
│                   │ │
│ DsDocument        │ │
│ DsProject         │ │
│ ModelManager      │ │
│ CsvToDsConverter  │ │
│ TextGridToCsv     │ │
│ PitchUtils        │ │
│ F0Curve           │ │
│ MouthCurve        │ │
│ PinyinG2PProvider │ │
│ ExportFormats     │ │
│ QualityMetrics    │ │
│ ModelDownloader   │ │
│ AudioFileResolver │ │
└───────┬───────────┘ │
        │             │
┌───────┴───────────┐ │
│  dstools-types    │ │
│  (HEADER-ONLY)    │◄┘
│  Result<T>        │
│  ExecutionProvider│
│  TimePos          │
│  PathEncoding     │
└───────────────────┘

┌────────────────────────────────────────┐
│              引擎层 (src/engine/)      │
│                                        │
│  dstools-infer-common 非独立 target。   │
│  IInferenceEngine 接口位于 dsfw::infer  │
│  命名空间（dsfw/infer/IInferenceEngine.h）│
│  OnnxEnv / OnnxModelBase / EP 选择     │
│  编译入 dsfw-infer（src/framework/infer/）│
│                                        │
│             │ PUBLIC                    │
│  ┌──────────┼──────────┬──────────────┬──────────────┐│
│  ▼          ▼          ▼              ▼              ▼│
│ game-infer rmvpe-infer hubert-infer  moe-infer   FunAsr
│ (SHARED)   (SHARED)   (SHARED)      (SHARED)     (STATIC)
│ (依赖 dsfw-audio)                      │
│                                        │
│  第三方引擎位于 src/engine/engines/     │
│  适配器桥接位于 src/engine/adapters/   │
└────────────────────────────────────────┘
```

### 各应用依赖明细

| 应用         | 直接链接的库                                                                                                                                                                                 |
|------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| LabelSuite | dsfw-widgets, dstools-domain, dsfw-audio-playback, cpp-pinyin, textgrid, FFTW3, SndFile, nlohmann_json                                                                                       |
| DsLabeler  | dsfw-widgets, dstools-domain, dsfw-audio-playback, hubert-infer, game-infer, rmvpe-infer, moe-infer, FunAsr, cpp-pinyin, textgrid, FFTW3, SndFile, nlohmann_json, Qt::Concurrent |

> **LabelSuite** 使用 `dsfw::AppShell` 多页面模式，11 个页面（Slice, ASR, Label, Align, Phone, CSV, MIDI, DS, Pitch,
> Settings, Log），各自使用 `DirectoryDataSource` 文件系统 I/O。**DsLabeler** 使用多页面模式，8 个页面（Welcome, Slicer,
> MinLabel, Phoneme, Pitch, Export, Settings, Log）由 `.dsproj`
> 工程文件驱动。详见 [unified-app-design.md](unified-app-design.md)。

---

## 6. 共享库

### dsfw-core (静态库)

通用框架核心。类型安全配置 (AppSettings)、服务定位器 (ServiceLocator)、异步任务 (AsyncTask)、结构化日志 (Logger)
、编码统一接口 (TextEncoding)、JSON 工具 (JsonHelper)、文档/文件/导出/G2P 抽象接口 (IDocument, IG2PProvider, IExportFormat 等)
、后端服务接口 (IAlignmentService, IAsrService, IPitchService, ITranscriptionService)。

依赖：dstools-types, Qt Core/Network, nlohmann_json

### dsfw-ui-core (静态库)

Qt 应用 UI 框架。统一窗口壳 (AppShell)、图标侧边导航 (IconNavBar)、主题管理 (Theme)、无边框窗口 (FramelessHelper)、页面接口 (
IPageActions/IPageLifecycle)。

依赖：dsfw-core, Qt Widgets, QWindowKit

### dstools-domain (静态库)

DiffSinger 领域逻辑。DsDocument/.ds 文件读写（含 SentenceView 值类型 API）、DsProject/.dsproj 项目、ModelManager (具体实现)
、F0 曲线、格式转换 (TextGrid↔CSV↔DS)、拼音 G2P、导出格式、质量评估、LayerData 强类型 (LayerDataVariant)。

依赖：dsfw-core, dstools-types, Qt Core/Gui/Network, nlohmann_json, textgrid (PRIVATE), SndFile (PRIVATE)

### dsfw-audio (静态库)

音频核心库（无 Qt 依赖）。AudioBuffer、AudioFormatInfo、ResampleConfig、FfmpegAudioDecoder（PIMPL）、SwresampleResampler（PIMPL）、
AudioPipeline（组合层）、AudioFileWriter。

依赖：dstools-types, dsfw-core (PRIVATE), FFmpeg, SDL2

### dsfw-audio-playback (静态库)

音频播放适配层。IAudioPlayerAdapter（std::function 回调）、AudioPlayerAdapter（具体实现）、AudioPlaybackAdapter（SDL2 PIMPL）。

依赖：dsfw-audio, dstools-types, Qt Core

### dsfw-widgets (动态库)

通用 GUI 组件。PlayWidget、FileProgressTracker、ProgressDialog、PropertyEditor、SettingsDialog、LogViewer 等。

依赖：dsfw-core (PUBLIC), dsfw-audio-playback (PUBLIC), dsfw-ui-core (PRIVATE)

### 推理库

| 库                    | 类型 | 功能                                                                         | 特有依赖                                          |
|----------------------|----|----------------------------------------------------------------------------|-----------------------------------------------|
| dsfw-infer           | 静态 | OnnxEnv 单例 + OnnxModelBase + IInferenceEngine + EP 选择 | dsfw-core, dsfw-types, onnxruntime, nlohmann_json |
| game-infer           | 动态 | GAME Audio→MIDI                                                            | dsfw-audio, wolf-midi, nlohmann_json |
| rmvpe-infer          | 动态 | RMVPE F0 提取                                                                | dsfw-audio                           |
| hubert-infer         | 动态 | HuBERT 强制对齐                                                                | dsfw-audio, nlohmann_json            |
| moe-infer            | 动态 | R3MOE 口型曲线预估                                                               | dsfw-audio, nlohmann_json            |
| FunAsr               | 静态 | FunASR Paraformer 中文 ASR                                                   | (直接链接 ORT)                                    |

> ¹ `dsfw-infer` 位于 `src/framework/infer/`，提供 IInferenceEngine 接口和 OnnxEnv/OnnxModelBase 实现。

### 第三方库

| 库        | 位置                                           |
|----------|----------------------------------------------|
| textgrid | src/libs/textgrid/ (header-only TextGrid 解析) |

---

## 7. 应用层结构

### 共享页面组件 (src/apps/shared/)

| Target            | 职责                                                                          |
|-------------------|-----------------------------------------------------------------------------|
| data-sources      | IEditorDataSource + Page + Service 统一入口                                     |
| audio-visualizer  | AudioVisualizerContainer + MiniMap + SliceTierLabel + AudioEditorWidgetBase |
| phoneme-editor    | 音素编辑 UI (含 TierLabelArea, ChartPanel, BoundaryOverlayWidget)                |
| pitch-editor      | 音高编辑 UI                                                                     |
| min-label-editor  | 歌词编辑 UI                                                                     |
| settings          | 设置 UI（CMake target: settings-page，目录: settings/）                            |
| log-page          | 日志查看 UI                                                                     |
| model-init        | 模型初始化注册                                                                     |
| mouth-curve-chart | 口型曲线图渲染                                                                     |

### Libs 层 (src/libs/) — CLI 可用的无 UI 接口

| 库（CMake 项目名）   | 用途                                      |
|----------------|-----------------------------------------|
| hubertfa-lib   | alignment decoding + ITaskProcessor 适配  |
| lyricfa-lib    | FunASR + 歌词匹配 + ITaskProcessor 适配       |
| slicer-lib     | RMS 切片算法 + ITaskProcessor 适配            |
| gameinfer-lib  | ITaskProcessor 适配（GAME MIDI）            |
| rmvpepitch-lib | ITaskProcessor 适配（RMVPE F0）             |
| minlabel-lib   | MinLabel 业务逻辑 + AddPhNum 处理器            |
| moelib         | R3MOE 口型曲线推理调度                          |
| infer-bridge   | 推理引擎桥接（domain ↔ infer 解耦，上接 ARCH-01） |

---

## 8. AppShell 统一壳

所有应用共享 `dsfw::AppShell`，不再各自实现 MainWindow。

**单页面模式**: IconNavBar 隐藏，表现为普通应用窗口

```cpp
AppShell shell("MinLabel");
shell.addPage(new MinLabelPage());
shell.show();
```

**多页面模式**: 显示侧边导航，页面切换时菜单栏/快捷键/状态栏自动跟随

```cpp
AppShell shell("DiffSinger Labeler");
shell.addPage(slicerPage, "S", "Slice");
shell.addPage(labelPage, "L", "Label");
shell.show();
```

壳与页面之间通过 `IPageLifecycle` / `IPageActions` 接口通信。

---

## 9. 松耦合策略

### 9.1 接口驱动

框架定义纯虚接口，应用层提供实现：

```
IDocument        ← DsDocumentAdapter
IModelProvider   ← GameModelProvider, HuBERTModelProvider
IG2PProvider     ← PinyinG2PProvider
IExportFormat    ← HtsLabelExportFormat, SinsyXmlExportFormat
IInferenceEngine ← GameEngine, HuBERTEngine, RmvpeEngine
```

### 9.2 ServiceLocator 依赖注入

```cpp
ServiceLocator::set<IG2PProvider>(new PinyinG2PProvider());
auto *g2p = ServiceLocator::get<IG2PProvider>();
```

### 9.3 CMake 消费

```cmake
find_package(dsfw REQUIRED)
target_link_libraries(myapp PRIVATE dsfw::core dsfw::ui-core)
```

---

## 10. 命名规范

命名规范详见 [conventions.md §2](../guides/conventions.md#2-命名规范)。

---

## 11. 框架接口一览

| 接口               | 层级      | 职责       | 实际实现                                                     |
|------------------|---------|----------|----------------------------------------------------------|
| IDocument        | core    | 文档生命周期   | DsDocumentAdapter (domain)                                |
| IModelProvider   | core    | 模型加载/卸载  | FunAsrModelProvider, GameModelProvider, HuBERTModelProvider (libs) |
| IG2PProvider     | core    | G2P 转换   | PinyinG2PProvider (domain)                                |
| IExportFormat    | core    | 数据导出     | HtsLabelExportFormat, SinsyXmlExportFormat (domain)       |
| ISliceDataSource | core    | 切片数据源    | ProjectDataSource (ds-labeler)                            |
| IAudioPlayer     | audio   | 音频播放     | AudioPlayer (audio)                                       |
| IInferenceEngine | infer   | 推理引擎     | GameEngine, HuBERTEngine, RmvpeEngine, MoeEngine, FunAsrEngine |

> **备注**：ModelManager 是具体类而非接口；QualityTypes 仅定义类型，无 IQualityMetrics 接口。

---

## 12. 构建顺序与目录结构

### 构建顺序

```
types → base → core → ui-core → audio → widgets → domain → libs → infer → apps → tests
```

所有推理库并行构建（仅共享 dstools-infer-common）。

### 目录结构

```
dataset-tools/
├── CMakeLists.txt              # 根配置: C++20, 输出目录, 编译器选项
├── cmake/
│   ├── DstoolsHelpers.cmake    # dstools_add_library() / dstools_add_executable() 宏
│   ├── deploy.cmake            # windeployqt/macdeployqt/Linux 部署逻辑
│   ├── infer-target.cmake      # dstools_add_infer_library() 宏
│   ├── setup-onnxruntime.cmake # ORT 下载脚本 (cpu/dml/gpu)
│   ├── dsfwConfig.cmake.in     # dsfw CMake 包配置模板
│   ├── dstools-typesConfig.cmake.in
│   └── winrc.cmake             # Windows 版本资源
├── src/
│   ├── types/                  # dstools-types (HEADER-ONLY)
│   │   └── include/dstools/    # Result<T>, ExecutionProvider, TimePos
│   ├── framework/
│   │   ├── core/               # dsfw-core (STATIC)
│   │   │   ├── include/dsfw/   # AppSettings, ServiceLocator, Logger, ...
│   │   │   └── src/
│   │   ├── ui-core/            # dsfw-ui-core (STATIC)
│   │   │   ├── include/dsfw/   # AppShell, Theme, FramelessHelper, IPageActions, ...
│   │   │   ├── src/
│   │   │   └── res/            # 主题 QSS, 资源文件
│   │   ├── audio/              # dsfw-audio (core, STATIC)
│   │   │   └── playback/       # dsfw-audio-playback (STATIC)
│   │   ├── infer/              # dsfw-infer (STATIC)
│   │   └── widgets/            # dsfw-widgets (SHARED)
│   ├── domain/                 # dstools-domain (STATIC)
│   │   ├── include/dstools/    # DsDocument, DsProject, CsvToDsConverter, ...
│   │   └── src/
│   ├── ui-core/                # dstools-ui-core (STATIC, 包装 dsfw-ui-core + dsfw-core + dstools-domain)
│   ├── engine/
│   │   ├── adapters/              # 引擎适配器/桥接层
│   │   │   ├── infer-bridge/     # InferBridge 统一入口
│   │   │   ├── lyric-fa/         # 歌词对齐适配器
│   │   │   ├── hubert-fa/        # HuBERT 强制对齐适配器
│   │   │   ├── game-infer-lib/   # GAME MIDI 适配器
│   │   │   ├── rmvpe-pitch/      # RMVPE F0 适配器
│   │   │   ├── min-label-lib/    # MinLabel 服务适配器
│   │   │   ├── slicer-lib/       # RMS 切片服务适配器
│   │   │   └── moe-lib/          # R3MOE 口型曲线适配器
│   │   └── engines/               # 推理引擎
│   │       ├── onnxruntime/       # 预下载 ORT 二进制
│   │       ├── game-infer/        # (SHARED)
│   │       ├── rmvpe-infer/       # (SHARED)
│   │       ├── hubert-infer/      # (SHARED)
│   │       ├── moe-infer/         # (SHARED)
│   │       └── FunAsr/            # (STATIC)
│   ├── apps/
│   │   ├── label-suite/            # LabelSuite — 通用标注工具集
│   │   ├── ds-labeler/             # DsLabeler — DiffSinger 专用标注器
│   │   ├── shared/                 # 共享编辑器组件
│   │   │   ├── audio-visualizer/   # 统一音频可视化容器 (AudioVisualizerContainer)
│   │   │   ├── data-sources/       # 数据源 + Page + Service
│   │   │   ├── min-label-editor/   # 歌词编辑 UI 组件
│   │   │   ├── phoneme-editor/     # 音素编辑 UI 组件 (含 TierLabelArea)
│   │   │   ├── pitch-editor/       # 音高编辑 UI 组件
│   │   │   ├── settings/           # 设置 UI
│   │   │   ├── log-page/           # 日志查看 UI
│   │   │   ├── chart-framework/   # 图表框架基类 (ChartConfigRegistry, ChartPanelBase)
│   │   │   ├── bridges/           # 文档桥接层 (DsTextDocBridge)
│   │   │   └── mouth-curve-chart/  # 口型曲线图渲染
│   │   ├── cli/                    # dstools-cli
│   │   └── widget-gallery/         # WidgetGallery
│   └── tests/
│       ├── framework/          # dsfw 核心类单元测试
│       └── CMakeLists.txt      # 推理库测试注册
├── vcpkg.json                  # vcpkg 依赖声明（项目根目录）
└── docs/
```

---

## 13. CMake 目标总览

```
~42 CMake targets (excluding tests):

Framework (7):  dsfw-signal → dsfw-core → dsfw-ui-core → dsfw-widgets, dsfw-audio, dsfw-audio-playback, dsfw-infer
Domain (1):     dstools-domain
App-Lib (1):    dstools-ui-core
Infer (5):      game-infer, hubert-infer, rmvpe-infer, moe-infer, FunAsr
Adapters (8):   slicer-lib, lyricfa-lib, hubertfa-lib, gameinfer-lib, rmvpepitch-lib, minlabel-lib, moelib, infer-bridge
App-Shared (9): data-sources, audio-visualizer, phoneme-editor, pitch-editor, min-label-editor, settings, log-page, model-init, mouth-curve-chart
Apps (5):       LabelSuite, DsLabeler, dstools-cli, WidgetGallery, TestShell
Header-Only (2): dstools-types, textgrid (no build output)
```

CI 验证模块独立构建。`find_package(dsfw)` 集成测试存在。

---
