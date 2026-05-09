# 技术摘要与架构债文档

> 版本：1.0
> 日期：2026-05-06
> 自动生成：基于代码库分析

---

## 一、项目概况

| 指标 | 值 |
|------|-----|
| 语言 | C++20 |
| UI 框架 | Qt 6.8+ (Widgets) |
| 构建系统 | CMake 3.21+ / Ninja / vcpkg |
| 推理运行时 | ONNX Runtime (DirectML/CUDA/CPU) |
| 源文件数 | 460 |
| 总代码行数 | ~60,000 |
| CMake 模块 | 49 |
| 单元测试 | 39 文件 / ~5,000 行 |
| 目标平台 | Windows 10-11 (主), macOS 11+, Linux |
| 产出物 | LabelSuite.exe, DsLabeler.exe, dstools-cli.exe |

---

## 二、模块架构

```
┌─────────────────────────────────────────────────────────┐
│  Applications (apps/)                                    │
│  ┌───────────┐  ┌───────────┐  ┌────────┐  ┌────────┐  │
│  │LabelSuite │  │ DsLabeler │  │  CLI   │  │Gallery │  │
│  └─────┬─────┘  └─────┬─────┘  └────────┘  └────────┘  │
│        └───────┬───────┘                                 │
│     ┌──────────┴──────────────┐                          │
│     │   Shared (apps/shared/) │                          │
│     │  data-sources           │  ← EditorPageBase        │
│     │  phoneme-editor         │  ← PhonemeEditor         │
│     │  pitch-editor           │  ← PitchEditor           │
│     │  audio-visualizer       │  ← AudioVisualizerContainer│
│     │  min-label-editor       │                          │
│     │  settings               │                          │
│     └─────────────────────────┘                          │
├─────────────────────────────────────────────────────────┤
│  Domain (domain/)        ~4,200 行                       │
│  DsProject, DsTextTypes, IEditorDataSource,             │
│  TranscriptionPipeline, CurveTools, ExportFormats       │
├─────────────────────────────────────────────────────────┤
│  Framework (framework/)  ~8,400 行                       │
│  ┌────────┐ ┌─────────┐ ┌────────┐ ┌───────┐ ┌──────┐ │
│  │  core  │ │ ui-core │ │widgets │ │ audio │ │ infer│ │
│  │AppSettings│AppShell │ │ViewportCtrl│AudioDecoder│OnnxEnv│
│  │ServiceLocator│Theme│ │TimeRuler│ │AudioPlayer│OnnxModel│
│  │PipelineRunner│    │ │PlayWidget│ │        │       │ │
│  └────────┘ └─────────┘ └────────┘ └───────┘ └──────┘ │
├─────────────────────────────────────────────────────────┤
│  Infer (infer/)          ~19,600 行                      │
│  audio-util, hubert-infer, rmvpe-infer, game-infer,     │
│  FunAsr (ASR engine)                                     │
├─────────────────────────────────────────────────────────┤
│  Libs (libs/)            ~2,900 行                       │
│  hubert-fa, rmvpe-pitch, game-infer-lib, lyric-fa,      │
│  slicer, min-label-lib, textgrid                        │
├─────────────────────────────────────────────────────────┤
│  Types (types/)          ~130 行 (header-only)           │
│  Result<T>, TimePos (int64 μs), ExecutionProvider       │
└─────────────────────────────────────────────────────────┘
```

### 层依赖方向

```
apps → domain → types
apps → framework → types
apps → libs → infer → types
framework ← (不依赖 domain/apps)
```

---

## 三、关键设计模式

### 3.1 Resolution 驱动的视口系统

```
ViewportController (resolution: int, samples/pixel)
  ├── resolutionTable: [10,20,30,40,60,80,100,150,200,300,400]
  ├── PPS = sampleRate / resolution (派生量)
  ├── zoomIn/zoomOut → 离散步进，无无极缩放
  └── viewportChanged(ViewportState) → 广播到所有子图
```

所有页面（Slicer、PhonemeLabeler）通过 AudioVisualizerContainer 统一管理：
- MiniMapScrollBar (缩略图 + 视口窗口)
- TimeRulerWidget (查表法刻度线)
- TierLabelArea (层级标签/边界指示)
- QSplitter → N 个 chart widgets
- BoundaryOverlayWidget (透明覆盖层，绘制贯穿线)
- ScaleLabel (右下角比例尺指示器)

### 3.2 IBoundaryModel + 容器通知 (P-02)

纯虚数据接口，无 QObject 信号。变更通知由 `AudioVisualizerContainer::invalidateBoundaryModel()` 统一分发。

### 3.3 EditorPageBase 模板方法

三个编辑器页面（MinLabel、Phoneme、Pitch）继承 EditorPageBase，获得：
- SliceListPanel + QSplitter 布局
- 生命周期：onActivated → restoreSplitter → ensureSelection → onAutoInfer
- 切片切换：maybeSave → onSliceSelectedImpl
- 导航 actions (prev/next)
- 异步引擎加载 (loadEngineAsync)

### 3.4 Result\<T\> 错误传播

应用层禁止异常，所有可失败操作返回 `Result<T>`。try-catch 仅在 ONNX/JSON/FFmpeg 第三方边界。

### 3.5 DsProject / DsTextDocument 数据模型

- **DsProject** (.dsproj): 工程级配置，含 items/slices/slicer state
- **DsTextDocument** (.dstext): 单切片标注数据（IntervalLayer + CurveLayer）
- **PipelineContext**: 切片处理状态（dirty layers、已完成步骤）

---

## 四、构建产出物

| 类型 | 文件 | 描述 |
|------|------|------|
| GUI 应用 | LabelSuite.exe | 多页面 AppShell (10 pages) |
| GUI 应用 | DsLabeler.exe | 项目驱动标注器 (7 pages) |
| CLI | dstools-cli.exe | 命令行批处理 |
| 共享库 | dsfw-widgets.dll | 框架 UI 组件 |
| 共享库 | dstools-widgets.dll | 应用级 UI 组件 |
| 共享库 | audio-util.dll | 音频解码/重采样 |
| 共享库 | hubert-infer.dll | HuBERT-FA 推理引擎 |
| 共享库 | rmvpe-infer.dll | RMVPE F0 估计引擎 |
| 共享库 | game-infer.dll | GAME MIDI 转写引擎 |
| 测试 | Test*.exe | 39 个单元测试可执行文件 |

---

## 五、架构债清单

### 🔴 高优先级 (影响稳定性/可维护性)

| # | 债务 | 当前状态 | 影响 | 建议修复 |
|---|------|---------|------|---------|
| TD-01 | **QMetaMethod 反射连接不可靠** | `AudioVisualizerContainer::connectViewportToWidget()` 通过 QMetaMethod::invoke 做运行时反射连接，如果 widget 的 `setViewport` 签名变化会静默失败 | Slicer/Phoneme 缩放可能失效 | 改为 C++ lambda + `qobject_cast<>` 类型检查，或定义 `IViewportAware` 接口 |
| TD-02 | **infer/ 模块代码量过大 (19,600 行)** | FunAsr 单独占 ~10,000 行，含大量复制粘贴的底层代码 | 编译慢，修改风险高 | FunAsr 应作为外部 submodule 或 prebuilt library |
| TD-03 | **Spectrogram 未做增量计算** | `computeSpectrogramRange` 对变化区域重新 FFT，但 `rebuildViewImage` 每次重绘完整 QImage | 缩放/滚动时 CPU 占用高 | 分块缓存 + 仅重绘可见区域差异部分 |
| TD-04 | **PhonemeEditor 的 BoundaryOverlay 依赖 mapTo(parentWidget)** | `repositionOverSplitter()` 使用 `m_trackedWidget->mapTo(parentWidget(), ...)` 计算位置 | 嵌套布局变化时 overlay 位置错位 | 改为 eventFilter 监听所有祖先 widget 的 Move/Resize 事件，或使用 QWidget::stackUnder 机制 |
| TD-05 | **WaveformWidget/SpectrogramWidget/PowerWidget 大量代码重复** | 三个 widget 各自实现 hitTestBoundary、startBoundaryDrag、updateBoundaryDrag、endBoundaryDrag (~150行/each) | 修改拖拽逻辑要改三处 | 提取 `BoundaryDragMixin` 基类或组合对象 |
| TD-06 | ✅ **已修复: AudioPlayer 数据竞争 + 双重所有权** | `AudioPlayer` 直接访问 `AudioDecoder`（无锁），与 SDL 回调线程并发读写；`m_decoder` unique_ptr 与 `AudioPlayback::Impl::decoder` 共享同一裸指针 | 右键播放崩溃 (0xc0000005) | 2026-05-07 已修复: 移除 m_decoder，路由到 AudioPlayback 线程安全方法 |
| TD-07 | ✅ **已修复: PlayWidget ServiceLocator 全局共享** | 第一个 PlayWidget 注册 IAudioPlayer 到 ServiceLocator，后续页面被追共享同一播放器 | 跨页面音频冲突 | 2026-05-07 已修复: 移除共享逻辑，每个 PlayWidget 自有 AudioPlayer (D-31) |
| TD-08 | ✅ **已修复: 4 个 PlayWidget 无 parent 内存泄漏** | PhonemeEditor/MinLabelEditor/DsSlicerPage/SlicerPage 的 PlayWidget 未设置 QObject parent | AppShell 析构野指针崩溃 | 2026-05-07 已修复: 全部添加 this parent |
| TD-09 | ✅ **已修复: WindowStateFilter 裸指针** | `WindowStateFilter` 持有 `TitleBar*` 裸指针，析构顺序不确定导致 use-after-free | AppShell 析构崩溃 | 2026-05-07 已修复: 改用 QPointer<TitleBar> |
| TD-10 | ✅ **已修复: ModelManager invalidateModel 顺序** | `invalidateModel` 先 unload 再 emit signal，页面无法阻止后台任务使用已销毁引擎 | 推理引擎 use-after-free | 2026-05-07 已修复: 先 emit 再 unload (D-32) |

### 🟡 中优先级 (影响开发效率)

| # | 债务 | 当前状态 | 影响 | 建议修复 |
|---|------|---------|------|---------|
| TD-11 | **SlicerPage 与 DsSlicerPage 功能重复** | SlicerPage (LabelSuite) 和 DsSlicerPage (DsLabeler) 约 60% 代码重复 | 修复 bug 需同改两处 | 将 DsSlicerPage 改为 SlicerPage 的子类，仅 override 工程保存逻辑 |
| TD-12 | **PhonemeTextGridTierLabel 未使用 IBoundaryModel::tierName()** | 标签区域只显示序号不显示层名（如 "phoneme"、"grapheme"） | 用户难区分层 | 增加 `IBoundaryModel::tierName(int)` 接口，TierLabel 绘制时显示 |
| TD-13 | **TextGridDocument 内部使用 textgrid.hpp 的 IntervalTier 指针** | 暴露第三方库内部类型到 UI 代码 | textgrid.hpp 升级时影响面大 | 包装为纯内部接口，UI 层只通过 IBoundaryModel 访问 |
| TD-14 | **dstools namespace 用于框架和应用** | `dstools` 既是 domain 层又是 apps 层的命名空间 | 不清楚类归属 | domain→`dstools`, apps→`dstools::app`, framework→`dsfw` (已分) |
| TD-15 | **ExportService 的 autoComplete 仍有遗留的 processEvents** | 虽然路线图标记已删除，但实际代码中可能残留 | UI 假死 | 全面审查并移入 QtConcurrent |
| TD-16 | **MiniMapScrollBar::setAudioData 中调用 setTotalDuration** | 缩略图加载音频后会覆盖 viewport 的 totalDuration，即使 loadAudioFile 已通过 setAudioParams 设置 | 可能重置采样率配置 | 移除 MiniMapScrollBar 中的 setTotalDuration 调用 |
| TD-17 | ⚠ **QtConcurrent::run 推理引擎悬空指针 (残留风险)** | PitchLabelerPage、MinLabelPage、ExportPage 尚未添加 engine-alive 令牌（PhonemeLabelerPage 已修复） | 模型卸载期间后台任务可能崩溃 | 遵循 P-09 模式，为所有推理页面添加 `std::shared_ptr<std::atomic<bool>>` 存活令牌 |

### 🟢 低优先级 (代码质量)

| # | 债务 | 当前状态 | 建议 |
|---|------|---------|------|
| TD-18 | 未使用的 unused variable warnings (paddedSize, halfWindow, yBot) | 编译警告 | 清理或 `[[maybe_unused]]` |
| TD-19 | `DsItemManager` 标记 deprecated 但仍在 DsSlicerPage 使用 | C4996 warning | 迁移到 PipelineContext |
| TD-20 | QSettings 键名不统一 | 部分硬编码字符串，部分用 SettingsKey | 全部迁移到 SettingsKey |
| TD-21 | 翻译文件 (.ts) 多数为空 | en.qm 全部 0 translations | 要么删除 en 翻译文件，要么补充英文翻译 |
| TD-22 | ~~`docs/refactoring-roadmap-v2.md` 未更新完成状态~~ | 已合并到 refactoring-plan.md | 已完成 |
| TD-23 | IntervalTierView 手动绝对定位 | 可能在 DPI 缩放时错位 | 使用 QGridLayout 或按 DPI 系数计算 |

---

## 六、模块间依赖图 (CMake targets)

```
dsfw-core ← dsfw-base
dsfw-ui-core ← dsfw-core, Qt::Widgets, QWindowKit
dsfw-widgets ← dsfw-core, dsfw-ui-core, Qt::Widgets
dstools-domain ← dsfw-core, dstools-types
dstools-audio ← dsfw-core, Qt::Multimedia/SDL
dstools-infer-common ← dstools-types, OnnxRuntime
audio-util ← dstools-infer-common, sndfile, soxr, fftw3
hubert-infer ← dstools-infer-common, audio-util
rmvpe-infer ← dstools-infer-common, audio-util
game-infer ← dstools-infer-common, audio-util
FunAsr ← dstools-infer-common
phoneme-editor ← dsfw-widgets, dstools-domain
audio-visualizer ← phoneme-editor, dsfw-widgets
data-sources ← phoneme-editor, audio-visualizer, pitch-editor, min-label-editor, all infer libs
LabelSuite ← data-sources, settings-page
DsLabeler ← data-sources, settings-page, dstools-domain
```

---

## 七、测试覆盖现状

| 模块 | 测试文件 | 覆盖范围 |
|------|---------|---------|
| framework/core | TestAppSettings, TestAsyncTask, TestJsonHelper, TestPipelineContext, TestPipelineRunner, TestServiceLocator, TestTaskProcessorRegistry, TestModelManager, TestModelDownloader, TestMockInference, TestOnnxModelBase | 核心框架接口 |
| framework/io | TestLocalFileIOProvider, TestIDocument, TestIFileIOProviderMock | 文件 I/O 抽象 |
| framework/audio | TestAudioProcessing | 音频解码/重采样 |
| domain | TestDsDocument, TestDsTextDocument, TestTranscriptionCsv, TestTranscriptionPipeline, TestTextGridToCsv, TestCsvToDsConverter, TestF0Curve, TestPitchUtils, TestCurveTools, TestPhNumCalculator, TestStringUtils, TestFormatAdapters, TestProjectPaths, TestPitchProcessor, TestPitchExtractionService, TestExportServiceMock, TestSlicerService | 领域逻辑 |
| types | TestTimePos, TestResult | 基础类型 |
| libs | TestMinLabelService, TestSlicerProcessor | 库逻辑 |

**缺失覆盖**：
- UI 组件（PhonemeEditor、AudioVisualizerContainer、WaveformWidget 等）— 无集成测试
- 端到端工作流（加载音频 → FA → 导出）
- 跨平台路径处理 (PathCompat)

---

## 八、性能热点

| 场景 | 热点 | 当前耗时估计 | 瓶颈 |
|------|------|-------------|------|
| 打开大音频 (>5min) | WaveformWidget::rebuildMinMaxCache | ~200ms | 逐像素遍历 samples |
| 缩放/滚动 | SpectrogramWidget::rebuildViewImage | ~100-500ms | 全画面 QImage 重建 |
| 批量 FA (100+ slices) | QtConcurrent::run(hfa->recognize) | ~2s/slice | ONNX 推理 |
| 项目打开 | ProjectDataSource::ensureAudioScanned | ~50ms/100 slices | 逐文件 stat() |
| 首次推理 | ONNX Session 创建 (DirectML) | ~3-5s | GPU 上下文初始化 |

---

## 九、外部依赖版本锁

| 依赖 | 最低版本 | 当前锁定 | 风险 |
|------|---------|---------|------|
| Qt | 6.8.0 | 6.9.3-6.10.0 | Qt 7 迁移遥远，当前安全 |
| ONNX Runtime | 1.16+ | via cmake setup | DirectML API 稳定 |
| nlohmann/json | 3.11+ | vcpkg latest | 接口稳定 |
| fftw3 | 3.3+ | vcpkg | API 几十年不变 |
| yaml-cpp | 0.7+ | vcpkg | 仅用于 dict 文件 |
| sndfile | 1.1+ | vcpkg | 稳定 |
| textgrid.hpp | HEAD | vendored | 无版本管理，需关注 |
| SDL | 2.x/3.x | vcpkg | 仅音频输出 |

---

## 十、建议优先偿还顺序

1. **TD-05** (拖拽代码重复) — 最高 ROI，三处改一处
2. **TD-01** (QMetaMethod 连接) — 静默失败最危险
3. **TD-11** (MiniMap setTotalDuration) — 简单修复，防止采样率重置
4. **TD-06** (Slicer 页面重复) — 中等工作量，大幅减少维护负担
5. **TD-03** (Spectrogram 性能) — 用户体验热点

---

## 附录 A：目录结构索引

```
src/
├── types/          ← Result<T>, TimePos, ExecutionProvider (header-only)
├── framework/
│   ├── base/       ← JsonHelper
│   ├── core/       ← AppSettings, ServiceLocator, Pipeline, Model interfaces
│   ├── ui-core/    ← AppShell, Theme, FramelessHelper
│   ├── widgets/    ← ViewportController, TimeRuler, PlayWidget, Toast
│   ├── audio/      ← AudioDecoder, AudioPlayer
│   └── infer/      ← OnnxEnv, OnnxModelBase, IInferenceEngine
├── domain/         ← DsProject, DsTextDocument, IEditorDataSource, Exports
├── infer/
│   ├── audio-util/ ← Slicer, SndfileVio, PathCompat, FlacDecoder
│   ├── hubert-infer/ ← HFA (HuBERT Forced Alignment)
│   ├── rmvpe-infer/  ← RMVPE (Pitch Estimation)
│   ├── game-infer/   ← GAME (Note-level MIDI Transcription)
│   └── FunAsr/       ← FunASR (Speech Recognition)
├── libs/
│   ├── hubert-fa/    ← HubertAlignmentProcessor (高层封装)
│   ├── rmvpe-pitch/  ← RmvpePitchProcessor
│   ├── game-infer-lib/ ← GameMidiProcessor
│   ├── lyric-fa/     ← LyricAlignment, ASR Pipeline
│   ├── slicer/       ← SlicerService, SlicerProcessor
│   ├── min-label-lib/ ← MinLabelService, PhNum
│   └── textgrid/     ← textgrid.hpp (vendored)
├── widgets/        ← dstools-widgets (BaseFileListPanel, GpuSelector, etc.)
├── ui-core/        ← dstools-ui-core (AppInit)
├── apps/
│   ├── label-suite/  ← LabelSuite main + pages
│   ├── ds-labeler/   ← DsLabeler main + pages
│   ├── shared/
│   │   ├── data-sources/    ← EditorPageBase, SlicerPage, *LabelerPage, services
│   │   ├── phoneme-editor/  ← PhonemeEditor + all UI widgets
│   │   ├── pitch-editor/    ← PitchEditor + PianoRollView
│   │   ├── audio-visualizer/ ← AudioVisualizerContainer + MiniMap + TierLabels
│   │   ├── min-label-editor/ ← MinLabelEditor
│   │   └── settings/        ← SettingsPage + AppSettingsBackend
│   ├── cli/          ← dstools-cli
│   └── widget-gallery/ ← WidgetGallery (开发测试用)
└── tests/          ← 39 test executables
```
