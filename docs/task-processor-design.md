# 任务处理器架构设计 v3

> 层路由流水线 + MakeDiffSinger 兼容 + 切片丢弃 + 撤销重做

---

## 1 问题陈述

### 1.1 v2 遗留问题

1. **CSV 是数据主干**：TranscriptionRow（CSV 行）贯穿 Step 6→7→8，字段逐步累积。
2. **processBatch 语义不一致**：4 个处理器中 2 个覆盖了 processBatch。
3. **步骤间数据路由僵硬**：每步只能消费上一步的直接输出。
4. **缺少预处理环节**：降噪、响度匹配无处安放。
5. **切片命名不可控**：缺乏按原始音频内顺序统一命名的机制。

### 1.2 新增需求

6. **MakeDiffSinger 兼容**：每步支持导入/导出 MakeDiffSinger 项目的文件格式（.lab、transcriptions.csv、TextGrid、.ds）。
7. **切片丢弃**：任意步骤可标记切片为"丢弃"，后续步骤自动跳过。
8. **撤销重做**：在合理复杂度下，每步支持撤销重做。
9. **崩溃日志**：所有步骤的崩溃/错误自动记录。

### 1.3 设计目标

- 每个处理器/页面有**固定的 m 层输入 → n 层输出**，模型可替换但 I/O 契约不变
- 步骤间通过 **PipelineContext（层字典）** 路由
- CSV/TextGrid/DS/.lab 等格式退化为**导入/导出适配器**
- 兼容 MakeDiffSinger 的目录结构和文件格式

---

## 2 MakeDiffSinger 兼容性分析

### 2.1 MakeDiffSinger 文件格式汇总

| 格式 | 用途 | 精确规格 |
|------|------|---------|
| `.lab` | 标注文件（音节级歌词） | 单行文本，空格分隔音节。如 `gan shou ting zai wo fa duan de zhi jian` |
| `transcriptions.csv` | 数据集核心文件 | CSV，UTF-8 + BOM 可选，RFC 4180。列名和累积顺序见下表。 |
| TextGrid | MFA 强制对齐输出 | Praat TextGrid 格式。2-tier 模式：`words`（词级）+ `phones`（音素级）。3-tier 模式：`sentences`（句级）+ `words` + `phones`。sentences tier 用于手动切片精修（combine_tg / slice_tg 工作流），mark 为空的 sentence 区间表示丢弃该段。 |
| `.ds` | DiffSinger 训练文件 | JSON 数组，每元素一个句子。键见下表。 |
| 词典 `.txt` | G2P 词典 | `音节\t音素1 [音素2]`，tab 分隔。1 音素 = 元音，2 音素 = 辅音+元音 |

### 2.2 transcriptions.csv 列定义（累积式）

列按流水线步骤逐步累积，不同阶段的 CSV 文件包含不同列子集：

| 列名 | 类型 | 产生步骤 | 说明 |
|------|------|---------|------|
| `name` | string | build_dataset | 文件名词干（无扩展名），对应 `wavs/{name}.wav` |
| `ph_seq` | space-sep string | build_dataset | 音素序列，含 `SP`/`AP` |
| `ph_dur` | space-sep float | build_dataset | 音素时长（秒），6位小数 |
| `ph_num` | space-sep int | add_ph_num | 每个词的音素个数 |
| `note_seq` | space-sep string | estimate_midi / SOME | 音符名（如 `C4`、`D#3`、`rest`） |
| `note_dur` | space-sep float | estimate_midi / SOME | 音符时长（秒） |
| `note_slur` | space-sep int (0/1) | convert_ds csv2ds | 连音标记（ds2csv 时**不**导出此列） |
| `note_glide` | space-sep string | 手动/工具 | 滑音类型（可选列，`none` 为无滑音） |

### 2.3 .ds 文件 JSON 结构

```json
[
    {
        "offset": 0.0,                        // 句子在音频中的起始偏移（秒）
        "text": "SP gan shou ting zai ...",    // 音素序列（= ph_seq）
        "ph_seq": "SP g an sh ou ...",         // 音素序列
        "ph_dur": "0.1 0.08 0.52 ...",         // 音素时长（空格分隔浮点数）
        "ph_num": "1 2 2 ...",                 // 词音素数
        "note_seq": "rest C4 D4 ...",          // 音符序列
        "note_dur": "0.1 0.6 0.6 ...",         // 音符时长
        "note_slur": "0 0 1 ...",              // 连音标记
        "note_glide": "none none up ...",       // 滑音类型（可选）
        "f0_seq": "0.0 261.6 293.7 ...",       // F0 序列（Hz，空格分隔，1位小数）
        "f0_timestep": "0.011609977..."         // F0 采样间隔（= hop_size / sample_rate）
    }
]
```

### 2.4 MakeDiffSinger 目录结构

```
dataset/
    raw/                                ← 用户原始数据（切片前）
        wavs/
            recording1.wav
        transcriptions.csv              ← 构建后的最终 CSV
    wavs/                               ← 切片后的音频 + .lab 标注
        segment_001.wav
        segment_001.lab
        segment_001.TextGrid            ← MFA 对齐结果
        segment_001.ds                  ← DS 训练文件
    transcriptions.csv                  ← 最终数据集 CSV
```

### 2.5 兼容策略

每个步骤支持**导入**和**导出**两个方向的格式适配：

| 步骤 | 导入格式 | 导出格式 | 说明 |
|------|---------|---------|------|
| Step 1 切片 | 3-tier TextGrid (sentences tier) | wavs/*.wav | 可导入 MDS combine_tg 输出的 3-tier TextGrid，按 sentences tier 切片（空 mark = 丢弃） |
| Step 3 歌词标注 | `.lab` 文件 | `.lab` 文件 | 空格分隔音节。可从 MDS 导入已有标注 |
| Step 4 音素对齐 | 2-tier TextGrid (words+phones) | 2-tier TextGrid (words+phones) | 兼容 MFA / HuBERT-FA 输出 |
| Step 5 音素修正 | 2-tier TextGrid | 2-tier TextGrid | 精修后可导出给 MDS 后续步骤 |
| Step 6 AddPhNum | CSV (ph_seq) | CSV (+ ph_num) | 兼容 add_ph_num.py |
| Step 7 MIDI | CSV (ph_seq+ph_dur+ph_num) | CSV (+ note_seq+note_dur) | 兼容 estimate_midi.py |
| Step 10 导出 | — | transcriptions.csv, .ds, 3-tier TextGrid | 完全兼容 MDS 目录结构。3-tier 导出支持精修工作流回传 |

---

## 3 完整流水线定义

### 3.0 全景图

```
原始音频 ─┬─ Step 0: 音频预处理（降噪/响度匹配，可选）
          │
          ▼
     预处理音频 ─── Step 1: AudioSlicer 自动切片
          │         ├─ 自动切片（RMS 静音检测）
          │         ├─ 参数不满意 → 调参重切（全部重生成）
          │         └─ 个别片段过长 → 波形图界面手动切片
          │
          ▼
     切片音频 ────── Step 2: LyricFA 歌词识别（可选）
     + 原歌词 txt     ├─ 以整首歌为单位 ASR
                      └─ 输出每句歌词，算法匹配原歌词最大可能项
          │
          ▼
     识别歌词 ────── Step 3: MinLabel 歌词核对 + G2P（人工）
          │           ├─ 人工核对/修正歌词
          │           ├─ G2P 转换为发音序列（如拼音）
          │           └─ 可导入/导出 .lab 文件
          │
          ▼
     发音序列 ────── Step 4: Alignment 音素对齐（模型可替换）
          │           ├─ 发音经模型自带词典 → 音素列表
          │           ├─ 对齐音频时间 → 音素时间序列
          │           └─ 可导入/导出 TextGrid 文件
          │
          ▼
     音素时间 ────── Step 5: PhonemeLabeler 音素修正（可选，人工）
          │
          ▼
     音素时间 ────── Step 6: AddPhNum 计算 ph_num（纯算法）
          │
          ▼
  音素+ph_num ────── Step 7: MIDI 提取（模型可替换）
          │
          ▼
     MIDI 数据 ───── Step 8: 音高提取（模型可替换）
          │
          ▼
    F0 曲线 ──────── Step 9: PitchLabeler 音高修正（人工）
          │
          ▼
                      Step 10: 导出（CSV / DS / 自定义格式）

    ※ 任意步骤可标记切片为"丢弃"，后续步骤自动跳过
```

### 3.1 各步骤 I/O 契约

| Step | 名称 | 类型 | 输入层 (category) | 输出层 (category) | 模型可替换 |
|------|------|------|-------------------|-------------------|-----------|
| 0 | AudioPreprocess | 自动 | — | — | 是 |
| 1 | AudioSlicer | 自动+人工 | — | `slices` | 是 |
| 2 | LyricFA | 自动 | — | `sentence` | 是 |
| 3 | MinLabel | 人工 | `sentence` | `grapheme` | 否 |
| 4 | Alignment | 自动 | `grapheme` | `phoneme` | 是 |
| 5 | PhonemeLabeler | 人工 | `phoneme` | `phoneme` | 否 |
| 6 | AddPhNum | 自动 | `phoneme`, `grapheme` | `ph_num` | 否 |
| 7 | MidiTranscription | 自动 | `phoneme`, `ph_num` | `midi` | 是 |
| 8 | PitchExtraction | 自动 | — | `pitch` | 是 |
| 9 | PitchLabeler | 人工 | `pitch`, `midi` | `pitch` | 否 |
| 10 | Export | 自动 | (all) | (files) | — |

### 3.2 层 category 定义

> **时间精度**：所有时间值以 `int64` 微秒存储。详见 [ds-format.md §1](ds-format.md)。

| category | JSON 形状 | MDS 对应列 | 说明 |
|----------|----------|-----------|------|
| `sentence` | `[{text, pos}]` | — | 整句歌词。`pos` = 微秒。 |
| `grapheme` | `[{text, pos}]` | `.lab` 内容 | 音节序列（如拼音）。`pos` = 微秒。 |
| `phoneme` | `[{phone, start, end}]` | `ph_seq` + `ph_dur` | 音素时间序列。`start`/`end` = 微秒。 |
| `ph_num` | `[int]` | `ph_num` | 每词音素个数 |
| `midi` | `[{note, duration, slur, glide}]` | `note_seq` + `note_dur` + `note_slur` + `note_glide` | MIDI 序列。`duration` = 微秒。 |
| `pitch` | `{f0: [int], timestep: int}` | `f0_seq` + `f0_timestep` | F0 曲线。`f0` = 毫赫兹(mHz)，`timestep` = 微秒。 |
| `slices` | `[{id, start, end, audioPath}]` | — | 切片边界。`start`/`end` = 微秒。 |

---

## 4 切片丢弃机制

### 4.1 需求

在任意步骤中，用户或自动检测可能发现某些切片不合适：
- 切片太短（< 0.5s）或太长（> 30s）
- ASR 识别失败或置信度低
- 音素对齐质量差
- 音高提取异常
- 用户主观判断不适合训练

### 4.2 设计

在 `PipelineContext` 中引入 `status` 字段：

```cpp
struct PipelineContext {
    // ... 现有字段 ...

    /// 切片状态
    enum class Status {
        Active,     ///< 正常参与流水线
        Discarded,  ///< 已丢弃，后续步骤跳过
        Error       ///< 处理出错（可恢复）
    };

    Status status = Status::Active;
    QString discardReason;              ///< 丢弃原因（人工备注或自动检测信息）
    QString discardedAtStep;            ///< 在哪一步被丢弃
};
```

### 4.3 行为规则

1. **丢弃传播**：一旦切片被标记为 `Discarded`，PipelineRunner 在后续所有步骤跳过该 item。
2. **可恢复**：用户可在任意步骤的 UI 中将 `Discarded` 改回 `Active`（如果前序步骤数据完整）。
3. **导出过滤**：Step 10 导出时默认排除 `Discarded` 切片（可选包含）。
4. **持久化**：status/reason 随 context JSON 持久化到 `dstemp/contexts/{itemId}.json`。
5. **UI 展示**：丢弃的切片在列表中灰显 + 删除线，可展开查看丢弃原因。

### 4.4 自动丢弃检测

PipelineRunner 在每步完成后可选执行验证回调：

```cpp
/// 验证单个 item 的处理结果，返回建议的 status。
using ValidationCallback = std::function<
    PipelineContext::Status(const PipelineContext &ctx, const TaskSpec &spec, QString &reason)>;
```

内置验证器示例：
- `SliceLengthValidator`：丢弃 < 0.3s 或 > maxLength 的切片
- `AlignmentQualityValidator`：丢弃对齐置信度低于阈值的切片
- `PitchCoverageValidator`：丢弃 F0 有效帧率 < 阈值的切片

---

## 5 撤销重做设计

### 5.1 原则

- 自动步骤（模型推理）：**不需要撤销重做**。重跑即可恢复，成本低。
- 人工步骤（MinLabel/PhonemeLabeler/PitchLabeler）：**必须撤销重做**。已有 QUndoStack 基础设施。
- 切片丢弃/恢复操作：**需要撤销重做**。

### 5.2 各步骤撤销重做支持

| Step | 是否需要 | 机制 | 说明 |
|------|---------|------|------|
| 0 预处理 | 否 | 重跑 | 可恢复原始音频 |
| 1 切片 | 部分 | QUndoStack | 手动切片点的添加/删除/移动可撤销。自动切片参数变更 → 全部重生成。 |
| 2 LyricFA | 否 | 重跑 | ASR 结果可重新运行 |
| 3 MinLabel | 是 | QUndoStack | 歌词编辑、G2P 修正 |
| 4 对齐 | 否 | 重跑 | 模型推理结果 |
| 5 PhonemeLabeler | 是 | QUndoStack | 音素边界拖动（已有 7 个 QUndoCommand） |
| 6 AddPhNum | 否 | 重跑 | 纯算法 |
| 7 MIDI | 否 | 重跑 | 模型推理 |
| 8 音高 | 否 | 重跑 | 模型推理 |
| 9 PitchLabeler | 是 | QUndoStack | F0 曲线编辑（已有 7 个 QUndoCommand） |
| 丢弃/恢复 | 是 | QUndoCommand | `DiscardSliceCommand` / `RestoreSliceCommand` |

### 5.3 切片丢弃撤销命令

```cpp
class DiscardSliceCommand : public QUndoCommand {
public:
    DiscardSliceCommand(PipelineContext &ctx, const QString &reason, const QString &step)
        : m_ctx(ctx), m_reason(reason), m_step(step) {
        setText(QObject::tr("Discard slice %1").arg(ctx.itemId));
    }

    void redo() override {
        m_ctx.status = PipelineContext::Status::Discarded;
        m_ctx.discardReason = m_reason;
        m_ctx.discardedAtStep = m_step;
    }

    void undo() override {
        m_ctx.status = PipelineContext::Status::Active;
        m_ctx.discardReason.clear();
        m_ctx.discardedAtStep.clear();
    }

private:
    PipelineContext &m_ctx;
    QString m_reason, m_step;
};
```

### 5.4 自动步骤的"重跑"替代撤销

自动步骤不做细粒度撤销。改为：
- PipelineRunner 在执行自动步骤前**快照**该步骤的输入状态
- "撤销"自动步骤 = 恢复快照 + 清除该步骤产出的层
- 实现方式：每步完成后将 context 序列化为 JSON，存入 `dstemp/snapshots/{itemId}_step{N}.json`

---

## 6 崩溃日志与错误恢复

### 6.1 已有基础设施

项目已有：
- `dsfw::CrashHandler`：跨平台 minidump
- `dsfw::FileLogSink`：7 天自动轮转文件日志
- `dsfw::AppPaths`：统一的 config/logs/dumps 路径
- `dsfw::BatchCheckpoint`：断点续处理

### 6.2 流水线级错误记录

每个步骤的执行结果记入 PipelineContext：

```cpp
struct StepRecord {
    QString stepName;
    QString processorId;
    QDateTime startTime;
    QDateTime endTime;
    bool success = false;
    QString errorMessage;
    ProcessorConfig usedConfig;         ///< 实际使用的参数（含模型路径）
};

struct PipelineContext {
    // ... 现有字段 ...
    std::vector<StepRecord> stepHistory;  ///< 完整的步骤执行历史
};
```

### 6.3 崩溃恢复

1. 每步执行前序列化 context → `dstemp/contexts/{itemId}.json`
2. 崩溃后重启，从 contexts/ 加载所有 item 的最新状态
3. 根据 `completedSteps` 和 `stepHistory` 判断从哪步恢复
4. CrashHandler 的 minidump 关联到当前正在处理的 itemId

---

## 7 核心类型修订

### 7.1 PipelineContext

```cpp
struct PipelineContext {
    QString audioPath;                          ///< 当前 item 的音频路径
    QString itemId;                             ///< item 标识（如 "guangnian_003"）
    std::map<QString, nlohmann::json> layers;   ///< category → layer data
    ProcessorConfig globalConfig;               ///< hopSize, sampleRate 等

    // ── 状态管理 ──
    enum class Status { Active, Discarded, Error };
    Status status = Status::Active;
    QString discardReason;
    QString discardedAtStep;

    // ── 步骤追踪 ──
    QStringList completedSteps;                 ///< 已完成步骤列表
    std::vector<StepRecord> stepHistory;        ///< 完整执行历史

    // ── 方法 ──
    Result<TaskInput> buildTaskInput(const TaskSpec &spec) const;
    void applyTaskOutput(const TaskSpec &spec, const TaskOutput &output);

    /// 序列化/反序列化
    nlohmann::json toJson() const;
    static Result<PipelineContext> fromJson(const nlohmann::json &j);
};
```

### 7.2 ITaskProcessor（不变）

```cpp
class ITaskProcessor {
public:
    virtual ~ITaskProcessor() = default;

    virtual QString processorId() const = 0;
    virtual QString displayName() const = 0;
    virtual TaskSpec taskSpec() const = 0;
    virtual ProcessorConfig capabilities() const { return {}; }

    virtual Result<void> initialize(IModelManager &mm,
                                    const ProcessorConfig &modelConfig) = 0;
    virtual void release() = 0;
    virtual Result<TaskOutput> process(const TaskInput &input) = 0;

    // processBatch 已移除 — 批量迭代由 PipelineRunner 统一控制。
};
```

### 7.3 IAudioPreprocessor

```cpp
class IAudioPreprocessor {
public:
    virtual ~IAudioPreprocessor() = default;
    virtual QString preprocessorId() const = 0;
    virtual QString displayName() const = 0;
    virtual Result<void> process(const QString &inputPath,
                                 const QString &outputPath,
                                 const ProcessorConfig &config) = 0;
};
```

### 7.4 IFormatAdapter

```cpp
class IFormatAdapter {
public:
    virtual ~IFormatAdapter() = default;
    virtual QString formatId() const = 0;
    virtual QString displayName() const = 0;

    virtual bool canImport() const { return false; }
    virtual bool canExport() const { return false; }

    /// 从外部文件导入层数据
    virtual Result<void> importToLayers(
        const QString &filePath,
        std::map<QString, nlohmann::json> &layers,
        const ProcessorConfig &config) = 0;

    /// 从层数据导出到外部文件
    virtual Result<void> exportFromLayers(
        const std::map<QString, nlohmann::json> &layers,
        const QString &outputPath,
        const ProcessorConfig &config) = 0;
};
```

---

## 8 格式适配器详细设计

### 8.1 LabAdapter — .lab 文件

```
格式ID: "lab"
方向: 双向

导入: 读取 .lab 文件 → 填充 grapheme 层
  文件格式: 单行文本，空格分隔音节
  解析: split(" ") → [{text: syllable, position: 0.0}]（无时间信息）

导出: grapheme 层 → 写 .lab 文件
  格式: 所有音节用空格连接为单行
```

### 8.2 TextGridAdapter — TextGrid 文件

```
格式ID: "textgrid"
方向: 双向

TextGrid 有两种布局：
  2-tier: [words, phones]          ← 切片级（每个切片一个 TextGrid）
  3-tier: [sentences, words, phones] ← 整首歌级（combine_tg 合并后）

导入（2-tier → 层数据）:
  "phones" tier → phoneme 层
    每个 interval → {phone: text, start: secToUs(minTime), end: secToUs(maxTime)}
    空标记替换为 "SP"
  "words" tier → grapheme 层（可选）
    每个 interval → {text: mark, pos: secToUs(minTime)}

导入（3-tier → 切片 + 层数据）:
  "sentences" tier → 切片边界
    每个 interval → {id: mark 或序号, start: secToUs(minTime), end: secToUs(maxTime)}
    mark 为空的 interval → 标记为 discarded（用户在 Praat/vLabeler 中删除句子的方式）
  对每个非空 sentence 区间：
    截取该时间段内的 words/phones interval → 生成对应切片的 grapheme/phoneme 层
    时间归零：pos -= sentenceStart

导出（层数据 → 2-tier）:
  phoneme 层 → "phones" tier
  grapheme 层 → "words" tier
  时间转换：usToSec(pos) → interval minTime/maxTime

导出（层数据 → 3-tier，用于精修工作流）:
  切片列表 → "sentences" tier（每个切片一个 sentence interval，mark = itemId）
  所有切片的 grapheme → 拼接成连续的 "words" tier
  所有切片的 phoneme → 拼接成连续的 "phones" tier
  discarded 切片 → mark 为空的 sentence interval
```

### 8.3 CsvAdapter — transcriptions.csv

```
格式ID: "csv"
方向: 双向

导入: 读取 CSV → 按列名填充各层
  name → itemId
  ph_seq + ph_dur → phoneme 层
  ph_num → ph_num 层
  note_seq + note_dur + note_slur + note_glide → midi 层

导出: 各层 → 写 CSV
  phoneme → ph_seq, ph_dur
  ph_num → ph_num
  midi → note_seq, note_dur, note_slur, note_glide
  仅写入有数据的列

内部使用 TranscriptionRow 做转换（保持现有 TranscriptionCsv 代码不变）
```

### 8.4 DsFileAdapter — .ds 文件

```
格式ID: "ds"
方向: 双向

导入: 读取 .ds JSON → 填充各层
  ph_seq + ph_dur → phoneme
  ph_num → ph_num
  note_seq + note_dur + note_slur + note_glide → midi
  f0_seq + f0_timestep → pitch
  offset → 记入 context metadata

导出: 各层 → 写 .ds JSON
  合成完整 .ds 结构（含 offset, text, f0_seq, f0_timestep）
  与 convert_ds.py csv2ds 输出格式完全一致
```

---

## 9 切片系统设计

### 9.1 命名规则

```
{原始音频前缀}_{序号}.wav

示例：
guangnian_001.wav   ← 第 1 段（按 startSample 排序）
guangnian_002.wav
guangnian_003.wav
```

序号从 001 起始，三位数零填充。MakeDiffSinger 兼容：其 `slice_tg.py` 输出 `item_000`, `item_001`... 同为前缀+序号模式。

### 9.2 切片流程

```
1. RMS 自动切片 → 切片边界列表
2. 验证：超过 maxLength 的片段
   ├─ 全部/大量超长 → 提示调参，重新自动切片
   └─ 个别超长 → 进入手动切片界面
3. 手动切片界面：
   ├─ 波形图 + 音频播放
   ├─ 在波形上点击标记切割点
   ├─ 切割点可拖动/删除（QUndoStack）
   └─ 仅处理标记为"过长"的片段
4. 写出切片音频 + 创建 PipelineContext
```

---

## 10 LyricFA 设计

### 10.1 以整首歌为单位

LyricFA 以**整首歌**为单位运行（`granularity: whole_audio`）。

1. ASR 识别整首歌音频 → 带时间戳的识别文本段
2. 按切片边界分割识别结果 → 每个切片分配一段
3. 如果提供原歌词 txt（按歌曲前缀匹配），与原歌词做最大相似度匹配
4. 输出每个切片的 `sentence` 层

### 10.2 原歌词目录

```
lyrics/
    guangnian.txt      ← 歌曲前缀对应的歌词文件
    wangqing.txt
```

每行一句歌词。匹配算法：编辑距离 / 最长公共子序列。

---

## 11 PipelineRunner 修订

### 11.1 StepConfig

```cpp
struct StepConfig {
    QString taskName;                ///< TaskProcessorRegistry 中的 taskName
    QString processorId;             ///< processorId（可替换后端）
    ProcessorConfig config;          ///< 步骤参数

    bool optional = false;           ///< 可选步骤
    bool manual = false;             ///< 人工步骤（暂停流水线）

    QString importFormat;            ///< 步骤前导入外部文件（可选）
    QString importPath;
    QString exportFormat;            ///< 步骤后导出（可选）
    QString exportPath;

    ValidationCallback validator;    ///< 步骤完成后的验证回调（可选）
};
```

### 11.2 执行流程伪代码

```cpp
Result<void> PipelineRunner::run(const Options &opts, ProgressCallback progress) {
    // Phase 0: 音频预处理
    for (auto &pre : opts.preprocessors)
        pre->process(audioPath, audioPath, opts.globalConfig);

    // Phase 1: 切片
    auto slices = runSlicerStep(opts);
    
    // Phase 2: 创建 PipelineContext per slice
    std::vector<PipelineContext> contexts = createContexts(slices);

    // Phase 3: 整首歌步骤（如 LyricFA）
    for (auto &step : opts.steps | filter(wholeAudio)) {
        runWholeAudioStep(step, originalAudioPath, contexts);
    }

    // Phase 4: 逐切片步骤
    for (auto &step : opts.steps | filter(perSlice)) {
        auto processor = registry.create(step.taskName, step.processorId);
        processor->initialize(mm, step.config);

        for (auto &ctx : contexts) {
            // 跳过已丢弃的切片
            if (ctx.status == PipelineContext::Status::Discarded)
                continue;

            // 人工步骤：暂停等待
            if (step.manual) {
                emit manualStepRequired(step, ctx);
                waitForManualCompletion();
                continue;
            }

            // 快照（用于自动步骤的"撤销"）
            saveSnapshot(ctx, step.taskName);

            // 导入
            if (!step.importFormat.isEmpty())
                formatAdapter(step.importFormat)->importToLayers(
                    step.importPath, ctx.layers, step.config);

            // 执行处理器
            auto inputResult = ctx.buildTaskInput(processor->taskSpec());
            if (!inputResult) {
                ctx.status = PipelineContext::Status::Error;
                logError(ctx, step, inputResult.error());
                continue;
            }
            auto output = processor->process(inputResult.value());
            if (!output) {
                ctx.status = PipelineContext::Status::Error;
                logError(ctx, step, output.error());
                continue;
            }
            ctx.applyTaskOutput(processor->taskSpec(), output.value());

            // 验证
            if (step.validator) {
                QString reason;
                auto newStatus = step.validator(ctx, processor->taskSpec(), reason);
                if (newStatus == PipelineContext::Status::Discarded) {
                    ctx.status = newStatus;
                    ctx.discardReason = reason;
                    ctx.discardedAtStep = step.taskName;
                }
            }

            // 记录步骤历史
            ctx.completedSteps.append(step.taskName);
            ctx.stepHistory.push_back({step.taskName, step.processorId, ...});

            // 导出
            if (!step.exportFormat.isEmpty())
                formatAdapter(step.exportFormat)->exportFromLayers(
                    ctx.layers, step.exportPath, step.config);

            // 持久化 context
            saveContext(ctx);
        }
    }
}
```

### 11.3 整首歌步骤 vs 逐切片步骤

| 粒度 | 步骤 |
|------|------|
| `whole_audio` | Step 0 (预处理), Step 1 (切片), Step 2 (LyricFA) |
| `per_slice` | Step 3-10 |

---

## 12 .dsproj 规范修订

### 12.1 tasks 声明

```json
{
    "tasks": [
        {
            "name": "audio_preprocess",
            "inputs": [],
            "outputs": [],
            "granularity": "whole_audio",
            "optional": true
        },
        {
            "name": "audio_slice",
            "inputs": [],
            "outputs": [{"slot": "slices", "category": "slices"}],
            "granularity": "whole_audio"
        },
        {
            "name": "asr",
            "inputs": [],
            "outputs": [{"slot": "sentence", "category": "sentence"}],
            "granularity": "whole_audio",
            "optional": true
        },
        {
            "name": "label_review",
            "inputs": [{"slot": "sentence", "category": "sentence"}],
            "outputs": [{"slot": "grapheme", "category": "grapheme"}],
            "granularity": "per_slice",
            "manual": true,
            "importFormats": ["lab"],
            "exportFormats": ["lab"]
        },
        {
            "name": "phoneme_alignment",
            "inputs": [{"slot": "grapheme", "category": "grapheme"}],
            "outputs": [{"slot": "phoneme", "category": "phoneme"}],
            "granularity": "per_slice",
            "importFormats": ["textgrid"],
            "exportFormats": ["textgrid"]
        },
        {
            "name": "phoneme_review",
            "inputs": [{"slot": "phoneme", "category": "phoneme"}],
            "outputs": [{"slot": "phoneme", "category": "phoneme"}],
            "granularity": "per_slice",
            "manual": true,
            "optional": true,
            "importFormats": ["textgrid"],
            "exportFormats": ["textgrid"]
        },
        {
            "name": "add_ph_num",
            "inputs": [
                {"slot": "phoneme", "category": "phoneme"},
                {"slot": "grapheme", "category": "grapheme"}
            ],
            "outputs": [{"slot": "ph_num", "category": "ph_num"}],
            "granularity": "per_slice"
        },
        {
            "name": "midi_transcription",
            "inputs": [
                {"slot": "phoneme", "category": "phoneme"},
                {"slot": "ph_num", "category": "ph_num"}
            ],
            "outputs": [{"slot": "midi", "category": "midi"}],
            "granularity": "per_slice"
        },
        {
            "name": "pitch_extraction",
            "inputs": [],
            "outputs": [{"slot": "pitch", "category": "pitch"}],
            "granularity": "per_slice"
        },
        {
            "name": "pitch_review",
            "inputs": [
                {"slot": "pitch", "category": "pitch"},
                {"slot": "midi", "category": "midi"}
            ],
            "outputs": [{"slot": "pitch", "category": "pitch"}],
            "granularity": "per_slice",
            "manual": true,
            "importFormats": ["ds"],
            "exportFormats": ["ds"]
        },
        {
            "name": "export",
            "inputs": [
                {"slot": "phoneme", "category": "phoneme"},
                {"slot": "ph_num", "category": "ph_num"},
                {"slot": "midi", "category": "midi"},
                {"slot": "pitch", "category": "pitch"}
            ],
            "outputs": [],
            "granularity": "per_slice",
            "exportFormats": ["csv", "ds"]
        }
    ]
}
```

新增字段：
- `granularity`: `"whole_audio"` | `"per_slice"`
- `optional`: 可选步骤
- `manual`: 人工步骤
- `importFormats` / `exportFormats`: 该步骤支持的导入/导出格式列表

### 12.2 defaults 扩展

```json
{
    "defaults": {
        "preprocessors": [
            {"id": "loudness_norm", "targetLufs": -23.0},
            {"id": "denoise", "model": "models/denoise/model.onnx"}
        ],
        "slicer": {
            "threshold": -40.0,
            "minLength": 5000,
            "minInterval": 300,
            "hopSize": 10,
            "maxSilKept": 500,
            "maxSliceLength": 15000,
            "namingPrefix": "auto"
        },
        "validation": {
            "minSliceLength": 0.3,
            "maxSliceLength": 30.0,
            "minPitchCoverage": 0.5
        },
        "models": {
            "asr": {
                "processor": "funasr",
                "path": "models/asr/funasr_cn.onnx",
                "provider": "dml"
            },
            "phoneme_alignment": {
                "processor": "hubert-fa",
                "path": "models/alignment/hubert_fa.onnx",
                "provider": "dml"
            },
            "midi_transcription": {
                "processor": "game",
                "path": "models/game/",
                "provider": "dml"
            },
            "pitch_extraction": {
                "processor": "rmvpe",
                "path": "models/rmvpe/rmvpe.onnx",
                "provider": "cpu"
            }
        },
        "export": {
            "formats": ["csv", "ds"],
            "csvColumns": ["name", "ph_seq", "ph_dur", "ph_num", "note_seq", "note_dur", "note_glide"],
            "dsHopSize": 512,
            "dsSampleRate": 44100,
            "includeDiscarded": false
        }
    }
}
```

### 12.3 items 扩展

```json
{
    "items": [
        {
            "id": "a54c548a",
            "name": "GuangNianZhiWai",
            "speaker": "32e9",
            "language": "fe67",
            "audioSource": "audio/a54c548a.wav",
            "slices": [
                {
                    "id": "001",
                    "in": 1.14,
                    "out": 5.14,
                    "status": "active",
                    "discardReason": null
                },
                {
                    "id": "002",
                    "in": 8.17,
                    "out": 19.26,
                    "status": "active"
                },
                {
                    "id": "003",
                    "in": 22.50,
                    "out": 23.10,
                    "status": "discarded",
                    "discardReason": "Too short (0.6s)",
                    "discardedAtStep": "audio_slice"
                }
            ]
        }
    ]
}
```

新增 slice 字段：
- `status`: `"active"` | `"discarded"` | `"error"`
- `discardReason`: 丢弃原因
- `discardedAtStep`: 在哪步被丢弃

---

## 13 数据持久化

### 13.1 dstemp/ 目录结构

```
dstemp/
    preprocess/                        ← Step 0: 预处理后的音频
        guangnian.wav
    slices/                            ← Step 1: 切片音频
        guangnian_001.wav
        guangnian_002.wav
    contexts/                          ← 每个切片的 PipelineContext
        guangnian_001.json
        guangnian_002.json
    snapshots/                         ← 自动步骤快照（用于"撤销"自动步骤）
        guangnian_001_phoneme_alignment.json
        guangnian_001_pitch_extraction.json
    export/                            ← Step 10: 导出结果
        wavs/                          ← MakeDiffSinger 兼容目录结构
            guangnian_001.wav
            guangnian_001.ds
        transcriptions.csv
```

### 13.2 Context JSON 格式

```json
{
    "itemId": "guangnian_001",
    "audioPath": "dstemp/slices/guangnian_001.wav",
    "status": "active",
    "discardReason": null,
    "discardedAtStep": null,
    "completedSteps": ["audio_slice", "asr", "label_review", "phoneme_alignment"],
    "stepHistory": [
        {
            "stepName": "phoneme_alignment",
            "processorId": "hubert-fa",
            "startTime": "2026-05-02T10:30:00",
            "endTime": "2026-05-02T10:30:05",
            "success": true,
            "errorMessage": null,
            "usedConfig": {"path": "models/hubert_fa.onnx", "provider": "dml"}
        }
    ],
    "layers": {
        "sentence": [{"text": "感受停在", "position": 0.0}],
        "grapheme": [{"text": "gan", "position": 0.0}, {"text": "shou", "position": 0.6}],
        "phoneme": [
            {"phone": "g", "start": 0.0, "end": 0.08},
            {"phone": "an", "start": 0.08, "end": 0.52}
        ],
        "ph_num": [2, 2, 1],
        "midi": [
            {"note": "C4", "duration": 0.6, "slur": 0, "glide": "none"},
            {"note": "D4", "duration": 0.6, "slur": 0, "glide": "none"}
        ],
        "pitch": {
            "f0": [0.0, 261.6, 293.7],
            "timestep": 0.011609977
        }
    }
}
```

---

## 14 模型可替换性

同一 taskName 下所有处理器必须声明相同的 inputs/outputs category：

```
phoneme_alignment:
    hubert-fa → in: [grapheme], out: [phoneme]
    sofa      → in: [grapheme], out: [phoneme]
    mfa       → in: [grapheme], out: [phoneme]

midi_transcription:
    game      → in: [phoneme, ph_num], out: [midi]
    some      → in: [phoneme, ph_num], out: [midi]

pitch_extraction:
    rmvpe     → in: [], out: [pitch]
    fcpe      → in: [], out: [pitch]
    crepe     → in: [], out: [pitch]

asr:
    funasr    → in: [], out: [sentence]
    whisper   → in: [], out: [sentence]
```

Registry 注册时验证一致性，不一致拒绝注册。

---

## 15 迁移计划

| 阶段 | 内容 | 影响 |
|------|------|------|
| M1 | PipelineContext, IAudioPreprocessor, IFormatAdapter | 纯新增 |
| M2 | PipelineRunner + StepConfig + ValidationCallback | 纯新增 |
| M3 | 4 个 IFormatAdapter（Lab/TextGrid/CSV/DS） | 新增，内部包装现有代码 |
| M4 | ITaskProcessor 移除 processBatch | 4 个处理器适配 |
| M5 | SlicerProcessor, AddPhNumProcessor | 包装现有代码 |
| M6 | DiscardSliceCommand + 切片 status 持久化 | 新增 |
| M7 | DiffSingerLabeler 页面切换到 PipelineRunner | 应用层 |
| M8 | TranscriptionPipeline deprecated | 渐进 |

---

## 16 设计决策记录

| ADR | 决策 | 理由 |
|-----|------|------|
| ADR-30 | 层数据保持 nlohmann::json | 离线处理，JSON 开销可忽略。避免类型耦合。 |
| ADR-31 | PipelineContext 用 category 做扁平键 | 与 .dsproj tasks 的 category 绑定一致。 |
| ADR-32 | 移除 processBatch | 处理器只关心单项。批量迭代、丢弃跳过、错误记录归 Runner。 |
| ADR-33 | TranscriptionRow 降级为适配器内部 | 最小迁移爆炸半径。 |
| ADR-34 | 切片命名 {prefix}_{NNN}.wav | 按时间顺序，兼容 MDS 的 item_NNN 模式。 |
| ADR-35 | LyricFA 以整首歌为粒度 | ASR 需要完整上下文。 |
| ADR-36 | 同 taskName 处理器 I/O 必须一致 | 模型可替换的前提。Registry 注册时验证。 |
| ADR-37 | Context JSON 替代 .dsitem + BatchCheckpoint | 统一一个文件。 |
| ADR-38 | 音频预处理独立接口 | 操作音频文件，不产出层数据。 |
| ADR-39 | 切片丢弃通过 status 字段 + 传播 | 简单可靠，不需要额外的排除列表。丢弃操作可撤销（QUndoCommand）。 |
| ADR-40 | 自动步骤用快照替代细粒度撤销 | 自动步骤（模型推理）没有中间态，重跑 = 撤销。快照保证一致性。 |
| ADR-41 | 导入/导出格式声明在 task 定义中 | .dsproj 的 tasks[] 包含 importFormats/exportFormats，运行时 PipelineRunner 据此查找适配器。 |
| ADR-42 | MDS 兼容通过格式适配器实现 | 不在核心流水线中引入 MDS 的目录约定。导出时由 CsvAdapter/DsFileAdapter 生成兼容布局。 |

---

## 关联文档

- [ds-format.md](ds-format.md) — .dsproj / .dstext 格式规范
- [architecture-defects-and-tech-debt.md](architecture-defects-and-tech-debt.md)
- [improvement-directions.md](improvement-directions.md)
- [refactoring-roadmap.md](refactoring-roadmap.md)

> 更新时间：2026-05-02
