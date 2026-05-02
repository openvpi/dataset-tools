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

---

## Phase L — 层路由流水线 + 时间精度统一

> 设计文档：[task-processor-design.md](task-processor-design.md) v3 + [ds-format.md](ds-format.md) v2
>
> 目标：消除 CSV 中间格式，统一为层数据流转；int64 微秒时间精度；跨层边界绑定；MakeDiffSinger 兼容。

### 当前代码盘点（L.8 完成后）

| 维度 | 现状 |
|------|------|
| 时间表示 | `TimePos` (int64 μs) 已覆盖 F0Curve, DSFile, PhonemeLabeler 全栈。遗留 `double` 仅在 Widget 渲染层 |
| .dstext I/O | ✅ DsTextDocument 已实现（v2 格式，v1→v2 自动迁移） |
| Boundary 领域模型 | ✅ DsTextTypes.h (Boundary/IntervalLayer/CurveLayer/DsTextDocument) |
| PipelineContext | ✅ 已实现（状态/层数据/序列化/JSON 持久化） |
| PipelineRunner | ✅ 已实现（多步骤执行/验证/丢弃传播/格式导入导出） |
| IFormatAdapter | ✅ 4 个适配器（Lab/TextGrid/CSV/DS） |
| IAudioPreprocessor | ✅ 接口已定义 |
| ITaskProcessor.processBatch | ✅ 已移除，批量逻辑归 PipelineRunner |
| TranscriptionRow | 仍是数据主干（降级为适配器内部使用，ADR-33） |
| TranscriptionPipeline | 仍活跃，待 L.10 deprecated |
| DsItemManager/DsItemRecord | 仍活跃，待 L.10 deprecated |
| QUndoCommand 子类 | ✅ 全部已迁移到 TimePos |
| BoundaryBindingManager | ✅ 已迁移到 TimePos，整数容差比较 |
| F0Curve | ✅ 已迁移到 TimePos + int32 mHz |
| CurveTools | ✅ 重采样/插值/平滑/批量转换 |

---

### L.0 — 时间类型基础设施 + 精度回归测试 ✅

`TimePos = int64_t` 微秒类型 + `secToUs`/`usToSec`/`hzToMhz`/`mhzToHz` 转换 + 9 个精度测试。

---

### L.0b — 曲线插值工具库 (CurveTools) ✅

`CurveTools`：重采样/无声插值/平滑/批量转换/对齐/crossfade + 23 个测试。

---

### L.1 — Boundary / Layer 领域模型 + .dstext I/O ✅

DsTextTypes.h（Boundary/IntervalLayer/CurveLayer/DsTextDocument）、.dstext JSON I/O、v1→v2 迁移 + 6 个测试。

---

### L.2 — PipelineContext + IAudioPreprocessor + IFormatAdapter ✅

PipelineContext（状态/层数据/序列化）、IAudioPreprocessor、IFormatAdapter、FormatAdapterRegistry + 5 个测试。

---

### L.3 — 4 个格式适配器 ✅

LabAdapter、TextGridAdapter、CsvAdapter、DsFileAdapter + 3 个测试。

---

### L.4 — PipelineRunner ✅

PipelineRunner（多步骤执行/验证/丢弃传播/格式导入导出）+ 4 个测试。

---

### L.5 — 新增处理器包装 ✅

SlicerProcessor（包装 SlicerService）、AddPhNumProcessor（包装 PhNumCalculator）+ 2 个测试。

---

### L.6 — ITaskProcessor 接口精简 ✅

移除 `processBatch`/`BatchInput`/`BatchOutput`，批量逻辑归 PipelineRunner。4 个处理器 + 3 个应用消费者 + 1 个示例适配。

---

### L.7 — F0Curve / DSFile 时间类型迁移 ✅

F0Curve/DSFile/PitchLabeler Commands 从 `double` 秒/Hz 迁移到 `int64` 微秒/mHz。PitchProcessor 改用 CurveTools。RmvpePitchProcessor 增加重采样。

---

### L.8 — PhonemeLabeler 时间类型迁移 ✅

TextGridDocument/BoundaryBindingManager/5 个 QUndoCommand/5 个 Widget/EntryListPanel 从 `double` 迁移到 `TimePos`。

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

PCH（AUTOMOC 感知）、ccache/sccache 自动检测、MSVC /MP、测试分层（unit/integration/infer）。

---

## 阶段依赖图

```
L.0 ✅ ──┬──────────────────────────────────────────────────────┐
         │                                                      │
         ▼                                                      │
L.0b ✅ (CurveTools)                                            │
         │                                                      │
         ├──────────────────────────────────────┐               │
         ▼                                      ▼               ▼
L.1 ✅ (Boundary/Layer/.dstext)       L.7 ✅ (F0Curve/DSFile 迁移)
         │                                      │
         ▼                           L.8 ✅ (PhonemeLabeler 迁移)
L.2 ✅ (PipelineContext/IFormatAdapter)          │
         │                                      │
         ├──────────┐                           │
         ▼          ▼                           │
L.3 ✅ (Adapters) L.5 ✅ (新处理器)              │
         │          │                           │
         ▼          │                           │
L.4 ✅ (PipelineRunner)                         │
         │                                      │
         ▼                                      │
L.6 ✅ (processBatch 移除)                       │
         │                                      │
         ▼                                      │
L.9 (DiffSingerLabeler 集成) ◄──────────────────┘
         │
         ▼
L.10 (遗留清理)

L.11 (.dsproj 规范) ── 独立，可与 L.2-L.6 并行
L.12 ✅ (编译速度优化) ── 完全独立
```

**关键路径**：L.0 → L.0b → L.2 → L.3 → L.4 → L.6 → L.9 → L.10

**并行工作流**：
- 流水线主线（上方路径）
- 时间类型迁移线（L.0 → L.7 ∥ L.8，可与主线完全并行）
- .dsproj 规范线（L.0 → L.1 → L.11，与主线并行）

---

## 任务统计

| 阶段 | 新增文件 | 修改文件 | 删除/废弃 | 新增测试 | 状态 |
|------|---------|---------|----------|---------|------|
| L.0 | 2 | 0 | 0 | 1 | ✅ |
| L.0b | 2 | 0 | 0 | 1 | ✅ |
| L.1 | 3 | 0 | 0 | 1 | ✅ |
| L.2 | 5 | 1 (CMakeLists) | 0 | 1 | ✅ |
| L.3 | 9 | 1 (CMakeLists) | 0 | 1 | ✅ |
| L.4 | 4 | 1 (CMakeLists) | 0 | 1 | ✅ |
| L.5 | 5 | 2 (CMakeLists) | 0 | 2 | ✅ |
| L.6 | 0 | 12 | 0 | 1 (更新) | ✅ |
| L.7 | 0 | 8 | 0 | 1 (更新) | ✅ |
| L.8 | 0 | 14 | 0 | 0 (补 L.0 测试) | ✅ |
| L.9 | 1 | 5 | 0 | 0 | 待做 |
| L.10 | 0 | 6 | 0 | 0 | 待做 |
| L.11 | 1 | 2 | 0 | 1 | 待做 |
| L.12 | 0 | 3 | 0 | 0 | ✅ |
| **合计** | **32** | **55** | **0 (仅 deprecated)** | **11** | |

---

## 优先级排序建议

| 优先级 | 阶段 | 理由 |
|--------|------|------|
| **P1 — 下一步** | L.9 | DiffSingerLabeler 集成，所有依赖已就绪 |
| **P2 — 之后** | L.11 | .dsproj 规范落地，独立于 L.9 |
| **P3 — 收尾** | L.10 | 确认无遗留消费者后清理，依赖 L.9 完成 |

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

> 更新时间：2026-05-02
