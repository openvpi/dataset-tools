# 项目架构

> 编码规范见 [conventions-and-standards.md](conventions-and-standards.md)。测试架构见 [test-design.md](test-design.md)。设计准则与决策记录见 [human-decisions.md](human-decisions.md)。

---

## 1. 设计准则（最高优先级）

以下准则约束所有框架和应用层代码，详见 [human-decisions.md](human-decisions.md) P-01 ~ P-07 和 [conventions-and-standards.md](conventions-and-standards.md)。

| 编号 | 准则 | 要求 |
|---|---|---|
| **P-01** | 模块职责单一 | 相同行为只存在一处；刷新/通知由容器统一分发，不散落在各消费者 |
| **P-02** | 被动接口 + 容器通知 | 纯数据接口不加 QObject；变更通知由容器的 `invalidateXxx()` 负责 |
| **P-03** | 异步一切 | 超过 50ms 的操作禁止主线程同步执行；禁止 `processEvents` 反模式 |
| **P-04** | 错误根因传播 | 错误消息必须追溯到根因，不得忽略 out-parameter 继续报二次错误 |
| **P-05** | 异常边界隔离 | `Result<T>` 传播应用层错误；`try-catch` 仅限第三方库边界 |
| **P-06** | 接口稳定 | 公共头文件即契约；框架接口变更需考虑向后兼容 |
| **P-07** | 简洁可靠 | 遇错直接返回，不设计重试或回滚（除明确需求外） |

**模式示例**：`AudioVisualizerContainer::invalidateBoundaryModel()` — 一次调用刷新 overlay + tier label + 所有 chart，消费者无需知道内部有哪些 widget。

---

## 2. 技术栈

| 维度 | 选型 |
|------|------|
| 语言 | C++20 |
| GUI | Qt 6.8+ (Core, Widgets, Svg, Network, Concurrent) |
| 推理 | ONNX Runtime (DirectML / CUDA / CPU) |
| 构建 | CMake ≥ 3.21 + vcpkg |
| 音频 | FFmpeg (解码) + SDL2 (播放) + SndFile + mpg123 + soxr (重采样) |
| 平台 | Windows 10/11 (主), macOS 11+, Linux |

---

## 3. 框架分离原则

项目拆分为：
- **dsfw（通用框架层）**: 可复用的 C++20/Qt6 桌面应用框架，不含歌声合成领域逻辑
- **dstools-app（应用层）**: 基于 dsfw 的 DiffSinger 数据集处理工具

| 判断标准 | 归入框架 | 归入应用层 |
|---------|---------|-----------|
| 涉及 .ds/.dsproj 格式 | 否 | 是 |
| 涉及特定 G2P（拼音、假名） | 否 | 是 |
| 涉及特定 AI 模型 | 否 | 是 |
| 可脱离上述概念独立运行 | 是 | 否 |

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

## 4. 六层架构

```
Layer 5 ─ dsfw-infer       OnnxEnv, IInferenceEngine
Layer 4 ─ dsfw-widgets     通用 UI 组件 (SHARED DLL)
Layer 3 ─ dsfw-ui-core     AppShell, IconNavBar, Theme, FramelessHelper, IPageActions
Layer 2 ─ dsfw-audio       AudioDecoder (FFmpeg), AudioPlayback (SDL2)
Layer 1 ─ dsfw-core        AppSettings, ServiceLocator, AsyncTask, JsonHelper, 接口集
                           Logger, FileLogSink, CrashHandler, AppPaths, BatchCheckpoint
                           PipelineContext, PipelineRunner, ITaskProcessor
Layer 0.5─ dsfw-base       JsonHelper (Qt-free 静态库)
Layer 0 ─ dsfw-types       Result<T>, ExecutionProvider, TimePos (header-only)
```

### 依赖关系

```
dsfw-widgets → dsfw-ui-core → dsfw-core → dsfw-types
                                          ↑
dsfw-widgets → dsfw-audio                dsfw-infer
```

---

## 5. 模块依赖图

```
┌──────────────────────────────────────────────────────────────────────────┐
│                           应用层 (src/apps/)                             │
├──────────────────────────────────────────────────────────────────────────┤
│ LabelSuite (通用标注工具集, 9 页面)                                        │
│   Slice · ASR · Label · Align · Phone · CSV · MIDI · DS · Pitch        │
│                                                                          │
│ DsLabeler (DiffSinger 专用标注器, 7 页面)                                  │
│   WelcomePage · DsSlicerPage · DsMinLabelPage                           │
│   DsPhonemeLabelerPage · DsPitchLabelerPage · ExportPage · SettingsPage │
│                                                                          │
│ dstools-cli · TestShell · WidgetGallery                                  │
└─────┬──────┴────┬─────┴─────┬──────┴─────┬──────┴────┬─────┴─────┬──────┘
      │           │           │            │           │           │
      ▼           ▼           ▼            ▼           ▼           ▼
┌──────────────────────────────────────────────────────────────────────────┐
│                      dstools-widgets (SHARED DLL)                        │
│  GpuSelector · ShortcutManager · 领域 UI 组件                           │
└────────────────────────────┬─────────────────────────────────────────────┘
                             │ PUBLIC
┌────────────────────────────┴─────────────────────────────────────────────┐
│                      dsfw-widgets (SHARED DLL)                           │
│  PlayWidget · FileProgressTracker · ProgressDialog · PropertyEditor     │
│  SettingsDialog · LogViewer · RunProgressRow · PathSelector · ...        │
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
│ PipelineContext   │                    │
│ PipelineRunner    │                    │
│ ITaskProcessor    │                    │
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
│ ModelManager      │ │
│ CsvToDsConverter  │ │
│ TextGridToCsv     │ │
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
│  TimePos          │
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

### 各应用依赖明细

| 应用 | 直接链接的库 |
|------|-------------|
| LabelSuite | dstools-widgets, dstools-domain, dstools-audio, cpp-pinyin, cpp-kana, textgrid, FFTW3, SndFile, nlohmann_json |
| DsLabeler | dstools-widgets, dstools-domain, dstools-audio, audio-util, hubert-infer, game-infer, rmvpe-infer, FunAsr, cpp-pinyin, cpp-kana, textgrid, FFTW3, SndFile, nlohmann_json, Qt::Concurrent |

> **LabelSuite** 使用 `dsfw::AppShell` 多页面模式，10 个标注页面（Slice, ASR, Label, Align, Phone, CSV, MIDI, DS, Pitch, Settings），各自使用文件系统 I/O。**DsLabeler** 使用多页面模式，7 个页面（Welcome, Slicer, MinLabel, PhonemeLabeler, PitchLabeler, Export, Settings）由 `.dsproj` 工程文件驱动。详见 [unified-app-design.md](unified-app-design.md)。

---

## 6. 共享库

### dsfw-base (静态库, Qt-free)

JSON 工具 (JsonHelper，纯 nlohmann/json 封装)。无 Qt 依赖，可用于 CLI 工具。

依赖：nlohmann_json

### dsfw-core (静态库)

通用框架核心。类型安全配置 (AppSettings)、服务定位器 (ServiceLocator)、异步任务 (AsyncTask)、结构化日志 (Logger)、文档/文件/模型/导出/G2P/质量评估抽象接口 (IDocument, IModelManager, IModelDownloader 等)、后端服务接口 (ISlicerService, IAlignmentService, IAsrService, IPitchService, ITranscriptionService)。

依赖：dsfw-base, dstools-types, Qt Core/Network, nlohmann_json

### dsfw-ui-core (静态库)

Qt 应用 UI 框架。统一窗口壳 (AppShell)、图标侧边导航 (IconNavBar)、主题管理 (Theme)、无边框窗口 (FramelessHelper)、页面接口 (IPageActions/IPageLifecycle)。

依赖：dsfw-core, Qt Widgets, QWindowKit

### dstools-domain (静态库)

DiffSinger 领域逻辑。DsDocument/.ds 文件读写、DsProject/.dsproj 项目、ModelManager (具体实现)、F0 曲线、格式转换 (TextGrid↔CSV↔DS)、拼音 G2P、导出格式、质量评估。

依赖：dsfw-core, dstools-types, Qt Core/Gui/Network, nlohmann_json, textgrid (PRIVATE), SndFile (PRIVATE)

### dstools-audio (静态库)

AudioDecoder (FFmpeg)、AudioPlayback (SDL2)、AudioPlayer、WaveFormat。

依赖：Qt Core, FFmpeg, SDL2

### dsfw-widgets (动态库)

通用 GUI 组件。PlayWidget、FileProgressTracker、ProgressDialog、PropertyEditor、SettingsDialog、LogViewer 等。

依赖：dsfw-core (PUBLIC), dsfw-ui-core + dstools-audio (PRIVATE)

### dstools-widgets (动态库)

DiffSinger 领域 UI 组件。所有应用的公共 UI 基础设施。

依赖：dsfw-widgets (PUBLIC)

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

---

## 7. 应用层结构

### 共享页面组件 (src/apps/shared/)

| Target | 职责 |
|--------|------|
| data-sources | IEditorDataSource + Page + Service 统一入口 |
| audio-visualizer | AudioVisualizerContainer + MiniMap + SliceTierLabel |
| phoneme-editor | 音素编辑 UI (含 TierLabelArea, PhonemeTextGridTierLabel) |
| pitch-editor | 音高编辑 UI |
| min-label-editor | 歌词编辑 UI |
| settings-page | 设置 UI |

### Libs 层 (src/libs/) — CLI 可用的无 UI 接口

| 库 | 用途 |
|----|------|
| hubertfa-lib | alignment decoding + ITaskProcessor 适配 |
| lyricfa-lib | FunASR + 歌词匹配 + ITaskProcessor 适配 |
| slicer-lib | RMS 切片算法 |
| gameinfer-lib | ITaskProcessor 适配（GAME MIDI） |
| rmvpepitch-lib | ITaskProcessor 适配（RMVPE F0） |
| minlabel-lib | MinLabel 业务逻辑 |

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

| 模块 | CMake target | namespace | include 前缀 |
|------|-------------|-----------|-------------|
| dsfw-types | dsfw::types | dstools | `<dstools/Result.h>` |
| dsfw-base | dsfw::base | dstools | `<dsfw/JsonHelper.h>` |
| dsfw-core | dsfw::core | dstools | `<dsfw/AppSettings.h>` |
| dsfw-audio | dsfw::audio | dstools::audio | `<dstools/AudioDecoder.h>` |
| dsfw-ui-core | dsfw::ui-core | dsfw (AppShell/Theme), dstools::labeler (IPageActions/IPageLifecycle) | `<dsfw/AppShell.h>` |
| dsfw-widgets | dsfw::widgets | dsfw::widgets | `<dsfw/widgets/PlayWidget.h>` |
| dsfw-infer | dsfw::infer | dsfw::infer | — |
| dstools-domain | — | dstools | `<dstools/DsDocument.h>` |

```
框架: #include <dsfw/AppSettings.h>    namespace dstools
框架: #include <dsfw/AppShell.h>       namespace dsfw
框架: #include <dsfw/widgets/...>      namespace dsfw::widgets
应用: #include <dstools/DsDocument.h>   namespace dstools
```

---

## 11. 框架接口一览

| 接口 | 层级 | 职责 | 默认实现 |
|------|------|------|----------|
| IDocument | core | 文档生命周期 | — |
| IFileIOProvider | core | 文件 I/O | LocalFileIOProvider |
| IModelProvider | core | 模型加载/卸载 | — |
| IModelDownloader | core | 模型下载 | ModelDownloader, StubModelDownloader |
| IG2PProvider | core | G2P 转换 | StubG2PProvider |
| IExportFormat | core | 数据导出 | StubExportFormat |
| IQualityMetrics | core | 质量评估 | StubQualityMetrics |
| IAudioPlayer | audio | 音频播放 | AudioPlayer |
| IStepPlugin | ui-core | 步骤插件 | StubStepPlugin |
| IInferenceEngine | infer | 推理引擎 | — |

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
│   │   ├── base/                # dsfw-base (STATIC, Qt-free)
│   │   │   └── include/dsfw/   # JsonHelper
│   │   ├── core/               # dsfw-core (STATIC)
│   │   │   ├── include/dsfw/   # AppSettings, ServiceLocator, Logger, ...
│   │   │   └── src/
│   │   ├── ui-core/            # dsfw-ui-core (STATIC)
│   │   │   ├── include/dsfw/   # AppShell, Theme, FramelessHelper, IPageActions, ...
│   │   │   ├── src/
│   │   │   └── res/            # 主题 QSS, 资源文件
│   │   ├── audio/              # dstools-audio (STATIC)
│   │   └── widgets/            # dsfw-widgets (SHARED)
│   ├── domain/                 # dstools-domain (STATIC)
│   │   ├── include/dstools/    # DsDocument, DsProject, CsvToDsConverter, ...
│   │   └── src/
│   ├── widgets/                # dstools-widgets (SHARED)
│   ├── libs/
│   │   ├── textgrid/          # header-only
│   │   ├── hubert-fa/         # HuBERT 强制对齐处理器
│   │   ├── lyric-fa/          # 歌词对齐处理器
│   │   ├── game-infer-lib/    # GAME MIDI 处理器
│   │   ├── rmvpe-pitch/       # RMVPE F0 处理器
│   │   └── min-label-lib/     # MinLabel 服务 + AddPhNum 处理器
│   ├── infer/
│   │   ├── common/             # dstools-infer-common (STATIC)
│   │   ├── onnxruntime/        # 预下载 ORT 二进制
│   │   ├── audio-util/         # (SHARED, 独立可安装)
│   │   ├── game-infer/         # (SHARED)
│   │   ├── rmvpe-infer/        # (SHARED)
│   │   ├── hubert-infer/       # (SHARED)
│   │   └── FunAsr/             # (STATIC)
│   ├── apps/
│   │   ├── label-suite/            # LabelSuite — 通用标注工具集
│   │   ├── ds-labeler/             # DsLabeler — DiffSinger 专用标注器
│   │   ├── shared/                 # 共享编辑器组件
│   │   │   ├── audio-visualizer/   # 统一音频可视化容器 (AudioVisualizerContainer)
│   │   │   ├── data-sources/       # 数据源 + Page + Service
│   │   │   ├── min-label-editor/   # 歌词编辑 UI 组件
│   │   │   ├── phoneme-editor/     # 音素编辑 UI 组件 (含 TierLabelArea)
│   │   │   ├── pitch-editor/       # 音高编辑 UI 组件
│   │   │   └── settings/           # 设置 UI
│   │   ├── cli/                    # dstools-cli
│   │   ├── test-shell/             # TestShell
│   │   └── widget-gallery/         # WidgetGallery
│   └── tests/
│       ├── framework/          # dsfw 核心类单元测试
│       └── CMakeLists.txt      # 推理库测试注册
├── scripts/vcpkg-manifest/     # vcpkg 依赖声明
└── docs/
```

---

## 13. CMake 目标总览

```
~33 CMake targets (excluding tests):

Framework (6):  dsfw-base → dsfw-core → dsfw-ui-core → dsfw-widgets + dstools-audio + dstools-infer-common
Domain (1):     dstools-domain
Infer (5):      audio-util, FunAsr, game-infer, hubert-infer, rmvpe-infer
Libs (7):       slicer-lib, lyricfa-lib, hubertfa-lib, gameinfer-lib, rmvpepitch-lib, minlabel-lib, textgrid
App-Shared (6): data-sources, audio-visualizer, phoneme-editor, pitch-editor, min-label-editor, settings-page
Apps (4+):      LabelSuite, DsLabeler, dstools-cli, WidgetGallery
```

CI 验证模块独立构建。`find_package(dsfw)` 集成测试存在。

---

## 14. 迁移状态

| 阶段 | 状态 |
|------|------|
| 接口泛化 (DocumentFormat/ModelType 枚举) | ✅ 已完成 |
| 独立仓库 | ⏳ 待定（当前单仓库模式，ADR-8） |

---

## 15. 已知架构问题

1. **大类**：DsSlicerPage (~945 行), PitchLabelerPage (~707 行) — 多数已做 service 提取，DsSlicerPage 仍较大
2. **线程安全**：`QtConcurrent::run` 在 Page 类中捕获 `this` — 已使用 QPointer guard 模式

---

## 代码质量观察

- 头文件规范良好（无 `using namespace` in headers）
- 无废弃代码块（`#if 0`）
- 无遗留 TODO
- CI 覆盖多平台、模块独立性验证、clang-tidy
- 框架可通过 `find_package(dsfw)` 独立消费
- 32+ 测试文件覆盖 domain + framework core
