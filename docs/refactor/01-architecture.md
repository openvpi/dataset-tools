# 01 — 总体架构与模块设计

**Version**: 3.0  
**Date**: 2026-04-26  
**变更**: v3.0 修正 Pipeline 设计：AudioSlicer / LyricFA / HubertFA 三个步骤使用 Tab 分页隔开，**不提供一键串联运行**，每个 Tab 页独立操作。保留「数据集处理管线」(1 EXE) + 3 个独立工具 (3 EXE)，共 4 个可执行文件。

---

## 1. 重构动因

### 1.1 当前痛点

| 问题 | 影响 |
|------|------|
| 6 个 EXE + 30+ DLL 平铺在一个目录 | 用户困惑 |
| AudioSlicer / LyricFA / HubertFA 是数据集制作的连续流程，却要分别启动 | 工作流割裂 |
| PlayWidget 75% 重复、DmlGpuUtils 100% 重复、EP 枚举 4 份 | 维护成本 |
| qsmedia 插件/工厂体系从未使用 | 过度设计 |
| F0Widget 1097 行单文件 | 低内聚 |
| 43 个已知 Bug（含 3 个 Critical） | 运行时风险 |
| 各应用字体/样式/主题不统一，无深色模式 | 视觉不一致 |

### 1.2 重构原则

1. **功能守恒** — 所有工具的输入/输出/行为不变
2. **按使用场景分组** — 数据集管线一体化，创作工具独立
3. **共享样式，独立进程** — 4 个 EXE 通过共享库获得统一字体/主题/深色模式
4. **高内聚低耦合** — 消除重复，明确模块边界
5. **渐进式** — 按阶段实施，每阶段可独立验证

### 1.3 应用分组逻辑

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

而 MinLabel（手工标注）、SlurCutter（MIDI编辑）、GameInfer（音频转MIDI）
是独立的创作/编辑工具，各有不同的使用场景和窗口布局，适合作为独立 EXE。
```

---

## 2. 重构后架构

### 2.1 应用拓扑

```
4 个可执行文件:

  ┌──────────────────────────────────────────────┐
  │ DatasetPipeline.exe                           │
  │  ┌──────────┬──────────┬──────────┐          │
  │  │AudioSlicer│ LyricFA │ HubertFA │ ← Tab分页 │
  │  │  (Tab 1)  │(Tab 2)  │ (Tab 3)  │          │
  │  └──────────┴──────────┴──────────┘          │
  │  各 Tab 独立操作，不提供一键串联               │
  └──────────────────────────────────────────────┘

  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐
  │ MinLabel.exe  │ │SlurCutter.exe│ │ GameInfer.exe│
  │  音频标注      │ │ MIDI句子编辑  │ │ 音频转MIDI   │
  └──────────────┘ └──────────────┘ └──────────────┘

  所有 4 个 EXE 共享:
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
┌─────────────────────────────────────────────────────────────────┐
│ DatasetPipeline.exe │ MinLabel.exe │ SlurCutter.exe │ GameInfer │
├─────────────────────┴──────────────┴────────────────┴───────────┤
│                      Shared Widgets Layer                        │
│  PlayWidget │ TaskWindow │ GpuSelector │ (dstools-widgets.dll)  │
├─────────────────────────────────────────────────────────────────┤
│                        Audio Layer                               │
│  AudioDecoder (FFmpeg) │ AudioPlayback (SDL2) │ (dstools-audio) │
├─────────────────────────────────────────────────────────────────┤
│                       Inference Layer                            │
│  OnnxEnv │ game-infer │ some-infer │ rmvpe-infer │ FunAsr       │
│          │ hubert-infer │ audio-util │                           │
├─────────────────────────────────────────────────────────────────┤
│                         Core Layer                               │
│  AppInit │ Config │ ErrorHandling │ ThreadUtils │ Theme          │
├─────────────────────────────────────────────────────────────────┤
│            External: Qt 6.8+, ONNX Runtime, SDL2, FFmpeg         │
└─────────────────────────────────────────────────────────────────┘
```

### 2.3 依赖方向

```
Apps (4 EXE) ──→ dstools-widgets ──→ dstools-audio ──→ dstools-core
    │                                     │
    └──→ Inference libs ─────────────────┘
              │
         (onnxruntime, fftw3, sndfile, soxr...)
```

规则：
- 箭头方向不可逆转
- 同层 App 不互相依赖
- dstools-widgets 是 SHARED 库（DLL），4 个 EXE 共享一份

---

## 3. 重构后目录结构

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
├── src/
│   ├── CMakeLists.txt
│   │
│   ├── core/                         # Core Layer
│   │   ├── CMakeLists.txt            # → dstools-core (STATIC)
│   │   ├── include/dstools/
│   │   │   ├── AppInit.h             # 统一初始化（字体/root/crash）
│   │   │   ├── Config.h              # 统一配置
│   │   │   ├── ErrorHandling.h       # Result<T> / Status
│   │   │   ├── ThreadUtils.h         # invokeOnMain
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
│   │   │   ├── PlayWidget.h          # 合并两份 PlayWidget
│   │   │   ├── TaskWindow.h          # 原 AsyncTaskWindow
│   │   │   └── GpuSelector.h         # 合并两份 DmlGpuUtils
│   │   └── src/
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
│   │   ├── hubert-infer/             # → hubert-infer (SHARED)，从 HubertFA 提取
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
│   │   ├── SlurCutter/               # ★ SlurCutter.exe
│   │   │   ├── CMakeLists.txt
│   │   │   ├── main.cpp
│   │   │   ├── gui/
│   │   │   │   ├── MainWindow.h/.cpp     # 保持 QMainWindow
│   │   │   │   ├── DsSentence.h
│   │   │   │   └── intervaltree.hpp
│   │   │   └── F0Widget/
│   │   │       ├── F0Widget.h/.cpp
│   │   │       ├── PianoRollRenderer.h/.cpp
│   │   │       ├── NoteDataModel.h/.cpp
│   │   │       ├── MusicUtils.h
│   │   │       └── UndoCommands.h/.cpp
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
    └── refactor/
```

### 3.1 与原结构映射

| 原路径 | 新路径 | 说明 |
|--------|--------|------|
| `src/apps/AudioSlicer/` | `src/apps/pipeline/slicer/` | 并入 Pipeline |
| `src/apps/LyricFA/` | `src/apps/pipeline/lyricfa/` | 并入 Pipeline |
| `src/apps/HubertFA/` | `src/apps/pipeline/hubertfa/` | 并入 Pipeline |
| `src/apps/MinLabel/` | `src/apps/MinLabel/` | 保留独立 EXE |
| `src/apps/SlurCutter/` | `src/apps/SlurCutter/` | 保留独立 EXE |
| `src/apps/GameInfer/` | `src/apps/GameInfer/` | 保留独立 EXE |
| `src/libs/qsmedia/` + plugins | `src/audio/` | 去掉插件体系 |
| `src/libs/AsyncTaskWindow/` | `src/widgets/TaskWindow` | 共享组件 |
| 两份 `PlayWidget` | `src/widgets/PlayWidget` | 合并 |
| 两份 `DmlGpuUtils` | `src/widgets/GpuSelector` | 合并 |
| `src/libs/FunAsr/` | `src/infer/funasr/` | 移入 infer 层 |
| `src/apps/HubertFA/util/Hfa.*` 等 | `src/infer/hubert-infer/` | 提取为库 |

---

## 4. CMake 构建目标

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
| `hubert-infer` | SHARED | audio-util, nlohmann_json, dstools-infer-common |
| `FunAsr` | STATIC | fftw3, dstools-infer-common |
| **`DatasetPipeline`** | **EXE** | dstools-widgets, audio-util, FunAsr, hubert-infer, SndFile, cpp-pinyin |
| **`MinLabel`** | **EXE** | dstools-widgets, cpp-pinyin, cpp-kana |
| **`SlurCutter`** | **EXE** | dstools-widgets |
| **`GameInfer`** | **EXE** | dstools-widgets, game-infer |

### 4.2 关键设计决策

**为什么 dstools-widgets 是 SHARED (DLL)?**

4 个 EXE 都链接它。如果是 STATIC，主题/样式代码会被编译进每个 EXE，增加体积且无法运行时统一更新主题。SHARED 确保：
- 一份 QSS 加载代码
- 一份 `Theme::palette()` 色板
- 一份 PlayWidget / TaskWindow / GpuSelector 实现
- 用户替换 `dstools-widgets.dll` 可同时更新所有 4 个 EXE 的外观

**为什么 3 个工具保持独立 EXE 而不并入 Pipeline?**

- MinLabel 是手工交互标注工具，有完整的文件树浏览器和音频播放器，工作模式与批处理完全不同
- SlurCutter 是 MIDI 钢琴卷帘编辑器，高度专业化的 UI，不适合嵌入选项卡
- GameInfer 是单文件推理工具，界面简单但独立运行更方便

### 4.3 部署结构

```
DatasetTools/                        # 安装目录
├── DatasetPipeline.exe              # 数据集处理管线（切片+识别+对齐）
├── MinLabel.exe                     # 音频标注
├── SlurCutter.exe                   # MIDI 编辑
├── GameInfer.exe                    # 音频转 MIDI
│
├── dstools-widgets.dll              # 共享样式/组件库（统一外观的关键）
├── audio-util.dll                   # 推理共享库
├── game-infer.dll
├── some-infer.dll
├── rmvpe-infer.dll
├── hubert-infer.dll
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
- 原先 6 个 EXE 混在 DLL 中 → 现在 4 个 EXE，名称清晰
- 新增 `DatasetPipeline.exe` 整合 3 个批处理工具
- `dstools-widgets.dll` 保证 4 个 EXE 视觉一致

---

## 5. DatasetPipeline 设计

### 5.1 界面布局

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

### 5.2 运行模式：分页独立运行

**每个 Tab 页独立操作，不提供一键串联运行。**

- 用户选择某个 Tab（AudioSlicer / LyricFA / HubertFA）
- 在该 Tab 中配置参数、添加文件
- 点击该 Tab 内的「Run」按钮
- 行为与原独立应用完全一致
- 用户需要手动在上一步完成后，切换到下一个 Tab，设置输入目录为上一步的输出目录

> **设计理由**: 三个工具虽然构成流水线，但实际使用中用户经常需要在每一步之后
> 检查结果、调整参数、手动修正错误文件，然后再进行下一步。强制串联反而降低了
> 灵活性。分页隔开让每个步骤保持独立，同时共享一个窗口减少了启动多个 EXE 的麻烦。

### 5.3 各 Tab 页保留的完整功能

| Tab | 原应用 | 功能（完全保留） |
|-----|--------|-----------------|
| 1 | AudioSlicer | 拖放文件、参数配置、WAV/CSV 输出、任务栏进度、输出格式选择 |
| 2 | LyricFA | 模型加载、ASR 识别、歌词匹配、差异高亮 |
| 3 | HubertFA | 模型加载、GPU 选择、多语言 G2P、TextGrid 输出 |

### 5.4 SlicerPage 特殊处理

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

---

## 6. 独立 EXE 的统一样式方案

### 6.1 共享方式

3 个独立 EXE（MinLabel, SlurCutter, GameInfer）通过链接 `dstools-widgets.dll` 获得统一样式：

```cpp
// 每个独立 EXE 的 main.cpp — 仅需 3 行即可获得统一外观
#include <dstools/AppInit.h>
#include <dstools/Theme.h>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    dstools::AppInit::init(app);                // 统一字体
    dstools::Theme::apply(app, dstools::Theme::Dark);  // 统一深色主题
    // ... 创建 MainWindow ...
}
```

### 6.2 统一内容

| 统一项 | 实现位置 | 效果 |
|--------|----------|------|
| 字体 | `AppInit::init()` | 4 个 EXE 都使用 Microsoft YaHei / Segoe UI |
| 深色/亮色主题 | `Theme::apply()` 加载 QSS | 所有 QWidget 控件统一配色 |
| 色板 | `Theme::palette()` | 自绘组件（F0Widget）也使用统一色板 |
| PlayWidget | `dstools-widgets.dll` | MinLabel/SlurCutter 用同一份播放组件 |
| GpuSelector | `dstools-widgets.dll` | GameInfer/HubertFA 用同一份 GPU 选择器 |
| 图标 | `resources.qrc` 编译进 `dstools-widgets.dll` | 4 个 EXE 共享同一套 SVG 图标 |

### 6.3 独立 EXE 保持 QMainWindow

MinLabel、SlurCutter、GameInfer 保持 `QMainWindow` 作为主窗口（不改为 QWidget Page），因为：
- 它们有各自的菜单栏（File/Edit/Help）
- 窗口大小、位置独立管理
- 不需要与其他工具共享窗口

变化仅限于：
1. `main.cpp` 调用 `AppInit::init()` + `Theme::apply()`（替代各自的字体设置代码）
2. 使用共享的 `PlayWidget`（替代各自的重复实现）
3. 使用共享的 `GpuSelector`（GameInfer）
4. 所有 UI 字符串用 `tr()` 包裹

---

## 7. 消除重复方案

| 重复代码 | 当前 | 合并到 | 消除方式 |
|----------|------|--------|----------|
| PlayWidget (75% 重复) | MinLabel + SlurCutter 各一份 | `dstools-widgets/PlayWidget` | 以 SlurCutter 版为基础合并 |
| DmlGpuUtils (100% 重复) | GameInfer + HubertFA 各一份 | `dstools-widgets/GpuSelector` | 合并为 QComboBox 组件 |
| ExecutionProvider (100%×4) | 4 个推理库各一份 | `dstools-infer-common/ExecutionProvider.h` | 统一枚举 |
| main.cpp 初始化 (~20行×6) | 6 个 main.cpp | `dstools-core/AppInit` | 统一初始化函数 |
| app.qss (100% 重复) | MinLabel + SlurCutter 各一份 | `resources/themes/` | 全局主题文件 |
| AsyncTaskWindow | 独立库 | `dstools-widgets/TaskWindow` | 归入共享层 |
| qsmedia 插件体系 | 3 个库 + 2 个插件 | `dstools-audio` | 直接集成，删除插件/工厂 |

---

## 8. 功能守恒验证清单

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

### SlurCutter
- [ ] .ds JSON 加载 + 句子导航
- [ ] F0 曲线 + MIDI 音符显示
- [ ] 分割/合并/移调/滑音
- [ ] 范围播放 + 播放头同步（共享 PlayWidget）
- [ ] 编辑追踪
- [ ] 缩放/滚动
- [ ] 深色主题

### GameInfer
- [ ] GAME 模型加载
- [ ] 音频 → MIDI 推理
- [ ] CPU/CUDA/DML 切换（共享 GpuSelector）
- [ ] MIDI 输出
- [ ] 深色主题
