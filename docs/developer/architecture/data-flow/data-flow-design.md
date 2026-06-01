# 数据流设计

> 本文档合并自 `pipeline.md`、`dirty-mechanism.md` 和 `ds-format.md §6`，统一描述流水线定义、层依赖关系、脏数据传播机制。
>
> 版本 1.0 — 2026-06-01

---

## 1 流水线定义

### 1.1 完整流水线

```
原始音频 ─── Step 1: AudioSlicer 自动切片
          │
          ▼
     切片音频 ────── Step 2: FunAsr ASR 歌词识别（可选。输出 transcription/text，非 sentence）
          │
          ▼
     识别歌词 ────── Step 3: MinLabel 歌词核对 + G2P（人工，无 ITaskProcessor 封装）
          │
          ▼
     发音序列 ────── Step 4: Alignment 音素对齐（模型可替换，当前仅 hubert-fa）
          │
          ▼
     音素时间 ────── Step 5: PhonemeLabeler 音素修正（可选，人工，无 ITaskProcessor 封装）
          │
          ▼
     音素时间 ────── Step 6: AddPhNum 计算 ph_num（纯算法，仅需 phoneme 输入）
          │
          ▼
     [音频直读] ──── Step 7: MIDI 提取（模型可替换，当前仅 game。直接从音频提取）
          │
          ▼
     [音频直读] ──── Step 8: 音高提取（模型可替换，当前仅 rmvpe。输出扁平 float 数组）
          │
          ▼
    F0+MIDI ──────── Step 9: PitchLabeler 音高修正（人工，无 ITaskProcessor 封装）
          │
          ▼
                      Step 10: 导出（通过 FormatAdapter，非单个处理器）

  ※ 任意步骤可标记切片为"丢弃"，后续步骤自动跳过
  ※ Step 5 和 Step 9 可跳过，导出时自动补全
  ※ Step 6 在 PitchLabeler 或导出时自动执行
  ※ Step 0（音频预处理）接口 IAudioPreprocessor 存在但尚无实现
  ※ Steps 3/5/9 为纯 UI 页面，无对应的 ITaskProcessor 注册
```

### 1.2 v2 遗留问题与解决

| 问题 | 解决方案 |
|------|---------|
| CSV 是数据主干 | PipelineContext 层字典替代 CSV 作为中间数据载体 |
| processBatch 语义不一致 | 移除 processBatch，批量迭代归 PipelineRunner |
| 步骤间数据路由僵硬 | PipelineContext 层字典支持任意输入/输出组合 |
| 缺少预处理环节 | 独立的 IAudioPreprocessor 接口 |
| 切片命名不可控 | ADR-34：{prefix}_{NNN}.wav 按时间顺序 |

### 1.3 新增需求覆盖

| 需求 | 解决方案 |
|------|---------|
| MakeDiffSinger 兼容 | 导入/导出通过 FormatAdapter，非流水线核心 |
| 切片丢弃 | PipelineContext.status 字段 + 传播 |
| 撤销重做 | 自动步骤→快照，人工步骤→QUndoStack |
| 崩溃日志 | Logger + CrashHandler 自动记录 |

---

## 2 I/O 契约

### 2.1 步骤 ↔ 层映射

| Step | 名称 | 类型 | taskName | 输入层 | 输出层 | 注册 ID |
|------|------|------|----------|--------|--------|---------|
| 0 | AudioPreprocess | 自动 | — | — | — | — |
| 1 | AudioSlicer | 自动+人工 | `audio_slice` | — | `slices` | `slicer` |
| 2 | ASR (FunAsr) | 自动 | `asr` | — | `transcription` | `funasr` |
| 3 | MinLabel | 人工 | — | — | `grapheme` | — |
| 4 | Alignment | 自动 | `phoneme_alignment` | `grapheme` | `phoneme` | `hubert-fa` |
| 5 | PhonemeLabeler | 人工 | — | — | — | — |
| 6 | AddPhNum | 自动 | `add_ph_num` | `phoneme` | `ph_num` | `add-ph-num` |
| 7 | MidiTranscription | 自动 | `midi_transcription` | —（直读音频） | `midi` | `game` |
| 8 | PitchExtraction | 自动 | `pitch_extraction` | —（直读音频） | `pitch` | `rmvpe` |
| 9 | PitchLabeler | 人工 | — | — | — | — |
| 10 | Export | 自动 | FormatAdapter | (all layers) | (files) | — |

### 2.2 层 category 格式

| category | JSON 形状 | 说明 |
|----------|----------|------|
| `sentence` | `[{text, pos}]` | 整句歌词。`pos` = 微秒 |
| `grapheme` | `[{text, pos}]` | 音节序列。`pos` = 微秒 |
| `phoneme` | `[{phone, start, end}]` | 音素时间序列。`start`/`end` = 微秒 |
| `ph_num` | `{"values": [int]}` | 每词音素个数 |
| `midi` | `[{pitch, onset, duration, voiced}]` | MIDI 音符序列。`onset`/`duration` = 微秒 |
| `pitch` | `{f0: [int], timestep: int}` | F0：毫赫兹，timestep：微秒 |
| `slices` | `[{id, in, out, status}]` | 切片边界。`in`/`out` = 微秒 |
| `transcription` | `[{text, pos}]` | ASR 识别结果。`pos` = 微秒 |

### 2.3 模型可替换性

同一 taskName 下所有处理器必须声明相同的 inputs/outputs category：

```
phoneme_alignment:    hubert-fa（唯一）     → in: [grapheme], out: [phoneme]
midi_transcription:   game（唯一）          → in: [], out: [midi]（直读音频）
pitch_extraction:     rmvpe（唯一）         → in: [], out: [pitch]
asr:                  funasr（唯一）         → in: [], out: [transcription]
add_ph_num:           add-ph-num（唯一）     → in: [phoneme], out: [ph_num]
audio_slice:          slicer（唯一）         → in: [], out: [slices]
```

> 以上每个 taskName 当前仅有一个处理器实现。`sofa`、`mfa`、`fcpe`、`crepe`、`whisper`、`some` 等备选为未来计划，尚未注册。

`TaskProcessorRegistry::registerProcessor()` 当前**不执行 I/O 一致性校验**：注册时直接插入 map，无 inputs/outputs 验证。一致性校验为计划功能。

---

## 3 层依赖 DAG

### 3.1 DAG 定义

基于 I/O 契约推导的层依赖关系：

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

### 3.2 步骤 → 输出层映射

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

---

## 4 脏数据失效传播

### 4.1 两种标脏维度

| 维度 | 粒度 | 含义 | 示例 |
|------|------|------|------|
| **层级标脏** (Layer Dirty) | 层 / category | 某层数据因上游修改而过期，重算后可恢复 | grapheme 修改 → phoneme/ph_num/midi dirty |
| **步骤级标脏** (Step Dirty) | Pipeline 步骤 | 某步骤因上游修改需要重新执行/审核 | 在 MinLabel 修改 → Step 4~10 全部需要重新标注 |

两者的关系：**层级标脏是步骤级标脏的数据基础**。步骤级标脏通过分析 `stepHistory` + `dirty` 推导得出。

### 4.2 失效传播规则

当用户修改某层数据时，利用 BFS 从被修改层出发，沿 DAG 下游遍历，所有可达层标记 dirty：

| 用户修改的层 | 自动标记 dirty 的层 | 影响步骤 |
|-------------|-------------------|---------|
| `sentence` | （不传播，仅页面提示） | 无 |
| `grapheme` | `phoneme`, `ph_num` | Step 4, 6, (5, 9 人工) |
| `phoneme` | `ph_num` | Step 6, (9 人工) |
| `ph_num` | （无下游） | 仅自身 |
| `midi` | （无下游） | (Step 9 人工) |
| `pitch` | （无下游） | 仅自身 |

### 4.3 传播算法

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

实现在 `PipelineContext::propagateDirty()` 中（BFS 遍历 DAG）。

### 4.4 步骤 dirty 判定

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

### 4.5 Context JSON 中的 dirty 字段

PipelineContext 中的 `dirty` 字段记录哪些层因上游修改而过期：

```json
{
    "dirty": ["ph_num", "midi"]
}
```

**dirty 标记是 per-slice 的**。修改某个切片的音素不影响其他切片。dirty 列表中的层数据**仍然保留**（是旧版本），但标记为不可信。重算后从 dirty 列表中移除。

完整 Context JSON 示例：

```json
{
    "itemId": "a54c548a",
    "audioPath": "dstemp/slices/guangnian_001.wav",
    "status": "active",
    "discardReason": "",
    "discardedAtStep": "",
    "globalConfig": {},
    "dirty": ["ph_num", "midi"],
    "manuallyEdited": [],
    "stepHistory": [
        {
            "stepName": "phoneme_alignment",
            "processorId": "hubert-fa",
            "startTime": "2026-05-02T10:30:05",
            "endTime": "2026-05-02T10:30:08",
            "success": true,
            "errorMessage": "",
            "usedConfig": {}
        }
    ],
    "editedSteps": ["phoneme_review"],
    "layers": {
        "grapheme": [...],
        "phoneme": [...],
        "ph_num": {"values": [...]},
        "midi": [...],
        "pitch": [...]
    }
}
```

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
5. 可选：更新 PipelineContext.editedSteps
6. 持久化 PipelineContext JSON
7. 触发 UI 刷新（dirty 指示器更新）
```

**实现**：在 `EditorPageBase` 基类提供 `markDirtyAfterSave(modifiedLayers)` 方法。

### 5.2 自动步骤执行 → 清除 dirty

**触发点**：`PipelineRunner::run()` 执行自动步骤后。

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
   → 不清除 dirty，展示"上游数据已修改"提示
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

### 5.5 完整数据流示例

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
```

---

## 6 切片丢弃机制

在 `PipelineContext` 中引入 `status` 字段：`Active` / `Discarded` / `Error`。

**行为规则**：
1. 一旦标记 `Discarded`，PipelineRunner 在后续所有步骤跳过该 item
2. 用户可在任意步骤 UI 中将 `Discarded` 改回 `Active`
3. 导出时默认排除 `Discarded` 切片（可选包含）
4. status/reason 随 context JSON 持久化到 `dstemp/contexts/{itemId}.json`
5. 丢弃的切片在列表中灰显 + 删除线

**自动丢弃检测**：PipelineRunner 在每步完成后可选执行验证回调（`ValidationCallback`）。内置验证器（`SliceLengthValidator`、`AlignmentQualityValidator`、`PitchCoverageValidator`）为计划功能，尚未实现。

---

## 7 撤销重做设计

**原则**：
- 自动步骤（模型推理）：**不需要撤销重做**，重跑即可恢复
- 人工步骤（MinLabel/PhonemeLabeler/PitchLabeler）：**必须撤销重做**，已有 QUndoStack 基础设施
- 切片丢弃/恢复操作：**需要撤销重做**

| Step | 是否需要 | 机制 |
|------|---------|------|
| 0 预处理 | 否 | 重跑 |
| 1 切片 | 部分 | QUndoStack（手动切点）；自动切片参数变更 → 全部重生成 |
| 2 LyricFA | 否 | 重跑 |
| 3 MinLabel | 是 | QUndoStack |
| 4 对齐 | 否 | 重跑 |
| 5 PhonemeLabeler | 是 | QUndoStack |
| 6 AddPhNum | 否 | 重跑 |
| 7 MIDI | 否 | 重跑 |
| 8 音高 | 否 | 重跑 |
| 9 PitchLabeler | 是 | QUndoStack |
| 丢弃/恢复 | 是 | DiscardSliceCommand |

**自动步骤快照**（计划功能，尚未实现）：自动步骤执行前序列化 context → `dstemp/snapshots/{itemId}_step{N}.json`，"撤销"自动步骤 = 恢复快照 + 清除该步骤产出的层。

---

## 8 UI 表现

### 8.1 SliceListPanel 切片列表

| 状态 | 视觉效果 | 含义 |
|------|---------|------|
| 正常 | 无标记 | 所有层最新 |
| 层 dirty | 橙色圆点 + 层名列表 tooltip | 某些层因上游修改而过期 |
| 步骤 dirty | 黄色警告图标 + "N 步需重标" tooltip | 多个下游步骤需要重新标注 |

已有 `SliceListPanel::setSliceDirtyLayers()` 方法。

### 8.2 编辑器页面内提示

**人工步骤页面**（MinLabel / PhonemeLabeler / PitchLabeler）：
- 页面顶部 banner：`"⚠ 上游数据已修改，建议重新标注 — Step 4(Alignment), Step 6(AddPhNum), Step 7(MIDI) 已过期"`
- 自动推理按钮旁显示橙色警告图标
- 进入页面时 Toast 通知，3 秒后自动消失

### 8.3 状态栏

在 EditorPageBase 的 StatusBarBuilder 中增加 dirty 状态标签：
```
"切片 #001 | 3 层过期: phoneme, ph_num, midi | [重新计算]"
```

---

## 9 PipelineContext 与 .dstext 的关系

| 维度 | .dstext | PipelineContext |
|------|---------|-----------------|
| 用途 | 持久标注数据，纳入版本控制 | 运行时中间数据，不纳入版本控制 |
| 层格式 | 统一 Boundary 格式 `{id, pos, text}` | 按 category 的紧凑格式 |
| 生命周期 | 用户保存时写入 | 每步完成后自动写入 |

加载 .dstext 到 PipelineContext 时：
```
phoneme 层 boundaries → [{phone: b[i].text, start: b[i].pos, end: b[i+1].pos}]
grapheme 层 boundaries → [{text: b[i].text, pos: b[i].pos}]
midi 层 boundaries → [{pitch: parseNote(b[i].text), onset: b[i].pos, duration: b[i+1].pos - b[i].pos, voiced: true}]
```

---

## 10 设计决策 (ADR)

| ADR | 决策 | 理由 |
|-----|------|------|
| ADR-30 | 层数据保持 nlohmann::json | 离线处理，JSON 开销可忽略 |
| ADR-31 | PipelineContext 用 category 做扁平键 | 与 .dsproj tasks 的 category 绑定一致 |
| ADR-32 | 移除 processBatch | 批量迭代、丢弃跳过、错误记录归 Runner |
| ADR-33 | TranscriptionRow 降级为适配器内部 | 最小迁移爆炸半径 |
| ADR-34 | 切片命名 {prefix}_{NNN}.wav | 按时间顺序，兼容 MDS |
| ADR-35 | LyricFA 以整首歌为粒度 | ASR 需要完整上下文 |
| ADR-36 | 同 taskName 处理器 I/O 必须一致 | 模型可替换的前提 |
| ADR-37 | Context JSON 替代 .dsitem + BatchCheckpoint | 统一一个文件 |
| ADR-38 | 音频预处理独立接口 | 操作音频文件，不产出层数据 |
| ADR-39 | 切片丢弃通过 status 字段 + 传播 | 简单可靠，可撤销 |
| ADR-40 | 自动步骤用快照替代细粒度撤销 | 自动步骤没有中间态 |
| ADR-41 | 导入/导出格式声明在 task 定义中 | 运行时 PipelineRunner 据此查找适配器 |
| ADR-42 | MDS 兼容通过格式适配器实现 | 不在核心流水线中引入 MDS 目录约定 |
| ADR-43 | 以层 dirty 为主、步骤 dirty 为推导 | 层 dirty 是精确的数据依赖关系 |
| ADR-44 | 传播算法集中在 DirtyTracker | ARCH-01（职责单一），避免各页面分散实现 |
| ADR-45 | 保存时对比新旧数据确定修改层 | 避免过度 dirty |
| ADR-46 | 自动步骤 dirty → 自动重算；人工步骤 dirty → 仅提示 | 自动步骤重跑没有副作用 |
| ADR-47 | dirty 标记 per-slice，不跨切片传播 | 修改切片 #001 不影响切片 #002 |
| ADR-48 | 步骤 dirty 基于 I/O 契约推导，非简单线性传递（v4 修正） | Step 7/8 直读音频，不受 Step 6 dirty 牵连 |

---

## 11 实施状态

| 功能 | 状态 | 位置 |
|------|------|------|
| `propagateDirty()` BFS 传播算法 | ✅ 已实现 | `PipelineContext.cpp` |
| `addDirtyLayer()` / `removeDirtyLayer()` | ✅ 已实现 | `PipelineContext.h/cpp` |
| `ProjectDataSource::addDirtyLayers()` 调用 propagate | ✅ 已实现 | `ProjectDataSource.cpp` |
| `EditorPageBase::markLayersModified()` | ✅ 已实现 | `EditorPageBase.cpp` |
| `saveCurrentSlice()` 调用 markLayersModified | ✅ 已实现 | MinLabel/Pitch/Phoneme 三页面 |
| 页面切换自动重算 (onAutoInfer) | ✅ 已实现 | 各 EditorPage |
| `manuallyEdited` 检查 (onAutoInfer 跳过手动编辑层) | ✅ 已实现 | PitchLabelerPage / PhonemeLabelerPage |
| 步骤级标脏推导 (dirtySteps) | ✅ 已实现 | `PipelineContext::deriveDirtySteps()` |
| SliceListPanel 增强 dirty tooltip | ✅ 已实现 | `SliceListPanel::setSliceDirtyLayers()` |
| 页面 banner 组件 | ✅ 已实现 | `EditorPageBase::showBanner()` + `PageBanner` widget |
| PipelineRunner dirty 集成 | ✅ 已实现 | 通过 ProjectDataSource 层间传播 |
| TaskProcessorRegistry I/O 一致性校验 | ⏳ 计划 | 当前注册时无校验 |
| 自动步骤快照 | ⏳ 计划 | `dstemp/snapshots/` |
| 自动丢弃检测验证器 | ⏳ 计划 | SliceLengthValidator / AlignmentQualityValidator / PitchCoverageValidator |

---

## 关联文档

- [human-decisions.md](../../human-decisions.md) — 设计准则与决策
- [ds-format.md](ds-format.md) — .dsproj / .dstext 文件格式规范
- [overview.md](../overview.md) — 项目架构总览
- [unified-app-design.md](../framework/unified-app-design.md) — 应用页面设计
- [component-design.md](component-design.md) — 核心组件设计
