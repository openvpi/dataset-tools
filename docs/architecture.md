# DiffSinger Dataset Tools — 架构设计文档

> **组织**: Team OpenVPI
> **许可证**: Apache 2.0

---

## 1. 产品概述

### 1.1 产品定位

DiffSinger Dataset Tools 是一套面向歌声合成数据集制作的桌面工具集。它覆盖了从原始音频切分、歌词对齐、音素标注，到 MIDI 估计的完整工作流，为 DiffSinger 等歌声合成模型的训练数据准备提供支撑。

### 1.2 目标用户

- 歌声合成数据集制作者
- DiffSinger 模型训练者
- 语音/音乐研究人员

### 1.3 产品组成

工具集包含 **5 个独立应用程序**：

| 应用 | 中文名称 | 用途 |
|------|----------|------|
| DatasetPipeline.exe | 数据集处理管线 | 音频切分、歌词对齐、音素对齐 |
| MinLabel.exe | 音频标注工具 | G2P 转换、JSON 标注编辑 |
| PhonemeLabeler.exe | TextGrid 音素边界编辑器 | 波形/频谱/能量可视化、跨层边界绑定 |
| PitchLabeler.exe | DS 文件 F0 曲线编辑器 | 钢琴卷帘可视化、多工具编辑 |
| GameInfer.exe | GAME 音频转 MIDI | 基于 ONNX 的四模型推理管线 |

### 1.4 重构动因

| 问题 | 影响 |
|------|------|
| 6 个 EXE + 30+ DLL 平铺在一个目录 | 用户困惑 |
| AudioSlicer / LyricFA / HubertFA 是数据集制作的连续流程，却要分别启动 | 工作流割裂 |
| PlayWidget 75% 重复、DmlGpuUtils 100% 重复、EP 枚举 4 份 | 维护成本 |
| qsmedia 插件/工厂体系从未使用 | 过度设计 |
| F0Widget 1097 行单文件 | 低内聚 |
| 53 个已知 Bug（含 5 个 Critical） | 运行时风险 |
| 各应用字体/样式/主题不统一，无深色模式 | 视觉不一致 |

### 1.5 重构原则

1. **功能守恒** — 所有工具的输入/输出/行为不变
2. **按使用场景分组** — 数据集管线一体化，创作工具独立
3. **共享样式，独立进程** — 5 个 EXE 通过共享库获得统一字体/主题/深色模式
4. **高内聚低耦合** — 消除重复，明确模块边界
5. **渐进式** — 按阶段实施，每阶段可独立验证

### 1.6 应用分组逻辑

```
数据集制作的典型工作流：

  原始长音频 ──→ AudioSlicer（切片）
                     │
              短音频片段 ──→ LyricFA（歌词识别+对齐）
                                │
                         标注JSON ──→ HubertFA（音素强制对齐）
                                        │
                                   TextGrid（训练数据）

这三个工具是数据集处理的流水线，应合并为一个「Dataset Pipeline」应用。
三个步骤使用 Tab 分页隔开，各自独立配置和运行，**不提供一键串联**。
用户按需切换 Tab，逐步完成流水线。上一步的输出目录可手动设为下一步的输入目录。

而 MinLabel（手工标注）、PhonemeLabeler（音素边界编辑）、
PitchLabeler（F0曲线编辑）、GameInfer（音频转MIDI）
是独立的创作/编辑工具，各有不同的使用场景和窗口布局，适合作为独立 EXE。
```

| 应用 | 使用场景 | 分组依据 |
|------|----------|----------|
| AudioSlicer | 数据集批处理 — 第 1 步 | 三者是顺序流水线 |
| LyricFA | 数据集批处理 — 第 2 步 | 输入来自上一步输出 |
| HubertFA | 数据集批处理 — 第 3 步 | 输入来自上一步输出 |
| MinLabel | 手工交互：逐文件标注 | 独立使用场景 |
| PhonemeLabeler | 手工交互：TextGrid 音素边界编辑 | 独立使用场景 |
| PitchLabeler | 手工交互：钢琴卷帘 F0 编辑 | 独立使用场景 |
| GameInfer | 单文件推理 | 独立使用场景 |

---

## 2. 架构设计

### 2.1 应用拓扑

```
5 个可执行文件:

  ┌──────────────────────────────────────────────┐
  │ DatasetPipeline.exe                           │
  │  ┌──────────┬──────────┬──────────┐          │
  │  │AudioSlicer│ LyricFA │ HubertFA │ ← Tab分页 │
  │  │  (Tab 1)  │(Tab 2)  │ (Tab 3)  │          │
  │  └──────────┴──────────┴──────────┘          │
  │  各 Tab 独立操作，不提供一键串联               │
  └──────────────────────────────────────────────┘

  ┌──────────────┐ ┌──────────────────┐ ┌──────────────────┐ ┌──────────────┐
  │ MinLabel.exe  │ │PhonemeLabeler.exe│ │ PitchLabeler.exe │ │ GameInfer.exe│
  │  音频标注      │ │ 音素边界编辑     │ │  F0 曲线编辑     │ │ 音频转MIDI   │
  └──────────────┘ └──────────────────┘ └──────────────────┘ └──────────────┘

  所有 5 个 EXE 共享:
  ┌─────────────────────────────────────────────┐
  │         dstools-widgets (统一样式库)          │
  │  PlayWidget │ TaskWindow │ GpuSelector│
  ├─────────────────────────────────────────────┤
  │         dstools-audio (音频层)               │
  │  AudioDecoder (FFmpeg) │ AudioPlayback (SDL2)│
  ├─────────────────────────────────────────────┤
  │         dstools-core (核心层)                │
  │  AppInit │ Config │ ErrorHandling │ Theme    │
  └─────────────────────────────────────────────┘
```

### 2.2 层次图

```
┌──────────────────────────────────────────────────────────────────────────────────┐
│ DatasetPipeline │ MinLabel │ PhonemeLabeler │ PitchLabeler │ GameInfer            │
├─────────────────┴──────────┴────────────────┴──────────────┴─────────────────────┤
│                            Shared Widgets Layer                                   │
│  PlayWidget │ TaskWindow │ GpuSelector │ (dstools-widgets.dll)                    │
├──────────────────────────────────────────────────────────────────────────────────┤
│                              Audio Layer                                          │
│  AudioDecoder (FFmpeg) │ AudioPlayback (SDL2) │ (dstools-audio)                   │
├──────────────────────────────────────────────────────────────────────────────────┤
│                            Inference Layer                                        │
│  OnnxEnv │ game-infer │ some-infer │ rmvpe-infer │ FunAsr                         │
│          │ audio-util │                                                           │
├──────────────────────────────────────────────────────────────────────────────────┤
│                              Core Layer                                           │
│  AppInit │ AppSettings │ DsDocument │ JsonHelper │ Theme │ FramelessHelper        │
├──────────────────────────────────────────────────────────────────────────────────┤
│              External: Qt 6.8+, ONNX Runtime, SDL2, FFmpeg, FFTW3                 │
└──────────────────────────────────────────────────────────────────────────────────┘
```

### 2.3 依赖方向

```
Apps (5 EXE) ──→ dstools-widgets ──→ dstools-audio ──→ dstools-core
    │                                     │
    └──→ Inference libs ─────────────────┘
              │
         (onnxruntime, fftw3, sndfile, soxr...)
```

规则：
- 箭头方向不可逆转
- 同层 App 不互相依赖
- dstools-widgets 是 SHARED 库（DLL），5 个 EXE 共享一份

### 2.4 库类型

| 库 | 类型 | 说明 |
|----|------|------|
| dstools-core | STATIC | 应用初始化、配置、JSON 工具 |
| dstools-audio | STATIC | 音频解码与播放 |
| dstools-widgets | SHARED (DLL) | 可视化控件、主题系统，五个应用共享 |
| dstools-infer-common | STATIC | ONNX Runtime 封装、ExecutionProvider 管理 |
| audio-util | SHARED | 音频分析工具（F0 提取等） |
| game-infer | SHARED | GAME 模型推理 |
| some-infer | SHARED | SOME MIDI 估计推理 |
| rmvpe-infer | SHARED | RMVPE F0 估计推理 |
| textgrid | HEADER-ONLY | Praat TextGrid 读写（src/libs/textgrid） |
| FunAsr | STATIC | FunASR Paraformer 中文语音识别 |
| dstools-core 内含 JsonHelper | — | nlohmann::json 安全封装（路径解析、类型安全访问、原子文件 I/O） |

> `dstools-widgets` 通过 `WidgetsGlobal.h` 定义 `DSTOOLS_WIDGETS_API` 导出宏（`Q_DECL_EXPORT` / `Q_DECL_IMPORT`）。

### 2.5 消除重复方案

| 重复代码 | 当前 | 合并到 | 消除方式 |
|----------|------|--------|----------|
| PlayWidget (75% 重复) | MinLabel + PitchLabeler 各一份 | `dstools-widgets/PlayWidget` | 以 PitchLabeler 版为基础合并 |
| DmlGpuUtils (100% 重复) | GameInfer + HubertFA 各一份 | `dstools-widgets/GpuSelector` | 合并为 QComboBox 组件 |
| ExecutionProvider (100%×4) | 4 个推理库各一份 | `dstools-infer-common/ExecutionProvider.h` | 统一枚举 |
| main.cpp 初始化 (~20行×6) | 6 个 main.cpp | `dstools-core/AppInit` | 统一初始化函数 |
| app.qss (100% 重复) | MinLabel + PitchLabeler 各一份 | `resources/themes/` | 全局主题文件 |
| AsyncTaskWindow | 独立库 | `dstools-widgets/TaskWindow` | 归入共享层 |
| qsmedia 插件体系 | 3 个库 + 2 个插件 | `dstools-audio` | 直接集成，删除插件/工厂 |

---

## 3. 目录结构

```
dataset-tools/
├── CMakeLists.txt
├── cmake/                            # 构建工具（原 scripts/cmake/ 提升）
│   ├── utils.cmake
│   ├── winrc.cmake
│   └── setup-onnxruntime.cmake
├── vcpkg/                            # vcpkg 配置（原 scripts/vcpkg-* 合并）
│   ├── vcpkg.json
│   ├── ports/
│   └── triplets/
├── resources/                        # 全局资源（所有 EXE 共享）
│   ├── icons/                        # 统一 SVG 图标
│   ├── themes/
│   │   ├── dark.qss
│   │   └── light.qss
│   └── resources.qrc
│
│   > **注意**: `resources/icons/` 和 `resources/themes/` 目录已创建但**尚未填充内容**（dark.qss/light.qss/SVG 图标均为规划中）。QSS 主题当前由 `src/core/res/` 内的资源提供。
│
├── src/
│   ├── CMakeLists.txt
│   │
│   ├── core/                         # Core Layer
│   │   ├── CMakeLists.txt            # → dstools-core (STATIC)
│   │   ├── include/dstools/
│   │   │   ├── AppInit.h             # 统一初始化（字体/root/crash）
│   │   │   ├── AppSettings.h         # 类型安全配置（SettingsKey<T>）
│   │   │   ├── DsDocument.h          # 文档模型基类
│   │   │   ├── FramelessHelper.h     # 无边框窗口装饰
│   │   │   ├── JsonHelper.h          # nlohmann::json 安全封装
│   │   │   ├── CommonKeys.h          # 通用配置键
│   │   │   └── Theme.h               # 主题管理 + 色板
│   │   └── src/
│   │
│   ├── audio/                        # Audio Layer
│   │   ├── CMakeLists.txt            # → dstools-audio (STATIC)
│   │   ├── include/dstools/
│   │   │   ├── AudioDecoder.h        # FFmpeg 直接集成
│   │   │   ├── AudioPlayback.h       # SDL2 直接集成
│   │   │   └── WaveFormat.h
│   │   └── src/
│   │
│   ├── widgets/                      # Shared Widgets Layer
│   │   ├── CMakeLists.txt            # → dstools-widgets (SHARED/DLL)
│   │   ├── include/dstools/
│   │   │   ├── WidgetsGlobal.h       # DSTOOLS_WIDGETS_API 导出宏
│   │   │   ├── PlayWidget.h          # 合并两份 PlayWidget
│   │   │   ├── TaskWindow.h          # 原 AsyncTaskWindow
│   │   │   ├── GpuSelector.h         # 合并两份 DmlGpuUtils
│   │   │   ├── ShortcutEditorWidget.h
│   │   │   ├── FileProgressTracker.h
│   │   │   └── FileStatusDelegate.h
│   │   └── src/
│   │
│   ├── libs/
│   │   └── textgrid/                 # textgrid.hpp (HEADER-ONLY)
│   │
│   ├── infer/                        # Inference Layer
│   │   ├── common/                   # → dstools-infer-common (STATIC)
│   │   │   └── include/dstools/
│   │   │       ├── OnnxEnv.h
│   │   │       └── ExecutionProvider.h
│   │   ├── audio-util/               # → audio-util (SHARED)，保留
│   │   ├── game-infer/               # → game-infer (SHARED)，保留
│   │   ├── some-infer/               # → some-infer (SHARED)，保留
│   │   ├── rmvpe-infer/              # → rmvpe-infer (SHARED)，保留
│   │   ├── funasr/                   # → FunAsr (STATIC)，修复 Bug
│   │   └── onnxruntime/              # 预编译 ONNX Runtime
│   │
│   ├── apps/
│   │   ├── CMakeLists.txt
│   │   │
│   │   ├── pipeline/                 # ★ DatasetPipeline.exe
│   │   │   ├── CMakeLists.txt
│   │   │   ├── main.cpp
│   │   │   ├── PipelineWindow.h/.cpp # 主窗口：Tab 分页
│   │   │   ├── slicer/              # AudioSlicer Tab（不继承 TaskWindow）
│   │   │   │   ├── SlicerPage.h/.cpp
│   │   │   │   ├── Slicer.h/.cpp
│   │   │   │   ├── WorkThread.h/.cpp
│   │   │   │   ├── MathUtils.h
│   │   │   │   ├── Enumerations.h
│   │   │   │   └── WinFont.h/.cpp    # Windows 字体工具
│   │   │   ├── lyricfa/             # LyricFA 步骤
│   │   │   │   ├── LyricFAPage.h/.cpp
│   │   │   │   ├── AsrTask.h/.cpp
│   │   │   │   ├── LyricMatchTask.h/.cpp
│   │   │   │   ├── Asr.h/.cpp
│   │   │   │   ├── MatchLyric.h/.cpp
│   │   │   │   ├── LyricMatcher.h/.cpp
│   │   │   │   ├── LyricData.h         # 歌词数据结构
│   │   │   │   ├── Utils.h/.cpp         # 通用工具函数
│   │   │   │   ├── SequenceAligner.h/.cpp
│   │   │   │   ├── ChineseProcessor.h/.cpp
│   │   │   │   └── SmartHighlighter.h/.cpp
│   │   │   └── hubertfa/            # HubertFA 步骤
│   │   │       ├── HubertFAPage.h/.cpp
│   │   │       └── HfaTask.h/.cpp
│   │   │
│   │   ├── MinLabel/                 # ★ MinLabel.exe
│   │   │   ├── CMakeLists.txt
│   │   │   ├── main.cpp
│   │   │   └── gui/
│   │   │       ├── MainWindow.h/.cpp     # 保持 QMainWindow
│   │   │       ├── TextWidget.h/.cpp
│   │   │       ├── ExportDialog.h/.cpp
│   │   │       └── Common.h/.cpp
│   │   │
│   │   ├── PhonemeLabeler/           # ★ PhonemeLabeler.exe
│   │   │   ├── CMakeLists.txt
│   │   │   ├── main.cpp
│   │   │   ├── PhonemeLabelerKeys.h
│   │   │   └── gui/
│   │   │       ├── MainWindow.h/.cpp
│   │   │       └── ui/
│   │   │           ├── TextGridDocument.h/.cpp
│   │   │           ├── ViewportController.h/.cpp
│   │   │           ├── BoundaryBindingManager.h/.cpp
│   │   │           ├── WaveformWidget.h/.cpp
│   │   │           ├── SpectrogramWidget.h/.cpp
│   │   │           ├── PowerWidget.h/.cpp
│   │   │           ├── IntervalTierView.h/.cpp
│   │   │           ├── TierEditWidget.h/.cpp
│   │   │           ├── BoundaryOverlayWidget.h/.cpp
│   │   │           ├── TimeRulerWidget.h/.cpp
│   │   │           ├── EntryListPanel.h/.cpp
│   │   │           ├── FileListPanel.h/.cpp
│   │   │           ├── SpectrogramColorPalette.h
│   │   │           └── commands/
│   │   │               ├── MoveBoundaryCommand.h/.cpp
│   │   │               ├── LinkedMoveBoundaryCommand.h/.cpp
│   │   │               ├── InsertBoundaryCommand.h/.cpp
│   │   │               ├── RemoveBoundaryCommand.h/.cpp
│   │   │               └── SetIntervalTextCommand.h/.cpp
│   │   │
│   │   ├── PitchLabeler/             # ★ PitchLabeler.exe
│   │   │   ├── CMakeLists.txt
│   │   │   ├── main.cpp
│   │   │   ├── PitchLabelerKeys.h
│   │   │   └── gui/
│   │   │       ├── MainWindow.h/.cpp
│   │   │       ├── DSFile.h/.cpp
│   │   │       └── ui/
│   │   │           ├── PianoRollView.h/.cpp
│   │   │           ├── FileListPanel.h/.cpp
│   │   │           └── PropertyPanel.h/.cpp
│   │   │
│   │   └── GameInfer/                # ★ GameInfer.exe
│   │       ├── CMakeLists.txt
│   │       ├── main.cpp
│   │       ├── gui/
│   │       │   ├── MainWindow.h/.cpp     # 保持 QMainWindow
│   │       │   └── MainWidget.h/.cpp
│   │       └── utils/
│   │           ├── GpuInfo.h
│   │           └── DmlGpuUtils.h/.cpp
│   │
│   └── tests/
│
└── docs/
```

> **待清理**: `src/apps/HubertFA/` 目录仍保留完整的旧版独立应用代码（main.cpp + GUI + util/），但已不参与构建（`src/apps/CMakeLists.txt` 不含 `add_subdirectory(HubertFA)`）。其中 `util/` 下的推理代码是 hubert-infer 提取（AD-01）的来源。待 AD-01 完成后应删除整个 `src/apps/HubertFA/` 目录。

### 3.1 与原结构映射

| 原路径 | 新路径 | 说明 |
|--------|--------|------|
| `src/apps/AudioSlicer/` | `src/apps/pipeline/slicer/` | 并入 Pipeline |
| `src/apps/LyricFA/` | `src/apps/pipeline/lyricfa/` | 并入 Pipeline |
| `src/apps/HubertFA/` | `src/apps/pipeline/hubertfa/` | 并入 Pipeline |
| `src/apps/MinLabel/` | `src/apps/MinLabel/` | 保留独立 EXE |
| `src/apps/SlurCutter/` | `src/apps/PitchLabeler/` | 重写为 F0 曲线编辑器 |
| `src/apps/GameInfer/` | `src/apps/GameInfer/` | 保留独立 EXE |
| — | `src/apps/PhonemeLabeler/` | 新增 TextGrid 编辑器 |
| `src/libs/qsmedia/` + plugins | `src/audio/` | 去掉插件体系 |
| `src/libs/AsyncTaskWindow/` | `src/widgets/TaskWindow` | 共享组件 |
| 两份 `PlayWidget` | `src/widgets/PlayWidget` | 合并 |
| 两份 `DmlGpuUtils` | `src/widgets/GpuSelector` | 合并 |
| `src/libs/FunAsr/` | `src/infer/funasr/` | 移入 infer 层 |
| `src/apps/HubertFA/util/Hfa.*` 等 | `src/infer/hubert-infer/` | 提取为库 |

---

## 4. CMake 构建

### 4.1 目标清单

| 目标名 | 类型 | 链接依赖 |
|--------|------|----------|
| `dstools-core` | STATIC | Qt6::Core, Qt6::Widgets |
| `dstools-audio` | STATIC | dstools-core, FFmpeg, SDL2 |
| `dstools-widgets` | **SHARED** | dstools-core, dstools-audio, Qt6::Widgets |
| `dstools-infer-common` | STATIC | onnxruntime |
| `audio-util` | SHARED | SndFile, soxr, mpg123 |
| `game-infer` | SHARED | audio-util, wolf-midi, nlohmann_json, dstools-infer-common |
| `some-infer` | SHARED | audio-util, wolf-midi, dstools-infer-common |
| `rmvpe-infer` | SHARED | audio-util, dstools-infer-common |
| `FunAsr` | STATIC | fftw3, dstools-infer-common |
| **`DatasetPipeline`** | **EXE** | dstools-widgets, audio-util, FunAsr, cpp-pinyin, SndFile, Qt |
| **`MinLabel`** | **EXE** | dstools-widgets, cpp-pinyin, cpp-kana, Qt |
| **`PhonemeLabeler`** | **EXE** | dstools-widgets, dstools-audio, textgrid, FFTW3, Qt |
| **`PitchLabeler`** | **EXE** | dstools-widgets, Qt |
| **`GameInfer`** | **EXE** | dstools-widgets, game-infer, Qt |

> hubert-infer 尚未提取为独立库（AD-01），当前 Pipeline 的 HubertFA Tab 直接引用 `src/apps/HubertFA/util/` 中的源文件。

### 4.2 构建选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| BUILD_TESTS | ON | 构建测试可执行文件 |
| AUDIO_UTIL_BUILD_TESTS | ON | 构建 TestAudioUtil |
| GAME_INFER_BUILD_TESTS | ON | 构建 TestGame |
| SOME_INFER_BUILD_TESTS | ON | 构建 TestSome |
| RMVPE_INFER_BUILD_TESTS | ON | 构建 TestRmvpe |
| ONNXRUNTIME_ENABLE_DML | ON（仅 Windows） | 启用 DirectML 加速 |
| ONNXRUNTIME_ENABLE_CUDA | OFF | 启用 CUDA 加速 |

### 4.3 构建产物

**应用程序：**

- DatasetPipeline.exe
- MinLabel.exe
- PhonemeLabeler.exe
- PitchLabeler.exe
- GameInfer.exe

**共享库：**

- dstools-widgets.dll
- audio-util.dll
- game-infer.dll
- some-infer.dll
- rmvpe-infer.dll

**测试程序：**

- TestGame.exe
- TestRmvpe.exe
- TestSome.exe
- TestAudioUtil.exe

### 4.4 五个 EXE 的 CMakeLists

```cmake
# --- DatasetPipeline ---
# src/apps/pipeline/CMakeLists.txt
add_executable(DatasetPipeline WIN32
    main.cpp
    PipelineWindow.cpp
    slicer/SlicerPage.cpp
    slicer/Slicer.cpp
    slicer/WorkThread.cpp
    lyricfa/LyricFAPage.cpp
    lyricfa/AsrTask.cpp
    lyricfa/LyricMatchTask.cpp
    lyricfa/Asr.cpp
    lyricfa/MatchLyric.cpp
    lyricfa/LyricMatcher.cpp
    lyricfa/SequenceAligner.cpp
    lyricfa/ChineseProcessor.cpp
    lyricfa/SmartHighlighter.cpp
    hubertfa/HubertFAPage.cpp
    hubertfa/HfaTask.cpp
)
target_link_libraries(DatasetPipeline PRIVATE
    dstools-widgets          # 共享样式+TaskWindow+GpuSelector
    audio-util               # 音频处理
    FunAsr                   # ASR 推理
    cpp-pinyin::cpp-pinyin   # G2P (LyricFA)
    SndFile::sndfile         # WAV I/O (Slicer)
    Qt6::Core Qt6::Widgets
)

# --- MinLabel ---
# src/apps/MinLabel/CMakeLists.txt
add_executable(MinLabel WIN32
    main.cpp MainWindow.cpp TextWidget.cpp ExportDialog.cpp Common.cpp
)
target_link_libraries(MinLabel PRIVATE
    dstools-widgets          # 共享样式+PlayWidget
    cpp-pinyin::cpp-pinyin
    cpp-kana::cpp-kana
    Qt6::Core Qt6::Widgets
)

# --- PhonemeLabeler ---
# src/apps/PhonemeLabeler/CMakeLists.txt
file(GLOB_RECURSE PHONEMELABELER_SOURCES *.h *.cpp)
add_executable(PhonemeLabeler WIN32 ${PHONEMELABELER_SOURCES})
target_link_libraries(PhonemeLabeler PRIVATE
    dstools-widgets          # 共享样式+PlayWidget
    dstools-audio            # 音频解码与播放
    textgrid                 # TextGrid 读写
    FFTW3::fftw3             # 频谱分析
    Qt6::Core Qt6::Widgets
)

# --- PitchLabeler ---
# src/apps/PitchLabeler/CMakeLists.txt
file(GLOB_RECURSE PITCHLABELER_SOURCES *.h *.cpp)
add_executable(PitchLabeler WIN32 ${PITCHLABELER_SOURCES})
target_link_libraries(PitchLabeler PRIVATE
    dstools-widgets          # 共享样式+PlayWidget
    Qt6::Core Qt6::Widgets
)

# --- GameInfer ---
# src/apps/GameInfer/CMakeLists.txt
add_executable(GameInfer WIN32
    main.cpp MainWindow.cpp MainWidget.cpp
)
target_link_libraries(GameInfer PRIVATE
    dstools-widgets          # 共享样式+GpuSelector
    game-infer::game-infer
    Qt6::Core Qt6::Widgets
)
```

### 4.5 dstools-widgets.dll

```cmake
# src/widgets/CMakeLists.txt
add_library(dstools-widgets SHARED
    src/PlayWidget.cpp
    src/TaskWindow.cpp
    src/GpuSelector.cpp
    ${GLOBAL_RESOURCES}      # resources/resources.qrc (QSS + icons)
)
target_include_directories(dstools-widgets PUBLIC include)
target_link_libraries(dstools-widgets PUBLIC
    dstools-core
    dstools-audio
    Qt6::Core Qt6::Widgets Qt6::Svg
)
# 导出宏
target_compile_definitions(dstools-widgets PRIVATE DSTOOLS_WIDGETS_BUILD)
# 公共头文件中:
# #ifdef DSTOOLS_WIDGETS_BUILD
# #define DSTOOLS_WIDGETS_API Q_DECL_EXPORT
# #else
# #define DSTOOLS_WIDGETS_API Q_DECL_IMPORT
# #endif
```

### 4.6 关键设计决策

**为什么 dstools-widgets 是 SHARED (DLL)?**

5 个 EXE 都链接它。如果是 STATIC，主题/样式代码会被编译进每个 EXE，增加体积且无法运行时统一更新主题。SHARED 确保：
- 一份 QSS 加载代码
- 一份 `Theme::palette()` 色板
- 一份 PlayWidget / TaskWindow / GpuSelector 实现
- 用户替换 `dstools-widgets.dll` 可同时更新所有 5 个 EXE 的外观

**为什么 4 个工具保持独立 EXE 而不并入 Pipeline?**

- MinLabel 是手工交互标注工具，有完整的文件树浏览器和音频播放器，工作模式与批处理完全不同
- PhonemeLabeler 是 TextGrid 音素边界编辑器，高度专业化的多层可视化 UI，不适合嵌入选项卡
- PitchLabeler 是 F0 曲线钢琴卷帘编辑器，高度专业化的 UI，不适合嵌入选项卡
- GameInfer 是单文件推理工具，界面简单但独立运行更方便

### 4.7 部署脚本

```cmake
# src/CMakeLists.txt — install/deploy

# 五个 EXE
set(DEPLOY_TARGETS DatasetPipeline MinLabel PhonemeLabeler PitchLabeler GameInfer)

# windeployqt 对每个 EXE 运行
foreach(target ${DEPLOY_TARGETS})
    install(TARGETS ${target} RUNTIME DESTINATION .)
endforeach()

# 共享 DLL
install(TARGETS dstools-widgets RUNTIME DESTINATION .)

# 推理 DLL
install(TARGETS audio-util game-infer some-infer rmvpe-infer
        RUNTIME DESTINATION .)

# 外部 DLL (onnxruntime, SDL2, sndfile, etc.)
install(CODE "
    file(GLOB _ext_dlls \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/*.dll\")
    file(INSTALL \${_ext_dlls} DESTINATION \${CMAKE_INSTALL_PREFIX})
")

# cpp-pinyin 字典
install(DIRECTORY ${CPP_PINYIN_DICT_DIR} DESTINATION dict)

# windeployqt
add_custom_target(deploy ALL)
foreach(target ${DEPLOY_TARGETS})
    add_custom_command(TARGET deploy POST_BUILD
        COMMAND ${WINDEPLOYQT} --no-translations --no-opengl-sw
            --libdir ${CMAKE_INSTALL_PREFIX}
            --plugindir ${CMAKE_INSTALL_PREFIX}/plugins
            $<TARGET_FILE:${target}>
    )
endforeach()
```

---

## 5. 应用设计

### 5.1 DatasetPipeline — 数据集处理管线

入口：`src/apps/pipeline/main.cpp`

DatasetPipeline 将三个处理阶段整合为一个多标签页窗口，每个标签页独立运作。

#### 5.1.1 窗口结构

```cpp
// src/apps/pipeline/PipelineWindow.h
#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include <dstools/Config.h>

class SlicerPage;
class LyricFAPage;
class HubertFAPage;

/// 数据集处理管线主窗口。
/// 整合 AudioSlicer + LyricFA + HubertFA 三个步骤为 Tab 分页。
/// 每个 Tab 页独立操作，不提供一键串联运行。
class PipelineWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit PipelineWindow(QWidget *parent = nullptr);
    ~PipelineWindow() override;

private:
    void setupUI();
    void setupMenuBar();
    void setupConnections();

    // Tab 页面
    QTabWidget *m_tabWidget;
    SlicerPage *m_slicerPage;
    LyricFAPage *m_lyricFAPage;
    HubertFAPage *m_hubertFAPage;

    dstools::Config m_config;
};
```

> **注意**: 不再需要 `PipelineRunner`。没有全局进度条、全局 Run All 按钮。
> 每个 Tab Page 内部自带各自的 Run/Stop 按钮、进度条和日志。

#### 5.1.2 界面布局

```
┌─────────────────────────────────────────────────────────┐
│ Dataset Pipeline — DiffSinger Dataset Tools              │
├─────────────────────────────────────────────────────────┤
│  [AudioSlicer] │ [LyricFA] │ [HubertFA]                 │
│  ─────────────┴───────────┴────────────────────────────  │
│ ┌─────────────────────────────────────────────────────┐  │
│ │                                                     │  │
│ │   当前 Tab 页的完整 UI（配置+文件列表+操作按钮）     │  │
│ │                                                     │  │
│ │   每个 Tab 页各自有独立的:                           │  │
│ │   - 文件列表（拖放添加）                             │  │
│ │   - 参数配置面板                                     │  │
│ │   - [▶ Run] / [Stop] 按钮                           │  │
│ │   - 进度条                                           │  │
│ │   - 日志输出                                         │  │
│ │                                                     │  │
│ └─────────────────────────────────────────────────────┘  │
├──────────────────────────────────────────────────────────┤
│ Ready                                                     │
└──────────────────────────────────────────────────────────┘
```

#### 5.1.3 运行模式：分页独立运行

**每个 Tab 页独立操作，不提供一键串联运行。**

- 用户选择某个 Tab（AudioSlicer / LyricFA / HubertFA）
- 在该 Tab 中配置参数、添加文件
- 点击该 Tab 内的「Run」按钮
- 行为与原独立应用完全一致
- 用户需要手动在上一步完成后，切换到下一个 Tab，设置输入目录为上一步的输出目录

> **设计理由**: 三个工具虽然构成流水线，但实际使用中用户经常需要在每一步之后
> 检查结果、调整参数、手动修正错误文件，然后再进行下一步。强制串联反而降低了
> 灵活性。分页隔开让每个步骤保持独立，同时共享一个窗口减少了启动多个 EXE 的麻烦。

#### 5.1.4 各 Tab 页保留的完整功能

| Tab | 原应用 | 功能（完全保留） |
|-----|--------|-----------------|
| 1 | AudioSlicer | 拖放文件、参数配置、WAV/CSV 输出、任务栏进度、输出格式选择 |
| 2 | LyricFA | 模型加载、ASR 识别、歌词匹配、差异高亮 |
| 3 | HubertFA | 模型加载、GPU 选择、多语言 G2P、TextGrid 输出 |

#### 5.1.5 AudioSlicer Tab

基于 RMS（均方根能量）的自动音频切分工具。

- 输入：长音频文件
- 处理：按 RMS 阈值检测静音段并切分
- 输出：切分后的音频片段 + Audacity CSV 标记文件

#### 5.1.6 LyricFA Tab

歌词强制对齐工具，结合语音识别与序列比对。

- 使用 FunASR Paraformer 模型进行中文语音识别（ASR）
- 通过 Needleman-Wunsch 算法将 ASR 结果与原始歌词进行最优匹配
- 输出时间对齐的歌词标注

#### 5.1.7 HubertFA Tab

基于 HuBERT 模型的音素级强制对齐工具。

- 输入：音频 + 音素序列
- 处理：HuBERT 特征提取 + 音素对齐
- 输出：Praat TextGrid 格式的音素时间标注

#### 5.1.8 SlicerPage 特殊处理

AudioSlicer 原本是独立的 `QMainWindow`，**不继承 AsyncTaskWindow**，有自己独特的 UI：

- 自有的参数面板（阈值/最小长度/最小间隔/跳跃/最大静音/输出格式/切片模式/文件名后缀位数）
- 自有的文件列表（不同于 TaskWindow 的拖放逻辑，有不同的 signal 签名：`slot_oneFinished(filename, listIndex)`）
- 自有的主题/样式菜单（`initStylesMenu`）
- Windows 任务栏进度（`ITaskbarList3` + `nativeEvent`）
- 独特的 `setProcessing()` 状态管理

因此 SlicerPage **不继承 TaskWindow**，而是直接继承 `QWidget`，将原 `MainWindow` 的 UI 代码平移为 Page。
需要特别注意：
1. `ITaskbarList3` 的 HWND 需要通过 `window()->winId()` 获取（而非 `winId()`）
2. 原 `showEvent` / `nativeEvent` 需要适配为 QWidget 而非 QMainWindow
3. 原 `slot_oneFinished(filename, listIndex)` 的签名与 TaskWindow 不同，不可混淆
4. 原 `initStylesMenu()` 删除（由全局 Theme 替代）
5. `nativeEvent` 需从 PipelineWindow 转发或改用 `QAbstractNativeEventFilter`

| 差异 | AsyncTaskWindow (LyricFA/HubertFA) | AudioSlicer |
|------|-----------------------------------|-|
| 基类 | `AsyncTaskWindow : QMainWindow` | `QMainWindow`（独立） |
| 文件列表 | `QListWidget` + `slot_addFile` | 自有 `QListWidget` + 自有 slot |
| 完成信号 | `slot_oneFinished(filename, msg)` | `slot_oneFinished(filename, listIndex)` — **签名不同** |
| 参数面板 | 子类在 `m_rightPanel` 添加 | 完整的手工 UI（阈值/长度/间隔/跳跃/静音/格式/模式/后缀位数） |
| 主题菜单 | 无 | `initStylesMenu()` 自有样式系统 |
| 任务栏进度 | 无 | `ITaskbarList3` + `nativeEvent` |
| 状态管理 | `m_isRunning` | `m_processing` + `setProcessing()` |

#### 5.1.9 LyricFAPage / HubertFAPage

这两个继承 `TaskWindow`（原 `AsyncTaskWindow` 的重构版），与原代码保持一致：

**LyricFAPage**（原 LyricFAWindow）：
- 继承 `TaskWindow`
- `initCustomUI()` 中添加：模型路径、Lab/JSON/歌词输出目录、拼音选项、"Match Lyrics" 按钮
- `runTasks()` 中提交 `AsrTask` / `LyricMatchTask`
- 保留差异高亮

**HubertFAPage**（原 HubertFAWindow）：
- 继承 `TaskWindow`
- `initCustomUI()` 中添加：模型路径、TextGrid 输出目录、`GpuSelector`
- 模型加载后动态生成语言 radio 和非语音音素 checkbox
- `runTasks()` 中提交 `HfaTask`

#### 5.1.10 main.cpp

```cpp
// src/apps/pipeline/main.cpp
#include <QApplication>
#include <dstools/AppInit.h>
#include <dstools/Theme.h>
#include "PipelineWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Dataset Pipeline");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Team OpenVPI");

    dstools::AppInit::init(app, /*initPinyin=*/true);
    dstools::Theme::apply(app, dstools::Theme::Dark);

    PipelineWindow window;
    window.show();
    return app.exec();
}
```

### 5.2 PhonemeLabeler — TextGrid 音素边界编辑器

入口：`src/apps/PhonemeLabeler/main.cpp`
命名空间：`dstools::phonemelabeler`

PhonemeLabeler 是一个专用的 Praat TextGrid 音素边界编辑工具，提供波形、频谱、能量三视图可视化，支持跨层边界绑定和撤销/重做。

#### 5.2.1 核心功能

- **TextGrid 文档模型**：`TextGridDocument` 封装 TextGrid 文件的加载、保存和数据访问，内部使用 textgrid 头文件库解析
- **三视图可视化**：WaveformWidget（波形）、SpectrogramWidget（频谱图，使用 FFTW3 计算 STFT）、PowerWidget（能量曲线）
- **跨层边界绑定**：`BoundaryBindingManager` 管理多个 IntervalTier 之间的边界联动，拖动一个层的边界时，绑定层的对应边界同步移动
- **统一视口控制**：`ViewportController` 管理所有视图的时间轴缩放和滚动同步，确保波形、频谱、能量视图及各 Tier 视图始终对齐
- **多层编辑**：`IntervalTierView` 显示单个 Tier 的区间和边界，`TierEditWidget` 提供文本编辑入口，`BoundaryOverlayWidget` 绘制跨视图的边界对齐线
- **撤销/重做**：基于 QUndoStack，支持 MoveBoundaryCommand、LinkedMoveBoundaryCommand、InsertBoundaryCommand、RemoveBoundaryCommand、SetIntervalTextCommand
- **文件列表**：FileListPanel 管理当前工作目录下的 TextGrid 文件列表
- **条目导航**：EntryListPanel 显示当前 Tier 的所有区间，点击可快速跳转

#### 5.2.2 窗口结构

```
┌──────────────────────────────────────────────────────────┐
│ PhonemeLabeler — TextGrid Editor                          │
├──────────────────────────────────────────────────────────┤
│ ┌────────────┐ ┌──────────────────────────────────────┐  │
│ │ FileList   │ │  TimeRuler                            │  │
│ │            │ │  WaveformWidget                       │  │
│ │            │ │  SpectrogramWidget                    │  │
│ │            │ │  PowerWidget                          │  │
│ │            │ │  IntervalTierView (phones)            │  │
│ │            │ │  IntervalTierView (words)             │  │
│ │            │ │  BoundaryOverlayWidget                │  │
│ │            │ ├──────────────────────────────────────┤  │
│ │            │ │  EntryListPanel                       │  │
│ └────────────┘ └──────────────────────────────────────┘  │
├──────────────────────────────────────────────────────────┤
│ Ready                                                     │
└──────────────────────────────────────────────────────────┘
```

#### 5.2.3 频谱计算

SpectrogramWidget 通过 FFTW3 执行 STFT（短时傅里叶变换），生成时频图。SpectrogramColorPalette 定义热力图色板，将幅度映射为像素颜色。

#### 5.2.4 窗口装饰

使用 `FramelessHelper` 实现无边框窗口，自定义标题栏与拖动区域。

### 5.3 PitchLabeler — DS 文件 F0 曲线编辑器

入口：`src/apps/PitchLabeler/main.cpp`
命名空间：`dstools::pitchlabeler`

PitchLabeler 是 DiffSinger .ds 文件的 F0 曲线编辑工具，以钢琴卷帘为背景显示 F0 曲线，支持多工具编辑模式。

#### 5.3.1 核心功能

- **DS 文件 I/O**：`DSFile` 负责 DiffSinger .ds JSON 文件的加载与保存，解析句子、音符、F0 参数等结构
- **钢琴卷帘可视化**：`PianoRollView` 在钢琴卷帘背景上叠加绘制 F0 曲线和音符，支持缩放/滚动
- **三种工具模式**：
  - Select：选择音符和 F0 区域
  - Modulation：调节 F0 调制参数
  - Drift：调节 F0 偏移参数
- **A/B 对比**：支持加载两个版本的 F0 数据进行叠加对比显示
- **撤销/重做**：基于 QUndoStack 管理编辑历史
- **属性面板**：`PropertyPanel` 显示和编辑当前选中音符/区域的属性
- **文件列表**：`FileListPanel` 管理工作目录下的 .ds 文件列表，带进度追踪

#### 5.3.2 窗口结构

```
┌──────────────────────────────────────────────────────────┐
│ PitchLabeler — F0 Editor                                  │
├──────────────────────────────────────────────────────────┤
│ ┌────────────┐ ┌──────────────────────────────────────┐  │
│ │ FileList   │ │                                      │  │
│ │ (progress) │ │  PianoRollView                       │  │
│ │            │ │  (钢琴卷帘 + F0 曲线叠加)            │  │
│ │            │ │                                      │  │
│ │            │ │  工具栏: [Select] [Modulation] [Drift]│  │
│ │            │ ├──────────────────────────────────────┤  │
│ │            │ │  PropertyPanel                       │  │
│ └────────────┘ └──────────────────────────────────────┘  │
├──────────────────────────────────────────────────────────┤
│ Ready                                                     │
└──────────────────────────────────────────────────────────┘
```

#### 5.3.3 窗口装饰

使用 `FramelessHelper` 实现无边框窗口，自定义标题栏与拖动区域。

### 5.4 GameInfer — GAME 音频转 MIDI

入口：`src/apps/GameInfer/main.cpp`

使用 GAME（四模型 ONNX 管线）将音频转换为 MIDI。

推理流程：

```
音频 → Encoder → Segmenter → BD2Dur → Estimator → MIDI
```

四个 ONNX 模型各司其职：编码器提取特征，分段器划分音符边界，BD2Dur 预测时值，估计器输出最终音高与时值。

```cpp
// src/apps/GameInfer/main.cpp
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("GameInfer");
    app.setOrganizationName("Team OpenVPI");

    dstools::AppInit::init(app);
    dstools::Theme::apply(app, dstools::Theme::Dark);

    MainWindow window;
    window.show();
    return app.exec();
}
```

### 5.5 统一样式方案

#### 5.5.1 共享方式

5 个 EXE（DatasetPipeline, MinLabel, PhonemeLabeler, PitchLabeler, GameInfer）通过链接 `dstools-widgets.dll` 获得统一样式：

```cpp
// 每个 EXE 的 main.cpp — 仅需数行即可获得统一外观
#include <dstools/AppInit.h>
#include <dstools/Theme.h>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    dstools::AppInit::init(app);                // 统一字体
    dstools::Theme::apply(app, dstools::Theme::Dark);  // 统一深色主题
    // ... 创建 MainWindow ...
}
```

#### 5.5.2 统一内容

| 统一项 | 实现位置 | 效果 |
|--------|----------|------|
| 字体 | `AppInit::init()` | 5 个 EXE 都使用 Microsoft YaHei / Segoe UI |
| 深色/亮色主题 | `Theme::apply()` 加载 QSS | 所有 QWidget 控件统一配色 |
| 色板 | `Theme::palette()` | 自绘组件（PianoRollView 等）也使用统一色板 |
| PlayWidget | `dstools-widgets.dll` | MinLabel/PhonemeLabeler/PitchLabeler 用同一份播放组件 |
| GpuSelector | `dstools-widgets.dll` | GameInfer/HubertFA Tab 用同一份 GPU 选择器 |
| 图标 | `resources.qrc` 编译进 `dstools-widgets.dll` | 5 个 EXE 共享同一套 SVG 图标 |

#### 5.5.3 独立 EXE 保持 QMainWindow

MinLabel、PhonemeLabeler、PitchLabeler、GameInfer 保持 `QMainWindow` 作为主窗口（不改为 QWidget Page），因为：
- 它们有各自的菜单栏（File/Edit/Help）
- 窗口大小、位置独立管理
- 不需要与其他工具共享窗口

变化仅限于：
1. `main.cpp` 调用 `AppInit::init()` + `Theme::apply()`（替代各自的字体设置代码）
2. 使用共享的 `PlayWidget`（替代各自的重复实现）
3. 使用共享的 `GpuSelector`（GameInfer）
4. 所有 UI 字符串用 `tr()` 包裹

#### 5.5.4 对比表

| 方面 | 原代码 | 重构后 |
|------|--------|--------|
| 字体设置 | 各自 10-15 行 Win32 代码 | `AppInit::init()` 一行 |
| root 检查 | MinLabel 等各 5 行 | `AppInit::init()` 内部 |
| 深色模式 | 无 | `Theme::apply(Dark)` 一行 |
| 样式表 | 各应用各自 app.qss | 全局 dark.qss 由 Theme 加载 |
| PlayWidget | 各自一份 (~350行×2) | 链接 `dstools-widgets.dll` |
| 窗口类型 | QMainWindow | **不变**，仍是 QMainWindow |
| 菜单栏 | 各自创建 | **不变** |
| 独立进程 | 是 | **不变** |

---

## 6. 部署结构

### 6.1 安装目录

```
DatasetTools/                        # 安装目录
├── DatasetPipeline.exe              # 数据集处理管线（切片+识别+对齐）
├── MinLabel.exe                     # 音频标注
├── PhonemeLabeler.exe               # TextGrid 音素边界编辑
├── PitchLabeler.exe                 # F0 曲线编辑
├── GameInfer.exe                    # 音频转 MIDI
│
├── dstools-widgets.dll              # 共享样式/组件库（统一外观的关键）
├── audio-util.dll                   # 推理共享库
├── game-infer.dll
├── some-infer.dll
├── rmvpe-infer.dll
├── onnxruntime.dll
│
├── Qt6Core.dll                      # Qt (windeployqt)
├── Qt6Gui.dll
├── Qt6Widgets.dll
├── Qt6Svg.dll
├── SDL2.dll
├── sndfile.dll
├── ...
├── plugins/                         # Qt 插件
├── dict/                            # cpp-pinyin 字典
└── model/                           # 模型文件
```

对比原先：
- 原先 6 个 EXE 混在 DLL 中 → 现在 5 个 EXE，名称清晰
- 新增 `DatasetPipeline.exe` 整合 3 个批处理工具
- `dstools-widgets.dll` 保证 5 个 EXE 视觉一致

### 6.2 配置文件

每个 EXE 有自己的配置文件（保持独立、清晰）：

```
config/
├── DatasetPipeline.json   # Pipeline 全局 + 三步骤参数
├── MinLabel.json          # 与原格式兼容
├── PhonemeLabeler.json    # 音素编辑器配置
├── PitchLabeler.json      # F0 编辑器配置
└── GameInfer.json         # 与原格式兼容
```

DatasetPipeline.json 格式由 `AppSettings` 管理，基于 nlohmann::json 后端。各独立 EXE 的配置同理。`AppSettings` 类自动处理路径：

```cpp
// AppSettings 根据 appName 选择配置文件
dstools::AppSettings settings("MinLabel");
// → 读写 <appDir>/config/MinLabel.json
```

> **注意**: 不再有 `workDir` 概念（因为没有一键串联）。每个 Tab 各自管理输入/输出目录。

### 6.3 支持平台

| 平台 | 说明 |
|------|------|
| Windows 10/11 | 主要平台，支持 DirectML 加速 |
| macOS 11+ | 支持 |
| Linux (Ubuntu) | 支持 |

---

## 7. 技术栈与依赖

### 7.1 语言与框架

| 项目 | 版本 |
|------|------|
| C++ | C++17 |
| Qt | 6.8+（CI 使用 6.9.3） |
| CMake | 3.17+ |
| vcpkg | 依赖管理 |

### 7.2 推理与音频

| 依赖 | 用途 |
|------|------|
| ONNX Runtime | 模型推理（CPU / CUDA / DirectML） |
| FFmpeg | 音频解码 |
| SDL2 | 音频播放 |
| libsndfile | 音频文件读写 |
| soxr | 采样率转换 |
| mpg123 | MP3 解码 |
| FLAC | FLAC 解码 |
| fftw3 | 快速傅里叶变换 |

### 7.3 工具库

| 依赖 | 用途 |
|------|------|
| nlohmann/json | JSON 读写，配置文件后端 |
| wolf-midi | MIDI 文件处理 |
| cpp-pinyin | 普通话 G2P（vcpkg 依赖） |
| cpp-kana | 日语 G2P（vcpkg 依赖） |
| textgrid.hpp | Praat TextGrid 读写（src/libs/textgrid，头文件库） |

### 7.4 模型清单

| 模型 | 所属模块 | 说明 |
|------|----------|------|
| AsrModel | LyricFA | FunASR Paraformer，仅支持中文 |
| HuBERT | HubertFA | 音素强制对齐 |
| SomeModel | PitchLabeler | SOME MIDI 估计 |
| FblModel | DatasetPipeline | FoxBreatheLabeler 呼吸检测 |
| RMVPE | audio-util | F0 基频估计 |
| GAME（4 文件） | GameInfer | encoder, segmenter, bd2dur, estimator |

### 7.5 配置系统

配置基于 `AppSettings` 类，以 nlohmann::json 为后端存储。

- 类型安全的键定义：`SettingsKey<T>` 模板
- 每个键支持独立的观察者回调和 `keyChanged` 信号
- 写入时通过 `QSaveFile` 保证原子性
- 配置文件路径：`config/DatasetPipeline.json`、`config/MinLabel.json` 等

### 7.6 主题系统

通过 `Theme::apply()` 切换深色/浅色 QSS 主题，`Theme::palette()` 提供语义化调色板。五个应用共享 dstools-widgets.dll 中的主题实现。

主题文件位于 `resources/themes/`（dark.qss、light.qss）。详见 `ui-theming.md`。

### 7.7 内嵌第三方依赖

以下依赖以源码或预编译二进制形式内嵌于仓库中，不通过 vcpkg 管理：

| 依赖 | 位置 | 形式 | 说明 |
|------|------|------|------|
| ONNX Runtime | `src/infer/onnxruntime/` | 预编译头文件 + DLL | 通过 `cmake/setup-onnxruntime.cmake` 下载 |
| FunAsr | `src/infer/FunAsr/` | 完整源码 | 阿里 FunASR Paraformer 的定制副本，含独立 LICENSE |

其余依赖（Qt、SDL2、SndFile、soxr、fftw3、yaml-cpp、nlohmann/json、cpp-pinyin、cpp-kana 等）通过 vcpkg 管理，见 `scripts/vcpkg-manifest/`。

---

## 8. 用户使用流程

### 场景 A: 制作数据集（分步流水线）

1. 打开 `DatasetPipeline.exe`
2. 在 AudioSlicer Tab 中添加源音频，配置参数，点击 Run
3. 等待切片完成，检查输出结果
4. 切换到 LyricFA Tab，加载 ASR 模型
5. 将 AudioSlicer 的输出目录中的文件添加为输入，点击 Run
6. 等待识别完成，检查结果
7. 切换到 HubertFA Tab，加载 HuBERT 模型
8. 将 LyricFA 的输出文件添加为输入，点击 Run
9. 等待完成，在输出目录获得 TextGrid 训练数据

### 场景 B: 仅使用某一步

1. 打开 `DatasetPipeline.exe`
2. 直接切换到需要的 Tab（如 HubertFA）
3. 独立配置、运行
4. 无需关心其他 Tab

### 场景 C: 手工标注

1. 打开 `MinLabel.exe`
2. 与原使用方式完全一致

### 场景 D: 编辑音素边界

1. 打开 `PhonemeLabeler.exe`
2. 加载 TextGrid 文件目录
3. 在波形/频谱/能量视图中查看音频，编辑音素边界
4. 使用跨层绑定功能同步调整多层边界

### 场景 E: 编辑 F0 曲线

1. 打开 `PitchLabeler.exe`
2. 加载 .ds 文件目录
3. 在钢琴卷帘视图中查看和编辑 F0 曲线
4. 使用 Select/Modulation/Drift 工具进行精细调节

### 场景 F: 音频转 MIDI

1. 打开 `GameInfer.exe`
2. 与原使用方式完全一致

---

## 9. 功能守恒验证清单

### DatasetPipeline（含 AudioSlicer + LyricFA + HubertFA 全部功能）

**Tab 1 — AudioSlicer:**
- [ ] 拖放/浏览添加 WAV 文件
- [ ] 参数配置（阈值/最小长度/最小间隔/跳跃/最大静音）
- [ ] 批量切片 + WAV 输出
- [ ] CSV 标记导出/导入（BUG-006 修复后）
- [ ] 输出格式选择（16/24/32-bit）
- [ ] 切片模式选择
- [ ] 文件名后缀位数配置
- [ ] 覆盖标记选项
- [ ] Windows 任务栏进度

**Tab 2 — LyricFA:**
- [ ] 加载 Paraformer ASR 模型
- [ ] 批量 ASR → .lab
- [ ] 歌词匹配（Needleman-Wunsch + 滑动窗口）→ .json
- [ ] 差异高亮
- [ ] 中文文本清洗

**Tab 3 — HubertFA:**
- [ ] 加载 HuBERT ONNX 模型（CPU/DML/CUDA）
- [ ] 批量音素对齐 → TextGrid
- [ ] Viterbi 解码
- [ ] 多语言 G2P
- [ ] AP/SP 检测
- [ ] GPU 选择

**Pipeline 分页独立运行:**
- [ ] 三个 Tab 页各自独立配置和运行
- [ ] 每个 Tab 有独立的 Run/Stop/进度条/日志
- [ ] Tab 切换不影响正在运行的任务（或正确提示用户）
- [ ] SlicerPage 任务栏进度正常

### MinLabel
- [ ] 目录浏览 + 文件树
- [ ] 音频播放（共享 PlayWidget）
- [ ] JSON 标注编辑 + 自动保存
- [ ] G2P（普通话/粤语/日语）
- [ ] Lab→JSON 转换
- [ ] 导出
- [ ] 快捷键
- [ ] 深色主题

### PhonemeLabeler
- [ ] TextGrid 文件加载与保存
- [ ] 波形显示（WaveformWidget）
- [ ] 频谱图显示（SpectrogramWidget，FFTW3）
- [ ] 能量曲线显示（PowerWidget）
- [ ] 多 Tier 区间显示与编辑（IntervalTierView）
- [ ] 边界拖动（MoveBoundaryCommand）
- [ ] 跨层边界绑定（LinkedMoveBoundaryCommand）
- [ ] 边界插入与删除
- [ ] 区间文本编辑（SetIntervalTextCommand）
- [ ] 撤销/重做（QUndoStack）
- [ ] 视口缩放/滚动同步（ViewportController）
- [ ] 文件列表浏览（FileListPanel）
- [ ] 条目导航（EntryListPanel）
- [ ] 深色主题

### PitchLabeler
- [ ] .ds JSON 文件加载与保存
- [ ] 钢琴卷帘背景 + F0 曲线叠加显示
- [ ] Select 工具：选择音符和 F0 区域
- [ ] Modulation 工具：调节 F0 调制参数
- [ ] Drift 工具：调节 F0 偏移参数
- [ ] A/B 对比显示
- [ ] 属性面板编辑（PropertyPanel）
- [ ] 撤销/重做（QUndoStack）
- [ ] 文件列表 + 进度追踪（FileListPanel）
- [ ] 缩放/滚动
- [ ] 深色主题

### GameInfer
- [ ] GAME 模型加载
- [ ] 音频 → MIDI 推理
- [ ] CPU/CUDA/DML 切换（共享 GpuSelector）
- [ ] MIDI 输出
- [ ] 深色主题

---

## 10. 术语表

| 术语 | 说明 |
|------|------|
| DiffSinger | 基于扩散模型的歌声合成系统 |
| G2P | Grapheme-to-Phoneme，字素到音素的转换 |
| F0 | 基频（Fundamental Frequency），表征音高 |
| RMS | 均方根（Root Mean Square），衡量音频能量 |
| ASR | 自动语音识别（Automatic Speech Recognition） |
| Needleman-Wunsch | 全局序列比对算法，用于歌词匹配 |
| TextGrid | Praat 软件的时间标注格式 |
| HuBERT | Hidden-Unit BERT，Meta 提出的自监督语音表征模型 |
| RMVPE | Robust Model for Vocal Pitch Estimation |
| SOME | Singing-Oriented MIDI Extractor |
| GAME | 四模型音频转 MIDI 系统 |
| FunASR | 阿里达摩院开源语音识别框架 |
| Paraformer | FunASR 中的非自回归端到端语音识别模型 |
| DirectML | 微软 DirectX 机器学习加速 API |
| QSS | Qt Style Sheets，Qt 样式表 |
| Lab 文件 | HTK Label 格式，记录时间对齐的音素标注 |
| STFT | Short-Time Fourier Transform，短时傅里叶变换 |
| DS 文件 | DiffSinger 项目文件格式（.ds JSON） |
| vcpkg | 微软开源的 C/C++ 包管理器 |
