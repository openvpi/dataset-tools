# 任务处理器架构设计

> 层路由流水线 + MakeDiffSinger 兼容 + 切片丢弃 + 撤销重做

---

## 1 问题陈述

### 1.1 v2 遗留问题
1. **CSV 是数据主干**：TranscriptionRow 贯穿 Step 6→7→8，字段逐步累积
2. **processBatch 语义不一致**：4 个处理器中 2 个覆盖了 processBatch
3. **步骤间数据路由僵硬**：每步只能消费上一步的直接输出
4. **缺少预处理环节**：降噪、响度匹配无处安放
5. **切片命名不可控**

### 1.2 新增需求
6. **MakeDiffSinger 兼容**：每步支持导入/导出 MDS 文件格式
7. **切片丢弃**：任意步骤可标记切片为"丢弃"
8. **撤销重做**：在合理复杂度下支持撤销重做
9. **崩溃日志**：所有步骤的崩溃/错误自动记录

### 1.3 设计目标
- 每个处理器有固定的 m 层输入 → n 层输出，模型可替换但 I/O 契约不变
- 步骤间通过 PipelineContext（层字典）路由
- CSV/TextGrid/DS/.lab 退化为导入/导出适配器

---

## 2 MakeDiffSinger 兼容性分析

### 2.1 文件格式汇总

| 格式 | 用途 |
|------|------|
| `.lab` | 标注文件（音节级歌词），单行空格分隔音节 |
| `transcriptions.csv` | 数据集核心文件，UTF-8 + BOM 可选 |
| TextGrid | MFA 强制对齐输出，2-tier (words+phones) / 3-tier (sentences+words+phones) |
| `.ds` | DiffSinger 训练文件，JSON 数组 |
| 词典 `.txt` | G2P 词典，`音节\t音素1 [音素2]` |

### 2.2 transcriptions.csv 列定义

| 列名 | 类型 | 产生步骤 |
|------|------|---------|
| `name` | string | build_dataset |
| `ph_seq` | space-sep string | build_dataset |
| `ph_dur` | space-sep float | build_dataset |
| `ph_num` | space-sep int | add_ph_num |
| `note_seq` | space-sep string | estimate_midi |
| `note_dur` | space-sep float | estimate_midi |
| `note_slur` | space-sep int (0/1) | convert_ds |
| `note_glide` | space-sep string | 手动/工具 |

### 2.3 兼容策略

| 步骤 | 导入格式 | 导出格式 |
|------|---------|---------|
| Step 1 切片 | 3-tier TextGrid (sentences tier) | wavs/*.wav |
| Step 3 歌词标注 | `.lab` 文件 | `.lab` 文件 |
| Step 4 音素对齐 | 2-tier TextGrid | 2-tier TextGrid |
| Step 5 音素修正 | 2-tier TextGrid | 2-tier TextGrid |
| Step 6 AddPhNum | CSV (ph_seq) | CSV (+ ph_num) |
| Step 7 MIDI | CSV (ph_seq+ph_dur+ph_num) | CSV (+ note_seq+note_dur) |
| Step 10 导出 | — | transcriptions.csv, .ds, 3-tier TextGrid |

---

## 3 完整流水线定义

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
     [音频直读] ──── Step 7: MIDI 提取（模型可替换，当前仅 game。直接从音频提取，不消费 phoneme/ph_num 层）
          │
          ▼
     [音频直读] ──── Step 8: 音高提取（模型可替换，当前仅 rmvpe。输出扁平 float 数组，非 {f0, timestep}）
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
  ※ 详见 unified-app-design.md
```

### 3.1 I/O 契约（实际代码）

| Step | 名称 | 类型 | taskName | 输入层 (name) | 输出层 (name) | 注册 ID |
|------|------|------|----------|----------------|-----------------|---------|
| 0 | AudioPreprocess | 自动 | — | — | — | — |
| 1 | AudioSlicer | 自动+人工 | `audio_slice` | — | `slices` → slices | `slicer` |
| 2 | ASR (FunAsr) | 自动 | `asr` | — | `transcription` → transcription | `funasr` |
| 3 | MinLabel | 人工 | — | — | `grapheme` | — |
| 4 | Alignment | 自动 | `phoneme_alignment` | `grapheme` → grapheme | `phoneme` → phoneme | `hubert-fa` |
| 5 | PhonemeLabeler | 人工 | — | — | — | — |
| 6 | AddPhNum | 自动 | `add_ph_num` | `phoneme` → phoneme | `ph_num` → ph_num | `add-ph-num` |
| 7 | MidiTranscription | 自动 | `midi_transcription` | —（直读音频） | `midi` → midi | `game` |
| 8 | PitchExtraction | 自动 | `pitch_extraction` | —（直读音频） | `pitch` → pitch | `rmvpe` |
| 9 | PitchLabeler | 人工 | — | — | — | — |
| 10 | Export | 自动 | FormatAdapter | (all layers) | (files) | — |

### 3.2 层 category 定义（实际代码）

| category | JSON 形状 | 说明 |
|----------|----------|------|
| `sentence` | `[{text, pos}]` | 整句歌词。`pos` = 微秒 (`int64`) |
| `grapheme` | `[{text, pos}]` | 音节序列（如拼音）。`pos` = 微秒 |
| `phoneme` | `[{phone, start, end}]` | 音素时间序列。`start`/`end` = 微秒 |
| `ph_num` | `{"values": [int]}` | 每词音素个数 |
| `midi` | `[{pitch, onset, duration, voiced}]` | MIDI 音符序列（GAME 处理器输出）。`onset`/`duration` = 微秒，`pitch` = MIDI 音高编号，`voiced` = bool |
| `pitch` | `{f0: [int], timestep: int}` | F0：毫赫兹，timestep：微秒 |
| `slices` | `[{id, in, out, status}]` | 切片边界。`in`/`out` = 微秒（`int64`），`status` = `"active"`/`"discarded"`/`"error"` |
| `transcription` | `[{text, pos}]` | ASR 识别结果。`pos` = 微秒 |

---

## 4 切片丢弃机制

在 `PipelineContext` 中引入 `status` 字段：`Active` / `Discarded` / `Error`。

**行为规则**：
1. 一旦标记 `Discarded`，PipelineRunner 在后续所有步骤跳过该 item
2. 用户可在任意步骤 UI 中将 `Discarded` 改回 `Active`
3. 导出时默认排除 `Discarded` 切片（可选包含）
4. status/reason 随 context JSON 持久化到 `dstemp/contexts/{itemId}.json`
5. 丢弃的切片在列表中灰显 + 删除线

**自动丢弃检测**：PipelineRunner 在每步完成后可选执行验证回调（`ValidationCallback`）。内置验证器（`SliceLengthValidator`、`AlignmentQualityValidator`、`PitchCoverageValidator`）为计划功能，尚未实现。

---

## 5 撤销重做设计

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
| 丢弃/恢复 | 是 | DiscardSliceCommand（含 State::Discard / State::Restore）|

**自动步骤快照**（计划功能，尚未实现）：自动步骤执行前序列化 context → `dstemp/snapshots/{itemId}_step{N}.json`，"撤销"自动步骤 = 恢复快照 + 清除该步骤产出的层。

---

## 6 模型可替换性

同一 taskName 下所有处理器必须声明相同的 inputs/outputs category：

```
phoneme_alignment:    hubert-fa（仅此一个）    → in: [grapheme], out: [phoneme]
midi_transcription:   game（仅此一个）         → in: [], out: [midi]（直读音频）
pitch_extraction:     rmvpe（仅此一个）        → in: [], out: [pitch]（扁平数组）
asr:                  funasr（仅此一个）        → in: [], out: [transcription]
add_ph_num:           add-ph-num（仅此一个）    → in: [phoneme], out: [ph_num]
audio_slice:          slicer（仅此一个）        → in: [], out: [slices]
```

> **注意**：以上每个 taskName 当前仅有一个处理器实现。`sofa`、`mfa`、`fcpe`、`crepe`、`whisper`、`some` 等备选为未来计划，尚未注册。

`TaskProcessorRegistry::registerProcessor()` 当前**不执行 I/O 一致性校验**：注册时直接插入 map，无 inputs/outputs 验证。一致性校验为计划功能。

---

## 7 层依赖 DAG 与脏数据传播

基于 I/O 契约定义的层依赖关系：

```
grapheme → phoneme → ph_num
midi 直读音频（不依赖任何层）
pitch 直读音频（不依赖任何层）
sentence 不自动传播到 grapheme（grapheme 是人工步骤）
```

具体实现见 [dirty-mechanism.md](dirty-mechanism.md) 和 `PipelineContext::layerDag`。

---

## 8 设计决策记录 (ADR)

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

---

## 关联文档

- [ds-format.md](ds-format.md) — .dsproj / .dstext 格式规范
- [unified-app-design.md](unified-app-design.md) — LabelSuite + DsLabeler 设计
- [overview.md](../overview.md) — 项目架构
- [dirty-mechanism.md](dirty-mechanism.md) — 层依赖脏数据失效机制
- [human-decisions.md](../human-decisions.md) — 设计准则
