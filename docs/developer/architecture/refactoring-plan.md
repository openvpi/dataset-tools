# dataset-tools 精简重构方案（最终版）

> 版本：3.0.0
> 日期：2026-06-08
> 状态：方案设计阶段
> 修订说明：v2.0 方案中 Phase 2（目录重组）、Phase 3（合并单文件模块）、Phase 5（代码规范统一）经逐项核实属于"为重构而重构"——变更风险与收益不成比例。v3.0 仅保留真正关键且稳定的优化项。

---

## 0. 前置声明

### 0.1 方案目标

1. 在保证当前全部功能和行为完全不变的前提下，对项目进行合理适度的精简重构
2. 消除有实际危害的代码质量问题
3. 遵循 [human-decisions.md](../../human-decisions.md) 全部设计准则
4. 不得破坏现有的扩展性和模块解耦

### 0.2 方案约束

- 功能零退化：所有现有功能保持完全一致，行为不变
- 数据零风险：所有文件 I/O 操作保持原子性，数据完整性校验不变
- 编译零新增警告：重构后编译通过，无新增 warning
- 不引入任何新的外部依赖
- 变更粒度：每个任务独立提交，不推送

### 0.3 取舍原则

**保留标准**：满足以下全部条件才纳入方案：
1. 问题真实存在且已通过代码验证
2. 修复后能消除实际风险（编译风险、命名冲突风险、维护风险）
3. 变更风险极低（机械替换，编译器可验证）
4. 不涉及大规模文件移动或目录重组

**剔除标准**：满足以下任一条件即剔除：
1. 纯组织性调整（目录重组、单文件模块合并）—— 无实际危害，收益不抵 churn
2. 已有合规实现（如信号槽已全部使用新式语法）
3. 变更范围过大、风险不可控（如 PIMPL 隔离第三方头文件涉及大量重构）

---

## 1. 现状分析

### 1.1 已识别问题清单

| 编号 | 问题 | 严重度 | 是否纳入 | 说明 |
|------|------|--------|----------|------|
| P1 | 头文件中 `using namespace dsfw;` 污染命名空间 | **高** | **是** | 违反 SUP-02，可能导致名称遮蔽和编译错误 |
| P2 | `data-sources/` 目录为 catch-all | 中 | **否** | 仅组织问题，无实际危害。30+ 文件移动风险与收益不匹配 |
| P3 | `dstools-ui-core` 仅含 1 个类 | 低 | **否** | 模块虽小但功能明确，合并不产生实际收益 |
| P4 | `bridges/`、`log-page/`、`mouth-curve-chart/` 各仅含 1 个类 | 低 | **否** | 同 P3，单类模块无实际危害 |
| P5 | `audio-library-design.md` 状态过时 | 低 | **是** | 零风险，纯文档更新 |

### 1.2 P1 详析：头文件 `using namespace` 分布

经全项目扫描，`src/` 下共有 **33 个头文件**（v2.0 方案漏计 10 个，且误列入 1 个）包含 `using namespace dsfw;`：

| 目录 | 文件数 | 文件列表 |
|------|--------|----------|
| `src/domain/include/dstools/` | 8 | `DsProject.h`, `IEditorDataSource.h`, `DomainInit.h`, `CsvAdapter.h`, `CsvToDsConverter.h`, `ExportFormats.h`, `LayerSerialization.h`, `ModelManager.h`, `TextGridToCsv.h` |
| `src/domain/src/adapters/` | 2 | `LabAdapter.h`, `TextGridAdapter.h` |
| `src/engine/adapters/slicer/` | 2 | `SlicerProcessor.h`, `SlicerService.h` |
| `src/engine/adapters/rmvpe-pitch/` | 1 | `RmvpePitchProcessor.h` |
| `src/engine/adapters/min-label-lib/` | 1 | `AddPhNumProcessor.h` |
| `src/engine/adapters/hubert-fa/` | 1 | `HubertAlignmentProcessor.h` |
| `src/engine/adapters/game-infer-lib/` | 1 | `GameMidiProcessor.h` |
| `src/apps/shared/audio-visualizer/` | 2 | `AudioVisualizerContainer.h`（含 2 处）, `MiniMapScrollBar.h` |
| `src/apps/shared/bridges/` | 1 | `DsTextDocBridge.h` |
| `src/apps/shared/chart-framework/` | 5 | `IBoundaryModel.h`, `ChartPanelBase.h`, `BoundaryDragController.h`, `ChartConfigRegistry.h`, `MoveBoundaryCommand.h` |
| `src/apps/shared/phoneme-editor/ui/` | 4 | `IntervalTierView.h`, `SliceBoundaryModel.h`, `TextGridDocument.h`, `TierEditWidget.h` |
| `src/apps/shared/phoneme-editor/ui/commands/` | 1 | `BoundaryCommands.h` |
| `src/apps/shared/pitch-editor/ui/` | 1 | `NoteBoundaryModel.h` |
| `src/apps/shared/pitch-editor/ui/commands/` | 2 | `NoteCommands.h`, `SplitMergeCommands.h` |

> **v2.0 方案勘误**：
> - 漏计 10 个文件：`LayerSerialization.h`, `ModelManager.h`, `TextGridToCsv.h`, `SlicerProcessor.h`, `SlicerService.h`, `RmvpePitchProcessor.h`, `AddPhNumProcessor.h`, `HubertAlignmentProcessor.h`, `GameMidiProcessor.h`, `MiniMapScrollBar.h`
> - 误列入 1 个文件：`PitchCommands.h`（经核查不包含 `using namespace dsfw`）

**实际危害**：包含这些头文件的编译单元自动引入 `dsfw` 命名空间中的所有符号（`Result<T>`, `TimePos`, `PathUtils`, `ServiceLocator`, `SettingsKey<T>` 等数十个符号），可能导致：
- 非预期的名称遮蔽（如局部变量名与 `dsfw` 符号冲突）
- 依赖隐式命名空间引入，移除某个 `#include` 后编译失败
- 增加编译器符号查找负担

**修复方式**：机械替换 — 将头文件中的 `using namespace dsfw;` 移除，裸符号前加上 `dsfw::` 前缀。.cpp 文件中的 `using namespace` 不受影响。

---

## 2. 重构方案

### 2.1 阶段一：消除头文件命名空间污染（P1）

**目标**：移除所有 33 个头文件中的 `using namespace dsfw;`。

**变更范围**：33 个头文件 + 对应 .cpp 文件同步调整。

**变更示例**：

```cpp
// 变更前 (DsProject.h)
namespace dstools {
    using namespace dsfw;

    class DsProject {
        Result<void> load(const QString &path);
        TimePos duration() const;
    };
}

// 变更后
namespace dstools {
    class DsProject {
        dsfw::Result<void> load(const QString &path);
        dsfw::TimePos duration() const;
    };
}
```

**风险**：极低。纯机械替换，编译器可验证正确性。每个文件独立变更，不影响其他编译单元。

**验证**：编译通过，无新增错误。

**执行步骤**：
1. 按目录分批处理，每批 5-8 个文件
2. 每批编译验证通过后提交
3. 共约 4-5 批提交

---

### 2.2 阶段二：更新音频模块文档（P5）

**目标**：更新 `docs/developer/architecture/audio-library-design.md` 使其与实际实现一致。

**变更**：
- 将文档状态从"方案设计阶段"改为"已实施"
- 标记已完成的设计目标
- 移除已过时的章节

**风险**：零。纯文档更新，不涉及代码。

---

## 3. 实施计划

### 3.1 阶段划分

| 阶段 | 内容 | 优先级 | 风险 | 影响范围 |
|------|------|--------|------|----------|
| 一 | 消除头文件命名空间污染 | 高 | 极低 | 33 个头文件 + 对应 .cpp |
| 二 | 更新音频模块文档 | 低 | 零 | 1 个文档 |

### 3.2 执行顺序

1. 阶段一：消除头文件 `using namespace`（优先执行）
2. 阶段二：更新文档

### 3.3 每个阶段的执行流程

1. 执行代码变更
2. 编译通过（CLion MCP 增量编译）
3. 提交（不推送）

---

## 4. 不做的事及原因

| 内容 | 剔除原因 |
|------|----------|
| 整理 `data-sources/` 目录（v2.0 Phase 2） | 纯组织性调整，30+ 文件移动 + include 路径更新，风险中等，收益仅限代码导航便利性。属于"为重构而重构" |
| 合并 `dstools-ui-core` 到 domain（v2.0 Phase 3） | 模块虽小但功能明确（崩溃恢复/字体/权限），独立 CMake 模块无实际危害。合并不产生实际收益 |
| 合并单文件模块（bridges/log-page/mouth-curve-chart） | 单类模块无实际危害，合并仅减少目录嵌套，不解决任何真实问题 |
| 信号槽语法统一（v2.0 Phase 5） | 经核查，项目已全部使用新式信号槽语法（成员函数指针），无 `SIGNAL()`/`SLOT()` 宏残留 |
| PIMPL 隔离第三方头文件（v2.0 Phase 5） | 涉及 `nlohmann/json`、`onnxruntime` 等头文件暴露，需要大量重构，风险不可控。当前无实际危害（这些是稳定依赖，不会频繁变更） |
| constexpr 优先（v2.0 Phase 5） | 经核查，`Constants.h` 等已使用 `constexpr`，无需额外修改 |

---

## 5. SUP 原则参考

以下 SUP-01~SUP-10 原则作为编码规范参考，在日常开发中遵循，不单独设立重构任务：

| 编号 | 原则 | 当前状态 |
|------|------|----------|
| SUP-01 | 头文件自包含 | 持续遵循 |
| SUP-02 | 禁止头文件 `using namespace` | 阶段一执行 |
| SUP-03 | PIMPL 隔离第三方头文件 | 新代码遵循，存量暂不强制重构 |
| SUP-04 | 单一编译单元原则 | 持续遵循 |
| SUP-05 | 最小化公开 API | 持续遵循 |
| SUP-06 | constexpr 优先 | 已合规 |
| SUP-07 | 零开销抽象 | 持续遵循 |
| SUP-08 | 信号槽类型安全 | 已合规 |
| SUP-09 | 资源生命周期显式化 | 持续遵循 |
| SUP-10 | 目录职责单一 | 持续遵循 |