# 标脏机制设计

> 层依赖脏数据 + 步骤级重标提示，实现"上游修改 → 下游自动感知并提示重新标注"
>
> 关联文档：pipeline.md §3（流水线 I/O 契约）、ds-format.md §6（层依赖 DAG）

---

## 1 现状分析

### 1.1 设计文档与代码实现状态

[ds-format.md §6](ds-format.md#L517-L638) 完整定义了层依赖 DAG 和脏数据失效机制，包括：

- 层依赖 DAG（grapheme → phoneme → ph_num → midi，pitch 独立）
- dirty 标记（per-slice，`PipelineContext.dirty`）
- 失效传播规则（修改 grapheme → phoneme/ph_num/midi 全 dirty）
- 自动重算时机（切换页面、导出时）
- 详细的用户回退场景

**以上已全部在代码中实现**，核心传播逻辑位于 `PipelineContext::propagateDirty()`（BFS 遍历 DAG），页面通过 `EditorPageBase::markLayersModified()` → `ProjectDataSource::addDirtyLayers()` 链完成脏层传播。

### 1.2 代码已具备的基础设施

| 组件 | 文件 | 状态 |
|------|------|------|
| `PipelineContext.dirty` 字段 | [PipelineContext.h](../src/framework/core/include/dsfw/PipelineContext.h#L44) | ✅ 已定义，持久化到 Context JSON |
| `ProjectDataSource::dirtyLayers()` | [ProjectDataSource.cpp](../src/apps/ds-labeler/core/ProjectDataSource.cpp#L84-L89) | ✅ 从内存 PipelineContext 读取 |
| `ProjectDataSource::clearDirtyLayers()` | [ProjectDataSource.cpp](../src/apps/ds-labeler/core/ProjectDataSource.cpp#L91-L99) | ✅ 清除指定 dirty 层并持久化 |
| ExportPage dirty 统计 | [ExportPage.cpp](../src/apps/ds-labeler/ui/ExportPage.cpp#L775-L800) | ✅ 统计并展示 dirty 数量 |
| `MinLabelPage::onAutoInfer()` 清除 grapheme dirty | [MinLabelPage.cpp](../src/apps/shared/data-sources/MinLabelPage.cpp#L259-L270) | ✅ 仅清除自身层 dirty，无传播 |
| `PhonemeLabelerPage::isDirty()` | PhonemeLabelerPage.cpp | ✅ 检查 phoneme 层 dirty |
| `PitchLabelerPage::isDirty()` | PitchLabelerPage.cpp | ✅ 检查 pitch/midi 层 dirty |

### 1.3 过去缺失（已全部修复）

| # | 原缺失项 | 修复状态 | 实现位置 |
|---|---------|:---:|------|
| 1 | 无脏层传播逻辑 | ✅ | `PipelineContext::propagateDirty()` — BFS 层依赖 DAG 传播 |
| 2 | 流水线不感知 dirty | ✅ | 通过 `ProjectDataSource::addDirtyLayers()` 在流水线执行前传播；步骤级 `deriveDirtySteps()` 推导 dirty 步骤 |
| 3 | 无步骤级标脏 | ✅ | `PipelineContext::deriveDirtySteps()` — 基于 I/O 契约从层 dirty 推导步骤 dirty |
| 4 | 保存时无 dirty 更新 | ✅ | `EditorPageBase::markLayersModified()` → 所有页面 `saveCurrentSlice()` 调用 |
| 5 | 页面切换无自动检测 | ✅ | `EditorPageBase::onAutoInfer()` — 检查 dirty 并跳过 manuallyEdited 步骤；三页面均有实现 |

---

## 2 设计目标

### 2.1 两种标脏维度

| 维度 | 粒度 | 含义 | 示例 |
|------|------|------|------|
| **层级标脏** (Layer Dirty) | 层 / category | 某层数据因上游修改而过期，重算后可恢复 | grapheme 修改 → phoneme/ph_num/midi dirty |
| **步骤级标脏** (Step Dirty) | Pipeline 步骤 | 某步骤因上游修改需要重新执行/审核 | 在 MinLabel 修改 → Step 4~10 全部需要重新标注 |

两者的关系：**层级标脏是步骤级标脏的数据基础**。步骤级标脏通过分析 `stepHistory` + `dirty` 推导得出。

### 2.2 核心原则

- **P-01（ARCH-01 职责单一）**：dirty 传播算法集中在一处（`DirtyTracker` / PipelineContext 方法），不分散在各页面
- **P-16（ARCH-07 开闭原则）**：扩展 PipelineContext 和 EditorPageBase，不修改 PipelineRunner 核心流程
- **P-07（ROBUST-03 简洁可靠）**：DAG 遍历确定 dirty，不设计复杂的增量检测
- **P-15（ARCH-06 依赖倒置）**：页面通过 `IEditorDataSource` 接口查询 dirty，不直接操作 PipelineContext

---

## 3 层依赖 DAG 与失效传播

### 3.1 DAG 定义（v4 修正版 — 基于 pipeline.md §3.1 I/O 契约）

```
                    ┌─ (audio) ──→ pitch（独立，不依赖任何层）
                    │
sentence ──→ grapheme ──→ phoneme ──→ ph_num
                    │
                    └── (audio) ──→ midi（基本路径，直读音频）
```

| 层 category | 依赖的上游层 | 产生步骤 | 步骤类型 |
|-------------|------------|---------|---------|
| `sentence` | — | Step 2 (ASR) | 自动 |
| `grapheme` | sentence（可选，不传播 dirty） | Step 3 (MinLabel) | **人工** |
| `phoneme` | grapheme | Step 4 (Alignment) | 自动 |
| `ph_num` | phoneme, grapheme | Step 6 (AddPhNum) | 自动 |
| `midi` | (audio only) | Step 7 (MIDI) | 自动 |
| `pitch` | (audio only) | Step 8 (Pitch) | 自动 |

> **关键修正（v4）**：midi 基本路径直读音频，不依赖 phoneme/ph_num。仅在 align 模式将 phoneme duration 作为 segmenter 可选引导（非严格依赖）。pitch 独立，永远不因任何层修改而标脏。

### 3.2 失效传播规则

当用户修改某层数据时，利用 BFS/DFS 从被修改层出发，沿 DAG 下游遍历，所有可达层标记 dirty：

| 用户修改的层 | 自动标记 dirty 的层 | 影响步骤 |
|-------------|-------------------|---------|
| `sentence` | （不传播，仅页面提示） | 无 |
| `grapheme` | `phoneme`, `ph_num` | Step 4, 6, (5, 9 人工) |
| `phoneme` | `ph_num` | Step 6, (9 人工) |
| `ph_num` | （无下游） | 仅自身 |
| `midi` | （无下游） | (Step 9 人工) |
| `pitch` | （无下游） | 仅自身 |

> **pitch 不依赖任何层**，仅依赖音频。修改音素边界不会使 F0 失效。
> **midi 基本路径不依赖任何层**，直读音频。仅在 align 模式标记 phoneme dirty 为"建议重算"（非强制）。

### 3.3 传播算法

```
function propagateDirty(modifiedLayer: string) -> set of dirty layers:
    result = {}
    queue = [modifiedLayer]
    while queue not empty:
        current = queue.pop()
        for each downstream of current in DAG:
            if downstream not in result:
                result.add(downstream)
                queue.push(downstream)
    return result
```

集中实现在 `PipelineContext` 或独立 `DirtyTracker` 类中。

---

## 4 步骤级标脏机制

### 4.1 概念

步骤级标脏回答的问题是：**"哪些步骤的产出数据因上游修改而不再可信？"**

推导逻辑：
- 从层依赖 DAG 出发，确定哪些层是 dirty
- 每个步骤有一个或多个 output layer（见 [pipeline.md §3.1](pipeline.md#L108-L122)）
- 如果某步骤的任一 output layer 是 dirty，则该步骤是 dirty
- **传递性**：如果步骤 N 是 dirty，步骤 N+1..10 同样是 dirty

### 4.2 步骤 → 输出层映射

| Step | 名称 | 输出层 | 类型 |
|------|------|--------|------|
| 1 | AudioSlicer | `slices` | 自动+人工 |
| 2 | ASR | `sentence`, `transcription` | 自动 |
| 3 | MinLabel | `grapheme` | 人工 |
| 4 | Alignment | `phoneme` | 自动 |
| 5 | PhonemeLabeler | —（修正 phoneme） | 人工 |
| 6 | AddPhNum | `ph_num` | 自动 |
| 7 | MidiTranscription | `midi` | 自动 |
| 8 | PitchExtraction | `pitch` | 自动 |
| 9 | PitchLabeler | —（修正 pitch/midi） | 人工 |
| 10 | Export | (files) | 自动 |

### 4.3 步骤 dirty 判定

```
function computeDirtySteps(context):
    dirtySteps = []
    dirtyLayers = context.dirty  // from layer propagation
    
    for each step in pipeline (in order):
        outLayers = step.outputLayers
        if any layer in outLayers is in dirtyLayers:
            dirtySteps.add(step)
    
    return dirtySteps
```

简化规则：**步骤 dirty 基于实际 I/O 契约推导，非简单线性传递。Step 6 (AddPhNum) dirty → 仅 Step 6 dirty，不牵连 Step 7/8（两者直读音频）。**

### 4.4 与 stepHistory 的关系

`PipelineContext.stepHistory` 记录每个步骤的执行历史（`StepRecord` 结构体，含 `stepName`、`processorId`、`startTime`、`endTime`、`success`、`errorMessage`、`usedConfig`）。步骤级标脏的另一种推导方式：

- 如果 stepHistory 中步骤 N 有记录但步骤 N-1 之后被重跑（通过时间戳判断），则步骤 N→10 都是 dirty
- 但这种方式不够精确（无法区分"用户手动修改数据"和"自动步骤重跑"）

**决定**：以层 dirty 为主推导步骤 dirty（更精确），stepHistory 作为辅助展示"最后执行时间"。

---

## 5 触发时机

### 5.1 用户手动保存 → 标记 dirty

**触发点**：所有 EditorPage 的 `saveCurrentSlice()` 方法。

**流程**：
```
1. 用户编辑数据并保存
2. 对比保存前后的层数据，确定哪些层被修改
3. 对每个被修改的层，调用 propagateDirty() 计算下游 dirty 层
4. 更新 PipelineContext.dirty
5. 可选：更新 PipelineContext.editedSteps（记录哪个步骤被手动编辑）
6. 持久化 PipelineContext JSON
7. 触发 UI 刷新（dirty 指示器更新）
```

**实现**：在 `EditorPageBase` 基类提供 `markDirtyAfterSave(modifiedLayers)` 方法，子类 `saveCurrentSlice()` 返回前调用。

### 5.2 自动步骤执行 → 清除 dirty

**触发点**：`PipelineRunner::run()` 执行自动步骤后，或用户手动触发推理后。

**流程**：
```
1. 自动步骤执行成功
2. 该步骤的 output layers 从 dirty 列表中移除
3. 但如果上游层本身仍是 dirty，下游仍保留 dirty
4. 步骤记录追加到 stepHistory
5. 持久化 PipelineContext JSON
```

### 5.3 页面切换 → 检查 dirty + 自动重算

**触发点**：`EditorPageBase::onActivated()` → `onAutoInfer()`。

**流程**：
```
1. 用户切换到某页面（如 PhonemeLabeler）
2. onActivated() → onAutoInfer()
3. 检查当前切片哪些层是 dirty
4. 如果该页面依赖的自动步骤的输出层是 dirty：
   a. 自动步骤 → 自动重算，Toast 通知
   b. 人工步骤 → 不清除 dirty，在 UI 中展示"需要重新标注"提示
5. 如果该页面依赖的人工步骤的上游层是 dirty：
   → 不清除 dirty，展示"上游数据已修改，建议重新标注"提示
```

### 5.4 导出 → 自动重算所有 dirty

**触发点**：`ExportPage` 导出前。

**流程**：
```
1. 遍历所有 Active 切片
2. 对每个切片，检查 dirty 层
3. 自动步骤的 output 层 dirty → 自动重算
4. 人工步骤的 output 层 dirty → 跳过，导出时使用旧数据 + 警告标记
5. 所有 dirty 重算后，执行导出
```

---

## 6 UI 表现

### 6.1 SliceListPanel 切片列表

| 状态 | 视觉效果 | 含义 |
|------|---------|------|
| 正常 | 无标记 | 所有层最新 |
| 层 dirty | 橙色圆点 + 层名列表 tooltip | 某些层因上游修改而过期 |
| 步骤 dirty | 黄色警告图标 + "N 步需重标" tooltip | 多个下游步骤需要重新标注 |

已有 `SliceListPanel::setSliceDirty()` 方法，扩展支持传入 dirty 详情。

### 6.2 编辑器页面内提示

**人工步骤页面**（MinLabel / PhonemeLabeler / PitchLabeler）：

- 页面顶部 banner：`"⚠ 上游数据已修改，建议重新标注 — Step 4(Alignment), Step 6(AddPhNum), Step 7(MIDI) 已过期"`
- 自动推理按钮旁显示橙色警告图标
- 进入页面时 Toast 通知：`"切片 #001 的上游数据已修改（grapheme 层），phoneme/ph_num/midi 层已过期"`，3 秒后自动消失

**自动步骤触发**：切换页面时自动重算 → Toast 通知 `"已自动重新计算 ph_num 和 MIDI（音素边界已修改）"`

### 6.3 状态栏

在 EditorPageBase 的 StatusBarBuilder 中增加 dirty 状态标签：

```
"切片 #001 | 3 层过期: phoneme, ph_num, midi | [重新计算]"
```

### 6.4 导出页

已有 dirty 计数，增强为：列出每个 dirty 切片的 detail（哪些层 dirty、哪些步骤需要关注），提供"导出前自动重算"按钮。

---

## 7 数据流总览

```
用户修改 grapheme (MinLabel 保存)
    │
    ▼
saveCurrentSlice() 返回前
    │
    ├── 1. 对比旧数据，确定 grapheme 被修改
    ├── 2. propagateDirty("grapheme") → [phoneme, ph_num]
    ├── 3. context.dirty = ["phoneme", "ph_num"]
    ├── 4. 持久化 dstemp/contexts/{sliceId}.json
    └── 5. emit 信号 → UI 刷新 dirty 指示器

用户切换到 PhonemeLabeler 页
    │
    ▼
onActivated() → onAutoInfer()
    │
    ├── 1. source()->dirtyLayers(sliceId) → ["phoneme", "ph_num"]
    ├── 2. phoneme 是 dirty → 是自动步骤(Alignment)的输出
    │      → 自动重新执行 FA
    │      → Toast: "已重新执行强制对齐（歌词发音已修改）"
    │      → context.dirty 移除 "phoneme"
    │      → 但 ph_num 仍 dirty
    ├── 3. 用户手动修正音素，保存
    ├── 4. saveCurrentSlice() → phoneme 被修改
    │      → propagateDirty("phoneme") → [ph_num]
    │      → context.dirty = ["ph_num"]
    └── 5. 页面顶部 banner: "⚠ ph_num 层已过期"

用户切换到 PitchLabeler 页
    │
    ▼
onActivated() → onAutoInfer()
    │
    ├── 1. ph_num 是 dirty → 自动步骤(AddPhNum)
    │      → 自动重算 add_ph_num（瞬时，纯算法）
    │      → Toast: "已自动更新发音数（音素边界已修改）"
    │      → context.dirty 移除 "ph_num"
    ├── 2. midi 不依赖任何层，不因 upstream 修改而标脏
    └── 3. 用户继续编辑 pitch
```

---

## 8 设计决策 (ADR)

| ADR | 决策 | 理由 |
|-----|------|------|
| ADR-43 | 以层 dirty 为主、步骤 dirty 为推导 | 层 dirty 是精确的数据依赖关系，步骤 dirty 是 UI 层面的聚合视图 |
| ADR-44 | 传播算法集中在 DirtyTracker | ARCH-01（职责单一），避免各页面分散实现 |
| ADR-45 | 保存时对比新旧数据确定修改层 | 避免过度 dirty（用户可能打开页面不做任何修改就保存） |
| ADR-46 | 自动步骤 dirty → 自动重算；人工步骤 dirty → 仅提示 | 自动步骤重跑没有副作用；人工步骤需要用户确认 |
| ADR-47 | dirty 标记 per-slice，不跨切片传播 | 修改切片 #001 的 grapheme 不影响切片 #002 |
| ADR-48 | 步骤 dirty 基于 I/O 契约推导，非简单线性传递（v4 修正） | Step 7/8 直读音频，不受 Step 6 ph_num dirty 牵连 |

---

## 9 实施状态

| 功能 | 状态 | 位置 |
|------|------|------|
| `propagateDirty()` BFS 传播算法 | ✅ 已实现 | `PipelineContext.cpp` |
| `addDirtyLayer()` / `removeDirtyLayer()` | ✅ 已实现 | `PipelineContext.h/cpp` |
| `ProjectDataSource::addDirtyLayers()` 调用 propagate | ✅ 已实现 | `ProjectDataSource.cpp` |
| `EditorPageBase::markLayersModified()` | ✅ 已实现 | `EditorPageBase.cpp` |
| `saveCurrentSlice()` 调用 markLayersModified | ✅ 已实现 | MinLabel/Pitch/Phoneme 三页面 |
| 页面切换自动重算 (onAutoInfer) | ✅ 已实现 | 各 EditorPage |
| `manuallyEdited` 检查 (onAutoInfer 跳过手动编辑层) | ✅ 已实现 | PitchLabelerPage L414/L431, PhonemeLabelerPage L422 |
| 步骤级标脏推导 (dirtySteps) | ✅ 已实现 | `PipelineContext::deriveDirtySteps()` |
| SliceListPanel 增强 dirty tooltip | ✅ 已实现 | `SliceListPanel::setSliceDirtyLayers()` |
| 页面 banner 组件 | ✅ 已实现 | `EditorPageBase::showBanner()` + `PageBanner` widget |
| PipelineRunner dirty 集成 | ✅ 通过 ProjectDataSource 层间传播 | PipelineRunner 本身不直接感知 dirty，由上层 (EditorPageBase/ProjectDataSource) 在调用前完成脏层传播 |

> **所有功能已全部实现**。PipelineRunner 保持职责单一（执行 TaskInput/TaskOutput），dirty 检查与传播由上层组件负责。

---

## 关联文档

- [pipeline.md](pipeline.md) — 流水线 I/O 契约（步骤→层映射）
- [ds-format.md](ds-format.md) — PipelineContext JSON 格式、层依赖 DAG（§6）
- [unified-app-design.md](unified-app-design.md) — 页面架构与自动补全流程
- [overview.md](../overview.md) — 项目架构
- [human-decisions.md](../human-decisions.md) — 设计准则（ARCH/CONCUR/ROBUST/INFRA）

> 创建时间：2026-05-20