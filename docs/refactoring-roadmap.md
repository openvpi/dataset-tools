# 架构重构路线图

> 基于 2026-05-02 代码审计更新。
>
> **原则**: 功能齐全、简单可靠、不过度设计。接口保持一致性和合理的扩展预留。
>
> **范围**: 在本仓库内完成。所有模块作为 CMake 子目录共存。
>
> **C++ 标准**: 允许 C++20（当前 C++20，已有 `std::filesystem` 使用）。

---

## 已完成阶段摘要

| Phase | 名称 | 主要成果 |
|-------|------|---------|
| 0 | 预备工作 | ModelType/DocumentFormat 泛化，Qt 依赖分离，pipeline 子模块库化 |
| 1 | 核心分离 | ModelManager→domain，dsfw-base 创建，OnnxModelBase，IInferenceEngine |
| 2 | 库边界固化 | 目录重排，版本化 API (DSFW_VERSION)，FunASR 适配器，pipeline 后端提取 |
| 3 | 框架增强 | Logger，Undo/Redo (ICommand+UndoStack)，EventBus，CLI 工具 (dstools-cli) |
| 4 | 完善与扩展 | PluginManager，CrashHandler，UpdateChecker，RecentFiles，WidgetGallery，新标准控件 |
| 5-6 | 深度优化 | Result\<T\> 统一，UI 推理解耦，Slicer 合并，MinLabelService 提取，StringUtils 提取，CI 矩阵构建，clang-tidy CI，CHANGELOG，线程安全验证，冗余依赖清理 |
| B | 测试与质量 | 24 个测试模块 (80+ 用例)，TODO/FIXME 清理，文件操作错误处理补全 |
| C | 代码质量 | PitchLabelerPage/PhonemeLabelerPage 大文件拆分 (→Setup 文件)，魔法数字常量化 (kDefaultBufferSize) |
| D | CI/CD | 框架独立编译 CI (verify-modules.yml)，Doxygen CI (docs.yml)，跨平台包分发 (release.yml) |
| G | 任务处理器架构 | 死代码清理 (11 个)，ITaskProcessor + Registry，4 个处理器迁移，CLI/Labeler/GameInfer 集成，旧服务接口删除 |
| H | 用户体验与可靠性 | AppPaths (QStandardPaths)，CrashHandler 统一，FileLogSink (7 天轮转)，PitchLabeler 撤销重做补全 (7 个 Command)，BatchCheckpoint (断点续处理) |
| I | CMake 现代化 | DstoolsHelpers.cmake，40+ CMakeLists.txt 迁移 (1045→237 行)，cmake 3.21，qt_standard_project_setup |
| J | 框架功能补全 | 窗口状态持久化，SingleInstanceGuard，RecentFilesManager，ToastNotification，TranslationManager (i18n) |
| K | 代码规范化 | #pragma once 统一，Doxygen 补全，命名统一 |
| F.1 | 示例项目 | minimal-appshell GUI 示例 |
| L.0 | 时间类型基础设施 | `TimePos = int64_t` 微秒类型 + `secToUs`/`usToSec`/`hzToMhz`/`mhzToHz` 转换 + 9 个精度测试 |
| L.0b | 曲线工具库 | `CurveTools`：重采样/无声插值/平滑/批量转换/对齐/crossfade + 23 个测试 |
| L.12 | 编译速度优化 | PCH（AUTOMOC 感知）、ccache/sccache 自动检测、MSVC /MP、测试分层（unit/integration/infer） |
| L.1 | Boundary/Layer 领域模型 | DsTextTypes.h（Boundary/IntervalLayer/CurveLayer/DsTextDocument）、.dstext JSON I/O、v1→v2 迁移 + 6 个测试 |
| L.2 | 流水线核心接口 | PipelineContext（状态/层数据/序列化）、IAudioPreprocessor、IFormatAdapter、FormatAdapterRegistry + 5 个测试 |
| L.3 | 格式适配器 | LabAdapter、TextGridAdapter、CsvAdapter、DsFileAdapter + 3 个测试 |

---

## Phase L — 层路由流水线 + 时间精度统一

> 设计文档：[task-processor-design.md](task-processor-design.md) v3 + [ds-format.md](ds-format.md) v2
>
> 目标：消除 CSV 中间格式，统一为层数据流转；int64 微秒时间精度；跨层边界绑定；MakeDiffSinger 兼容。

### 当前代码盘点

| 维度 | 现状 |
|------|------|
| 时间表示 | `double` 秒，遍布 ~90 个头文件声明、~200+ 实现站点 |
| .dstext I/O | **完全未实现**。格式已定义但无解析器/写入器 |
| Boundary 领域模型 | **不存在**。PhonemeLabeler 用 TextGrid C++ 库直接操作 textgrid::IntervalTier |
| PipelineContext | **不存在** |
| PipelineRunner | **不存在** |
| IFormatAdapter | **不存在** |
| IAudioPreprocessor | **不存在** |
| ITaskProcessor.processBatch | 仍存在（接口 + 2 个覆盖 + 默认实现） |
| TranscriptionRow | 仍是数据主干（11 个文件，88 处引用） |
| TranscriptionPipeline | 仍活跃（3 个文件） |
| BatchCheckpoint | 仍活跃（5 个消费者） |
| DsItemManager/DsItemRecord | 仍活跃（3 个文件） |
| QUndoCommand 子类 | PhonemeLabeler 5 个 + PitchLabeler 8 个 = 13 个，全部用 `double` 时间 |
| BoundaryBindingManager | 仅 PhonemeLabeler 使用，基于 `double` 容差比较 |
| F0Curve | 存在但用 `double` (Hz, 秒) |
| curve 层类型 | 不存在 |

---

### L.0 — 时间类型基础设施 + 精度回归测试 ✅

**目标**：引入 `TimePos` 类型和转换工具，在迁移前建立回归测试基线。

| 任务 | 文件 | 说明 |
|------|------|------|
| L.0.1 引入 `TimePos` 类型定义 | `src/types/include/dstools/TimePos.h` (新) | `using TimePos = int64_t;` + 转换函数 `secToUs(double)→TimePos`、`usToSec(TimePos)→double`、`usToMs(TimePos)→double` |
| L.0.2 精度回归测试 | `src/tests/types/TestTimePos.cpp` (新) | 测试 `double↔int64` 往返精度、MDS 6位小数兼容、边界条件（0、负值、极大值） |
| L.0.3 F0 值类型辅助 | `TimePos.h` 中追加 | `hzToMhz(double)→int32_t`、`mhzToHz(int32_t)→double` |

**依赖**：无。**影响范围**：纯新增文件，零破坏。

---

### L.0b — 曲线插值工具库 (CurveTools) ✅

**目标**：通用曲线重采样/插值/平滑工具，解决不同 F0 模型 hop_size/sample_rate 差异问题。

| 任务 | 文件 | 说明 |
|------|------|------|
| L.0b.1 CurveTools 头文件 | `src/domain/include/dstools/CurveTools.h` (新) | `resampleCurve`（线性插值重采样）、`interpUnvoiced`（log2域无声帧插值）、`movingAverage`/`medianFilter`（平滑）、`hzToMhzBatch` 等批量转换、`alignToLength`（截断/填充）、`smoothstepCrossfade`（曲线拼接过渡）、`hopSizeToTimestep`/`expectedFrameCount`（辅助计算） |
| L.0b.2 CurveTools 实现 | `src/domain/src/CurveTools.cpp` (新) | 等价于 MDS `get_pitch.py:resample_align_curve` + `interp_f0`。从 `Rmvpe.cpp:interp_f0` 和 `PitchProcessor::movingAverage` 提取泛化 |
| L.0b.3 CurveTools 测试 | `src/tests/domain/TestCurveTools.cpp` (新) | 19 个用例：重采样（identity/up/down/align/tail-fill/rmvpe-to-ds）、无声插值（边缘/中间/MDS兼容）、平滑、转换、对齐、crossfade |

**依赖**：L.0（TimePos 类型）。**影响范围**：纯新增。现有 `Rmvpe.cpp:interp_f0` 和 `PitchProcessor::movingAverage` 不修改（L.7 迁移时替换调用）。

---

### L.1 — Boundary / Layer 领域模型 + .dstext I/O ✅

**目标**：创建 ds-format.md §4 定义的数据模型和文件读写器。

| 任务 | 文件 | 说明 |
|------|------|------|
| L.1.1 Boundary / Layer 领域类型 | `src/domain/include/dstools/DsTextTypes.h` (新) | `struct Boundary { int id; TimePos pos; QString text; }`、`struct Layer { QString name; vector<Boundary> boundaries; }`、`struct DsTextDocument { ... layers, groups, audio }` |
| L.1.2 .dstext JSON 解析器 | `src/domain/src/DsTextDocument.cpp` (新) | 读取 v2 格式（int64 `pos`）；检测 v1 格式（float `position`）并自动迁移 |
| L.1.3 .dstext JSON 写入器 | 同上 | 写出 v2 格式，int64 `pos` |
| L.1.4 CurveLayer 类型 | `DsTextTypes.h` 中追加 | `struct CurveLayer { QString name; int64_t timestep; vector<int32_t> values; }` |
| L.1.5 .dstext 测试 | `src/tests/domain/TestDsTextDocument.cpp` (新) | 读写往返、v1→v2 迁移、边界/组验证、curve 层 |

**依赖**：L.0（TimePos 类型）。**影响范围**：纯新增，零破坏。

---

### L.2 — PipelineContext + IAudioPreprocessor + IFormatAdapter ✅

**目标**：创建 v3 流水线的核心新类型。

| 任务 | 文件 | 说明 |
|------|------|------|
| L.2.1 PipelineContext | `src/framework/core/include/dsfw/PipelineContext.h` (新) | 含 `layers` map、`status` 枚举、`completedSteps`、`stepHistory`、`buildTaskInput()`、`applyTaskOutput()`、JSON 序列化 |
| L.2.2 IAudioPreprocessor | `src/framework/core/include/dsfw/IAudioPreprocessor.h` (新) | `preprocessorId()`、`process(in, out, config)` |
| L.2.3 IFormatAdapter | `src/framework/core/include/dsfw/IFormatAdapter.h` (新) | `formatId()`、`canImport/canExport()`、`importToLayers()`、`exportFromLayers()` |
| L.2.4 FormatAdapterRegistry | `src/framework/core/include/dsfw/FormatAdapterRegistry.h` (新) | 单例注册表，按 formatId 查找适配器 |
| L.2.5 PipelineContext 测试 | `src/tests/framework/TestPipelineContext.cpp` (新) | buildTaskInput/applyTaskOutput、JSON 往返、status 传播 |

**依赖**：L.0。**影响范围**：纯新增，dsfw-core 新增 4 个头文件 + 实现。

---

### L.3 — 4 个格式适配器 ✅

**目标**：实现 MDS 兼容的导入/导出。

| 任务 | 文件 | 说明 |
|------|------|------|
| L.3.1 LabAdapter | `src/domain/src/adapters/LabAdapter.h/.cpp` (新) | 空格分隔音节 ↔ grapheme 层 |
| L.3.2 TextGridAdapter | `src/domain/src/adapters/TextGridAdapter.h/.cpp` (新) | TextGrid phones/words tier ↔ phoneme/grapheme 层。内部使用 textgrid.hpp + `TextGridToCsv` 现有代码 |
| L.3.3 CsvAdapter | `src/domain/src/adapters/CsvAdapter.h/.cpp` (新) | transcriptions.csv ↔ 各层。**内部用 TranscriptionRow** 做桥接 (ADR-33) |
| L.3.4 DsFileAdapter | `src/domain/src/adapters/DsFileAdapter.h/.cpp` (新) | .ds JSON ↔ 各层。内部用 DsDocument |
| L.3.5 适配器测试 | `src/tests/domain/TestFormatAdapters.cpp` (新) | 每种格式的导入→导出往返测试 |

**依赖**：L.2（IFormatAdapter）。**影响范围**：新增文件，包装现有代码，不修改现有代码。

---

### L.4 — PipelineRunner

**目标**：流水线执行器，替代 TranscriptionPipeline 的角色。

| 任务 | 文件 | 说明 |
|------|------|------|
| L.4.1 StepConfig 类型 | `src/framework/core/include/dsfw/PipelineRunner.h` (新) | `StepConfig`：taskName、processorId、config、optional、manual、import/exportFormat、validator |
| L.4.2 PipelineRunner 实现 | `src/framework/core/src/PipelineRunner.cpp` (新) | 整首歌步骤 vs 逐切片步骤、discard 跳过、快照保存、validator 调用、context 持久化 |
| L.4.3 ValidationCallback + 内置验证器 | `src/framework/core/include/dsfw/PipelineValidators.h` (新) | SliceLengthValidator、PitchCoverageValidator |
| L.4.4 PipelineRunner 测试 | `src/tests/framework/TestPipelineRunner.cpp` (新) | 多步骤执行、discard 传播、快照/恢复、validator 触发 |

**依赖**：L.2（PipelineContext）、L.3（FormatAdapters）。**影响范围**：纯新增。

---

### L.5 — 新增处理器包装

**目标**：将现有纯算法包装为 ITaskProcessor。

| 任务 | 文件 | 说明 |
|------|------|------|
| L.5.1 SlicerProcessor | `src/libs/slicer/SlicerProcessor.h/.cpp` (新) | 包装 `SlicerService`，输出 `slices` 层。TaskSpec: `{"audio_slice", {}, [{slices, slices}]}` |
| L.5.2 AddPhNumProcessor | `src/libs/minlabel/AddPhNumProcessor.h/.cpp` (新) | 包装 `PhNumCalculator`，输入 phoneme+grapheme 层，输出 ph_num 层 |
| L.5.3 DiscardSliceCommand | `src/framework/core/include/dsfw/DiscardSliceCommand.h` (新) | QUndoCommand：设置/清除 PipelineContext::status |
| L.5.4 处理器测试 | `src/tests/libs/TestSlicerProcessor.cpp`, `TestAddPhNumProcessor.cpp` (新) | |

**依赖**：L.2（PipelineContext）。**影响范围**：新增文件，不修改现有 SlicerService / PhNumCalculator。

---

### L.6 — ITaskProcessor 接口精简 (Breaking Change)

**目标**：移除 `processBatch`，批量逻辑归 PipelineRunner。

| 任务 | 文件 | 说明 |
|------|------|------|
| L.6.1 移除 ITaskProcessor::processBatch | `ITaskProcessor.h` | 删除虚方法声明 |
| L.6.2 移除默认 processBatch 实现 | `TaskProcessorRegistry.cpp` | 删除 L53-L115 |
| L.6.3 移除 BatchInput / BatchOutput | `TaskTypes.h` | 删除 L46-L59 |
| L.6.4 适配 GameMidiProcessor | `GameMidiProcessor.h/.cpp` | 删除 processBatch 覆盖，保留 process() |
| L.6.5 适配 RmvpePitchProcessor | `RmvpePitchProcessor.h/.cpp` | 删除 processBatch 覆盖，保留 process() |
| L.6.6 适配 GameInfer MainWidget | `MainWidget.cpp` | 从 processBatch 调用改为循环 process() |
| L.6.7 适配 HubertFAPage | `HubertFAPage.h/.cpp` | 从 processBatch/BatchCheckpoint 改为循环 process() |
| L.6.8 适配 CLI | `cli/main.cpp` | 不再构造 BatchInput |
| L.6.9 适配示例 | `examples/minimal-task-processor/` | 移除 processBatch 示例 |
| L.6.10 更新测试 | `TestTaskProcessorRegistry.cpp` | 移除 processBatch 测试 |

**依赖**：L.4（PipelineRunner 可用作替代）。**影响范围**：4 个处理器 + 3 个应用消费者 + 1 个示例 + 1 个测试。

---

### L.7 — F0Curve / DSFile 时间类型迁移

**目标**：领域类型从 `double` 秒/Hz 迁移到 `int64` 微秒/mHz。引入 CurveTools 重采样。

| 任务 | 文件 | 说明 |
|------|------|------|
| L.7.1 F0Curve 迁移 | `F0Curve.h/.cpp` | `double timestep` → `TimePos timestep`、`vector<double> values` → `vector<int32_t> values` (mHz)、所有 API 方法签名。`getValueAtTime`/`getRange`/`setRange` 使用 CurveTools 内部算法 |
| L.7.2 DSFile 迁移 | `DSFile.h/.cpp` (PitchLabeler) | `Phone::start/duration` → `TimePos`、`Note::duration/start` → `TimePos`、`offset` → `TimePos` |
| L.7.3 PitchLabeler Commands 迁移 | `NoteCommands.h/.cpp`, `PitchCommands.h/.cpp` | 3 个含时间字段的 QUndoCommand 迁移 |
| L.7.4 PitchProcessor 迁移 | `PitchProcessor.h/.cpp` | 用 CurveTools::movingAverage 替换内部 movingAverage、用 CurveTools::smoothstepCrossfade 替换内部实现 |
| L.7.5 RmvpePitchProcessor 增加重采样 | `RmvpePitchProcessor.cpp` | process() 中调用 `CurveTools::resampleCurve` 将 RMVPE 10ms 输出重采样到目标 timestep（从 config 读取 hopSize/sampleRate） |
| L.7.6 更新测试 | `TestF0Curve.cpp` | 适配 int64/int32 类型 |

**依赖**：L.0 + L.0b（TimePos + CurveTools）。**影响范围**：PitchLabeler + RmvpePitchProcessor。可与 L.1-L.6 **并行**执行。

---

### L.8 — PhonemeLabeler 时间类型迁移

**目标**：PhonemeLabeler 的边界/绑定系统从 `double` 迁移到 `TimePos`。

| 任务 | 文件 | 说明 |
|------|------|------|
| L.8.1 TextGridDocument 迁移 | `TextGridDocument.h/.cpp` | `moveBoundary(int, int, double)` → `TimePos`、`boundaryTime()` → `TimePos`、信号 `boundaryMoved(int, int, double)` → `TimePos`。内部仍用 textgrid.hpp（double），在 getter/setter 边界做 `secToUs`/`usToSec` 转换 |
| L.8.2 BoundaryBindingManager 迁移 | `BoundaryBindingManager.h/.cpp` | `AlignedBoundary::time` → `TimePos`、`findAlignedBoundaries` / `createLinkedMoveCommand` 参数。容差判断改为 `abs(a - b) <= toleranceUs`（整数比较） |
| L.8.3 5 个 QUndoCommand 迁移 | `MoveBoundary/Insert/Remove/LinkedMove/SetIntervalText` `.h/.cpp` | 所有 `double m_oldTime/m_newTime/m_savedTime` → `TimePos` |
| L.8.4 5 个 Widget 迁移 | `IntervalTierView`, `PowerWidget`, `SpectrogramWidget`, `WaveformWidget`, `BoundaryOverlayWidget` | `m_dragStartTime` 等内部变量 → `TimePos`。`xToTime/timeToX` 返回/接受 `TimePos`。**渲染/像素计算仍用 double**（仅在 widget 内部转换） |
| L.8.5 EntryListPanel 迁移 | `EntryListPanel.h/.cpp` | `startTime/endTime` → `TimePos` |
| L.8.6 PhonemeLabelerPage 信号适配 | `PhonemeLabelerPageSetup.cpp` | 适配 `boundaryMoved(int, int, TimePos)` 信号连接 |

**依赖**：L.0（TimePos 类型）。**影响范围**：PhonemeLabeler 内部。可与 L.7 **并行**执行。

---

### L.9 — DiffSingerLabeler 集成

**目标**：DiffSingerLabeler 的 9 步 wizard 切换到 PipelineRunner。

| 任务 | 文件 | 说明 |
|------|------|------|
| L.9.1 BuildCsvPage → PipelineRunner | `BuildCsvPage.h/.cpp` | 不再直接操作 TextGrid + TranscriptionCsv，改为调用 PipelineRunner + TextGridAdapter + CsvAdapter |
| L.9.2 GameAlignPage → PipelineRunner | `GameAlignPage.h/.cpp` | 使用 PipelineRunner 调度 GameMidiProcessor |
| L.9.3 BuildDsPage → PipelineRunner | `BuildDsPage.h/.cpp` | 使用 PipelineRunner 调度 RmvpePitchProcessor + DsFileAdapter |
| L.9.4 TaskWindowAdapter 适配 | `TaskWindowAdapter.h/.cpp` | 适配 PipelineRunner 的 progress/manual-step 信号 |
| L.9.5 切片丢弃 UI | 新增 UI 组件 | 列表灰显 + 右键丢弃/恢复 + DiscardSliceCommand |

**依赖**：L.4（PipelineRunner）、L.5（SlicerProcessor）、L.6（processBatch 已移除）。

---

### L.10 — 遗留清理

**目标**：标记/移除被 PipelineRunner 替代的旧代码。

| 任务 | 文件 | 说明 |
|------|------|------|
| L.10.1 TranscriptionPipeline deprecated | `TranscriptionPipeline.h/.cpp` | 添加 `[[deprecated]]`，保留编译但标注不再推荐 |
| L.10.2 DsItemManager/DsItemRecord deprecated | `DsItemManager.h/.cpp`, `DsItemRecord.h` | PipelineContext 替代 |
| L.10.3 BatchCheckpoint 使用范围收缩 | `BatchCheckpoint.h/.cpp` | 从 processBatch 默认实现中解除，仅保留给遗留应用（SlicerPage, HubertFAPage 在 L.6 中已迁移后可移除引用） |
| L.10.4 DsProjectDefaults 遗留字段清理 | `DsProject.h/.cpp` | 移除 `asrModelPath/hubertModelPath/gameModelPath/rmvpeModelPath` 遗留字段，统一到 `taskModels` map |

**依赖**：L.9（集成完成后确认无消费者仍依赖旧代码）。

---

### L.11 — .dsproj 规范落地

**目标**：让 DsProject 类支持 ds-format.md v2 + task-processor-design.md 的完整 .dsproj 结构。

| 任务 | 文件 | 说明 |
|------|------|------|
| L.11.1 DsProject 扩展 | `DsProject.h/.cpp` | 新增 `schema`、`tasks`（含 granularity/optional/manual/importFormats/exportFormats）、`defaults.preprocessors`、`defaults.slicer`、`defaults.validation`、`defaults.export`。`items[].slices[].status/discardReason/discardedAtStep`。时间字段 int64 微秒。 |
| L.11.2 DsProject 测试 | `src/tests/domain/TestDsProject.cpp` (新) | 读写往返、v1 兼容迁移 |

**依赖**：L.1（DsTextTypes）。可与 L.2-L.6 **并行**。

---

### L.12 — 编译速度优化 ✅

**目标**：在不牺牲稳定性的前提下，显著缩短全量/增量编译时间。

**现状盘点**（编译单元约 400）：

| 维度 | 现状 |
|------|------|
| .cpp 文件数 | 190（不含 onnxruntime vendor） |
| .h 文件数 | 209 |
| CMake 目标数 | ~41（18 libraries + 9 executables + 3 infer tests + 11 domain/fw tests） |
| 预编译头 (PCH) | **未配置** |
| 编译器缓存 (ccache) | **未配置** |
| Unity Build | **未配置** |
| AUTOMOC 目标 | ~15 个（每个自动生成 moc_*.cpp） |

#### L.12.1 预编译头 (PCH) — 预期效果最大

**文件**：`cmake/DstoolsHelpers.cmake` 修改 + 各 CMakeLists.txt 不变

在 `dstools_add_library` / `dstools_add_executable` 中自动注入 PCH：

```cmake
# DstoolsHelpers.cmake 新增：
if(NOT _type STREQUAL "INTERFACE" AND NOT ARG_NO_PCH)
    # 框架层 PCH：Qt 核心 + STL 常用头
    target_precompile_headers(${target_name} PRIVATE
        <QString>
        <QStringList>
        <QObject>
        <QDebug>
        <vector>
        <map>
        <memory>
        <string>
        <functional>
        <cstdint>
        <nlohmann/json_fwd.hpp>   # json 前向声明（非完整头）
    )
endif()
```

**要点**：
- 用 `nlohmann/json_fwd.hpp`（前向声明）而非 `nlohmann/json.hpp`（18000行），在 PCH 中只放前向声明
- 推理层（infer/）可选使用 `NO_PCH` 跳过（vendor 代码编译选项可能不兼容）
- 每个 CMake 目标独立 PCH（CMake 3.16+ 原生支持），不共享跨目标 PCH（避免隐式依赖）
- `dstools_add_executable` 同理

**验收**：增量编译改单个 .cpp 时，PCH 不会重新生成。全量编译时间预期减少 30-50%。

#### L.12.2 编译器缓存 (ccache/sccache)

**文件**：`CMakeLists.txt` 根目录新增

```cmake
# 自动检测并启用 ccache/sccache
find_program(CCACHE_PROGRAM ccache)
find_program(SCCACHE_PROGRAM sccache)
if(SCCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
    message(STATUS "Using sccache: ${SCCACHE_PROGRAM}")
elseif(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
endif()
```

**要点**：
- 仅在检测到工具时自动启用，不强制依赖
- CI 环境和本地开发均受益
- sccache 优先（支持 MSVC 原生 /Zi）；ccache 回退
- 对 MSVC：需要 `/Z7` 替代 `/Zi`（ccache 不兼容 .pdb），或使用 sccache

**MSVC 注意事项**：

```cmake
if(CCACHE_PROGRAM AND NOT SCCACHE_PROGRAM AND MSVC)
    # ccache 不兼容 MSVC 的 /Zi（.pdb 文件锁），需改用 /Z7（内嵌调试信息）
    string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
    string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
endif()
```

**验收**：`cmake --build . --target all` 二次构建（无修改）近乎瞬间完成。

#### L.12.3 nlohmann/json 前向声明

**问题**：`nlohmann/json.hpp` 约 18000 行，被 `TaskTypes.h`、`DsProject.h`、`DsDocument.h` 等公共头文件 include，传播到几乎所有编译单元。

**方案**：公共头文件中只 include `nlohmann/json_fwd.hpp`（~200行），完整 `json.hpp` 仅在 .cpp 中 include。

| 文件 | 变更 |
|------|------|
| `TaskTypes.h` | `#include <nlohmann/json.hpp>` → `#include <nlohmann/json_fwd.hpp>` + 前向声明 `ProcessorConfig` 为 `nlohmann::json` |
| `DsProject.h` | 同上 |
| `DsDocument.h` | 同上（`nlohmann::json` 在 API 中仅以引用/指针形式出现） |
| `ITaskProcessor.h` | 已通过 `TaskTypes.h` 间接 include |
| `PipelineContext.h` (新) | 从一开始就用 `json_fwd.hpp` |
| `DsTextTypes.h` (新) | 同上 |
| 各 `.cpp` 文件 | 添加 `#include <nlohmann/json.hpp>` |

**约束**：
- 若头文件中需要 `nlohmann::json` 的完整类型（如成员变量），则用 `std::unique_ptr<nlohmann::json>` 或保留完整 include
- `ProcessorConfig`（typedef 为 `nlohmann::json`）在头文件中只能以引用/指针形式出现

**评估**：`TaskTypes.h` 中 `ProcessorConfig` 直接是 `nlohmann::json`，作为 `TaskInput.config` 的成员变量需要完整类型。因此 `TaskTypes.h` 无法改为 `json_fwd.hpp`。但 **`DsProject.h`** 中 `m_extraFields` 是 `nlohmann::json` 成员，可以改为 `std::unique_ptr` 或保持。实际收益评估：**中等**。优先做 PCH（L.12.1）和 ccache（L.12.2），json 前向声明为可选优化。

#### L.12.4 /MP 并行编译（MSVC）

**文件**：`cmake/DstoolsHelpers.cmake`

```cmake
if(MSVC)
    target_compile_options(${target_name} PRIVATE /MP)
endif()
```

**现状**：已有 `/utf-8 /W4 /wd4251`，但缺少 `/MP`（多核并行编译）。Ninja 生成器已支持跨目标并行，但 `/MP` 额外启用**单目标内多文件并行**。

**验收**：MSVC 构建日志出现 `/MP` 标志。

#### L.12.5 AUTOMOC 优化

**问题**：15 个目标启用 AUTOMOC，每个目标 CMake 重新生成时都会扫描所有头文件检测 `Q_OBJECT`。

**方案**：
- 不关闭 AUTOMOC（太多手动 moc 工作量）
- 改为设置 `CMAKE_AUTOMOC_DEPEND_FILTERS`（减少不必要的重扫描）
- 确保 `AUTOMOC_MOC_OPTIONS` 包含 `-b` (moc batch mode, CMake 3.27+)

```cmake
# 根 CMakeLists.txt:
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.27)
    set(CMAKE_AUTOMOC_EXECUTABLE_OPTIONS "-b")  # batch mode
endif()
```

**验收**：增量构建时 moc 不会对未修改文件重新运行。

| 任务 | 优先级 | 预期效果 | 风险 |
|------|--------|---------|------|
| L.12.1 PCH | P0 | 全量编译 -30~50% | 低（CMake 原生特性） |
| L.12.2 ccache/sccache | P0 | 清理后重编译 -80%+ | 低（可选启用） |
| L.12.4 /MP | P0 | MSVC 全量 -20~30% | 无（标准 MSVC 选项） |
| L.12.3 json_fwd | P2 | 单文件编译 -10~20% | 中（API 约束） |
| L.12.5 AUTOMOC batch | P2 | 增量构建 -5~10% | 低 |

**依赖**：无。**可与所有 L 阶段并行执行。**

---

## 阶段依赖图

```
L.0 ─────┬───────────────────────────────────────────────────────┐
         │                                                       │
         ▼                                                       │
L.0b (CurveTools)                                                │
         │                                                       │
         ├───────────────────────────────────────┐               │
         ▼                                       ▼               ▼
L.1 (Boundary/Layer/.dstext)          L.7 (F0Curve/DSFile 迁移，引用 CurveTools)
         │                                       │
         ▼                            L.8 (PhonemeLabeler 迁移)
L.2 (PipelineContext/IFormatAdapter)             │
         │                                       │
         ├──────────┐                            │
         ▼          ▼                            │
L.3 (Adapters)   L.5 (新处理器)                  │
         │          │                            │
         ▼          │                            │
L.4 (PipelineRunner)                             │
         │                                       │
         ▼                                       │
L.6 (processBatch 移除)                          │
         │                                       │
         ▼                                       │
L.9 (DiffSingerLabeler 集成) ◄───────────────────┘
         │
         ▼
L.10 (遗留清理)

L.11 (.dsproj 规范) ── 独立，可与 L.2-L.6 并行
L.12 (编译速度优化) ── 完全独立，可与所有阶段并行
```

**关键路径**：L.0 → L.0b → L.2 → L.3 → L.4 → L.6 → L.9 → L.10

**并行工作流**：
- 流水线主线（上方路径）
- 时间类型迁移线（L.0 → L.7 ∥ L.8，可与主线完全并行）
- .dsproj 规范线（L.0 → L.1 → L.11，与主线并行）

---

## 任务统计

| 阶段 | 新增文件 | 修改文件 | 删除/废弃 | 新增测试 |
|------|---------|---------|----------|---------|
| L.0 | 2 | 0 | 0 | 1 |
| L.0b | 2 | 0 | 0 | 1 |
| L.1 | 3 | 0 | 0 | 1 |
| L.2 | 5 | 1 (CMakeLists) | 0 | 1 |
| L.3 | 9 | 1 (CMakeLists) | 0 | 1 |
| L.4 | 4 | 1 (CMakeLists) | 0 | 1 |
| L.5 | 5 | 2 (CMakeLists) | 0 | 2 |
| L.6 | 0 | 12 | 0 | 1 (更新) |
| L.7 | 0 | 8 | 0 | 1 (更新) |
| L.8 | 0 | 14 | 0 | 0 (补 L.0 测试) |
| L.9 | 1 | 5 | 0 | 0 |
| L.10 | 0 | 6 | 0 | 0 |
| L.11 | 1 | 2 | 0 | 1 |
| L.12 | 0 | 3 | 0 | 0 |
| **合计** | **32** | **55** | **0 (仅 deprecated)** | **11** |

---

## 优先级排序建议

| 优先级 | 阶段 | 理由 |
|--------|------|------|
| **P0 — 立即** | L.0, L.0b, L.12.1/12.2/12.4 | 基础设施 + 编译加速。PCH/ccache//MP 无依赖，立即可做 |
| **P1 — 本周** | L.1, L.2 | 核心数据模型和接口，解锁所有后续开发 |
| **P1 — 本周** | L.7 | 与主线并行，PitchLabeler 内部改动。依赖 CurveTools 替换内部 interp/smooth |
| **P2 — 下周** | L.3, L.4, L.5 | 流水线功能完整 |
| **P2 — 下周** | L.8 | 与主线并行，PhonemeLabeler 内部改动 |
| **P2 — 下周** | L.11, L.12.3/12.5 | .dsproj 规范 + 可选编译优化 |
| **P3 — 第三周** | L.6 | Breaking change，需要 L.4 就绪 |
| **P3 — 第三周** | L.9 | 集成，需要 L.4 + L.6 |
| **P4 — 收尾** | L.10 | 确认无遗留消费者后清理 |

---

## 架构决策记录 (ADR)

| ADR | 决策 | 理由 |
|-----|------|------|
| ADR-1 | dsfw-base 为 Qt-free 静态库 | 支持 CLI 工具和非 Qt 消费者 |
| ADR-2 | ModelType 改为 int ID 注册表 | 框架层不感知业务模型类型 |
| ADR-3 | OnnxModelBase protected 继承 | 各推理 DLL 保持现有 API 稳定 |
| ADR-5 | dsfw-audio (FFmpeg/SDL2) 归框架 | 音频播放/解码是通用桌面能力 |
| ADR-7 | FunASR 永远不直接修改 | vendor 代码只用适配器包装 |
| ADR-8 | 单仓库模式 | 降低维护复杂度 |
| ADR-9 | 允许 C++20 | 编译器均已支持 |
| ADR-21 | 统一用 dsfw::CrashHandler | 替换 QBreakpad |
| ADR-22 | QStandardPaths 数据路径 | applicationDirPath() 在 macOS/Linux 下可能只读 |
| ADR-23 | 直接用 QUndoStack | Qt QUndoStack 已成熟 |
| ADR-24 | CMake GLOB_RECURSE CONFIGURE_DEPENDS | 通过 helper 函数封装 |
| ADR-25 | 推理库保留独立命名风格 | API 面向领域用户 |
| ADR-30 | 层数据保持 nlohmann::json | 离线处理，JSON 开销可忽略 |
| ADR-31 | PipelineContext 用 category 做扁平键 | 与 .dsproj tasks 的 category 绑定一致 |
| ADR-32 | 移除 processBatch | 批量逻辑归 PipelineRunner |
| ADR-33 | TranscriptionRow 降级为适配器内部 | 最小迁移爆炸半径 |
| ADR-34 | 切片命名 {prefix}_{NNN}.wav | 按时间顺序，兼容 MDS |
| ADR-35 | LyricFA 以整首歌为粒度 | ASR 需要完整上下文 |
| ADR-36 | 同 taskName 处理器 I/O 必须一致 | Registry 注册时验证 |
| ADR-37 | Context JSON 替代 .dsitem + BatchCheckpoint | 统一一个文件 |
| ADR-38 | 音频预处理独立接口 | 操作音频文件，不产出层数据 |
| ADR-39 | 切片丢弃通过 status + 传播 | 简单可靠，可撤销 |
| ADR-40 | 自动步骤用快照替代细粒度撤销 | 重跑 = 撤销 |
| ADR-41 | 导入/导出格式声明在 task 定义中 | .dsproj tasks 含 importFormats/exportFormats |
| ADR-42 | MDS 兼容通过格式适配器实现 | 不在核心引入 MDS 目录约定 |
| ADR-43 | int64 微秒时间精度 | 消除浮点累积误差，整数比较无容差 |
| ADR-44 | 边界字段 `pos` (int64 μs) 替代 `position` (float sec) | 缩短字段名 + 精度统一 |
| ADR-45 | F0 用 int32 毫赫兹存储 | 避免浮点精度问题，足够覆盖人声范围 |

---

## 关联文档

- [phase-l-implementation.md](phase-l-implementation.md) — **Phase L 细化实施方案**（逐任务接口签名、精确修改清单、验收标准）
- [ds-format.md](ds-format.md) — .dsproj / .dstext 格式规范 v2
- [task-processor-design.md](task-processor-design.md) — 流水线架构设计 v3
- [framework-architecture.md](framework-architecture.md) — 框架架构
- [architecture.md](architecture.md) — 项目架构概述

> 更新时间：2026-05-03
