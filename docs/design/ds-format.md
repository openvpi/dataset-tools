# DiffSinger Dataset Tools 工程格式规范

> 版本 3.1.0 — 2026-05-04
>
> v3.1.0 向后兼容 v3.0.0（读取时自动迁移 `defaults` 字段）。

---

## 1 时间精度约定

### 1.1 内部表示

所有时间值在运行时以 **`int64_t` 微秒**（μs）存储。

```cpp
using TimePos = int64_t;  // 微秒，1 秒 = 1'000'000
```

- 精度：1 μs（0.000001 秒），足够覆盖 192 kHz 采样率下的单采样精度（~5.2 μs）
- 范围：±9.2 × 10¹² μs ≈ ±106 天
- 运算：整数加减乘除，**无浮点累积误差**

### 1.2 文件存储

JSON 文件（.dsproj、.dstext）中，时间值以**整数微秒**存储：

```json
{ "pos": 1200000 }   // 1.200000 秒
```

### 1.3 导入/导出转换

| 外部格式 | 存储精度 | 导入转换 | 导出转换 |
|---------|---------|---------|---------|
| MakeDiffSinger CSV | 6 位小数秒 | `round(sec × 1e6)` → `int64` | `int64 / 1e6` → `"%.6f"` |
| TextGrid | 双精度秒 | `round(sec × 1e6)` → `int64` | `int64 / 1e6` → `double` |
| .ds JSON | 6 位小数字符串 | 同 CSV | 同 CSV |

### 1.4 比较

两个时间点的相等判断：**直接整数比较，无容差**。

### 1.5 路径约定

所有文件路径在 JSON 中以 **POSIX 风格正斜杠** `/` 存储，运行时由 `QDir::toNativeSeparators()` 转换。确保跨平台一致性。

---

## 2 工程目录结构

```
myproject/
    myproject.dsproj            ← 工程描述文件
    audio/                      ← 源音频（纳入版本控制）
        a54c548a.wav
    items/                      ← 标注文件
        a54c548a_GuangNianZhiWai/
            001_GanShouTingZai.dstext
            002_RuHeShunJianDongJie.dstext
    dictionaries/               ← G2P 词典
        CHN/
            dict.txt
            phoneset.txt
    dstemp/                     ← 中间文件（.gitignore）
        slices/                 ← 切片音频
        contexts/               ← PipelineContext JSON
        snapshots/              ← 自动步骤快照
```

### 2.1 audio/

源音频文件。文件名为 item ID 加扩展名。纳入版本控制。

### 2.2 items/

每个 item 一个子目录 `{itemId}_{itemName}/`，内含该 item 所有切片的 `.dstext` 标注文件。

切片 dstext 文件名：`{sliceId}_{sliceName}.dstext`。

### 2.3 dictionaries/

每种语言一个子目录，目录名为语言名称（如 `CHN`、`JPN`）。包含：
- `dict.txt`：`音节\t音素1 [音素2]`
- `phoneset.txt`：空格分隔音素表

### 2.4 dstemp/

中间文件，不纳入版本控制。

| 子目录 | 内容 |
|--------|------|
| `slices/` | 切片音频（{prefix}_{NNN}.wav） |
| `contexts/` | 每个切片的 PipelineContext JSON |
| `snapshots/` | 自动步骤快照（用于撤销） |

---

## 3 工程描述文件（.dsproj）

JSON 格式，扩展名 `.dsproj`。

### 3.1 顶层结构

```json
{
    "version": "3.1.0",
    "name": "Example Singing Dataset",
    "workingDirectory": "...",
    "slicer": {...},
    "export": {...},
    "items": [...]
}
```

**v3.1 变更**：
- 废弃 `defaults` 字段——模型/设备配置迁移到用户目录（`AppSettings`），`slicer` 和 `export` 提升为顶层字段。旧版 `.dsproj` 中的 `defaults` 在读取时自动迁移，不再写出。
- 新增 `workingDirectory` 字段。
- 移除 `tasks[]` — 流水线步骤在代码中固定定义，不在工程文件中声明。
- 路径字段统一用 POSIX 正斜杠。

### 3.2 schema — 标注层结构定义

```json
"schema": [
    {"type": "text",  "name": "lyrics",  "category": "sentence"},
    {"type": "text",  "name": "pinyin",  "category": "grapheme",  "subdivisionOf": 0},
    {"type": "note",  "name": "midi",    "category": "midi",      "subdivisionOf": 1},
    {"type": "text",  "name": "phone",   "category": "phoneme",   "subdivisionOf": 1},
    {"type": "text",  "name": "slur",    "category": "custom",    "alignWith": 3}
]
```

每个 Layer 定义：

| 字段 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `type` | `string` | 是 | `"text"` / `"note"` / `"curve"` |
| `name` | `string` | 是 | 层名称 |
| `category` | `string` | 否 | 层类别，用于任务路由 |
| `subdivisionOf` | `integer` | 否 | 本层是目标层（索引）的细分 |
| `alignWith` | `integer` | 否 | 本层与目标层一对一对齐 |

层间关系语义：

| 关系 | 含义 | 绑定行为 |
|------|------|---------|
| `subdivisionOf` | 子层每个区间完全落在父层某个区间内 | 移动父层边界时，子层首边界跟随 |
| `alignWith` | 两层完全一对一对齐 | 移动一层边界，另一层对应边界跟随 |

### 3.3 slicer — 切片状态

```json
"slicer": {
    "params": {
        "threshold": -40.0,
        "minLength": 5000,
        "minInterval": 300,
        "hopSize": 10,
        "maxSilence": 500
    },
    "audioFiles": ["audio/song1.wav", "audio/song2.wav"],
    "slicePoints": {
        "audio/song1.wav": [1.234, 5.678, 10.012],
        "audio/song2.wav": [0.5, 3.2]
    }
}
```

**v3.1 变更**：从 `defaults.slicer` 提升为顶层 `slicer`，新增 `audioFiles` 和 `slicePoints`。

#### 3.3.1 slicer.params — 切片参数

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `threshold` | `number` | -40.0 | 静音检测阈值（dB） |
| `minLength` | `integer` | 5000 | 最短切片长度（ms） |
| `minInterval` | `integer` | 300 | 最短切片间隔（ms） |
| `hopSize` | `integer` | 10 | 帧移（ms） |
| `maxSilence` | `integer` | 500 | 切片边缘保留最大静音（ms） |

#### 3.3.2 slicer.audioFiles — 音频文件列表

`array<string>`，Slicer 页面加载的音频文件路径（POSIX 正斜杠，相对于工程目录）。

#### 3.3.3 slicer.slicePoints — 切点映射

`object<string, array<number>>`，key 为音频文件路径，value 为该文件的切点时间列表（秒，浮点数）。

### 3.4 export — 导出配置

```json
"export": {
    "formats": ["csv", "ds"],
    "hopSize": 512,
    "sampleRate": 44100,
    "resampleRate": 44100,
    "includeDiscarded": false
}
```

**v3.1 变更**：从 `defaults.export` 提升为顶层 `export`。

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `formats` | `array<string>` | `["csv", "ds"]` | 导出格式列表 |
| `hopSize` | `integer` | 512 | 帧移（采样点数） |
| `sampleRate` | `integer` | 44100 | 采样率（Hz） |
| `resampleRate` | `integer` | 44100 | 重采样率（Hz） |
| `includeDiscarded` | `boolean` | false | 是否包含已丢弃切片 |

### 3.5 defaults 字段迁移（向后兼容）

旧版 v3.0.0 的 `defaults` 字段在读取时自动迁移：

| 旧路径 | 迁移目标 | 说明 |
|--------|---------|------|
| `defaults.slicer` | `slicer.params` | 切片参数 |
| `defaults.export` | `export` | 导出配置 |
| `defaults.hopSize` | `export.hopSize` | 旧版顶层 hopSize |
| `defaults.sampleRate` | `export.sampleRate` | 旧版顶层 sampleRate |
| `defaults.models` | `AppSettings`（用户目录） | 模型/设备配置不再存工程文件 |
| `defaults.preload` | `AppSettings`（用户目录） | 预加载配置不再存工程文件 |
| `defaults.validation` | — | 不再持久化 |
| `defaults.preprocessors` | — | 不再持久化 |

`save()` 不再写出 `defaults` 字段。

### 3.6 languages

```json
"languages": [
    {
        "id": "CHN",
        "name": "Chinese",
        "g2pProvider": "cpp-pinyin",
        "dictionaryPath": "dictionaries/CHN"
    }
]
```

### 3.7 speakers

```json
"speakers": [
    {"id": "spk1", "name": "SingerA"}
]
```

### 3.8 items — 音频项目与切片

```json
"items": [
    {
        "id": "a54c548a",
        "name": "GuangNianZhiWai",
        "speaker": "spk1",
        "language": "CHN",
        "audioSource": "audio/a54c548a.wav",
        "slices": [
            {
                "id": "001",
                "name": "GanShouTingZai",
                "in": 1140000,
                "out": 5140000,
                "status": "active"
            },
            {
                "id": "002",
                "name": "RuHeShunJianDongJie",
                "in": 8170000,
                "out": 19260000,
                "status": "active"
            },
            {
                "id": "003",
                "name": "TaiDuan",
                "in": 22500000,
                "out": 23100000,
                "status": "discarded",
                "discardReason": "Too short (0.6s)",
                "discardedAt": "slicer"
            }
        ]
    }
]
```

Slice 字段：

| 字段 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `id` | `string` | 是 | 切片编号 |
| `name` | `string` | 否 | 切片名称（用于 dstext 文件名和导出 wav 命名） |
| `in` | `integer` | 是 | 入点（微秒） |
| `out` | `integer` | 是 | 出点（微秒） |
| `language` | `string` | 否 | 覆盖 item 的语言 |
| `status` | `string` | 否 | `"active"`（默认）/ `"discarded"` / `"error"` |
| `discardReason` | `string` | 否 | 丢弃原因 |
| `discardedAt` | `string` | 否 | 在哪步被丢弃 |

---

## 4 标注文件（.dstext）

JSON 格式，扩展名 `.dstext`，每个文件对应一个切片的标注数据。

### 4.1 完整示例

```json
{
    "version": "3.0.0",
    "audio": {
        "path": "../../audio/a54c548a.wav",
        "in": 1140000,
        "out": 5140000
    },
    "layers": [
        {
            "name": "lyrics",
            "type": "text",
            "boundaries": [
                {"id": 1, "pos": 0,       "text": "感受"},
                {"id": 2, "pos": 1200000,  "text": "停在"},
                {"id": 3, "pos": 2400000,  "text": "我"},
                {"id": 4, "pos": 3000000,  "text": "发端的"},
                {"id": 5, "pos": 4000000,  "text": ""}
            ]
        },
        {
            "name": "pinyin",
            "type": "text",
            "boundaries": [
                {"id": 6,  "pos": 0,       "text": "gan"},
                {"id": 7,  "pos": 600000,  "text": "shou"},
                {"id": 14, "pos": 4000000, "text": ""}
            ]
        },
        {
            "name": "phone",
            "type": "text",
            "boundaries": [
                {"id": 24, "pos": 0,       "text": "g"},
                {"id": 25, "pos": 80000,   "text": "an"},
                {"id": 40, "pos": 4000000, "text": ""}
            ]
        },
        {
            "name": "midi",
            "type": "note",
            "boundaries": [
                {"id": 15, "pos": 0,       "text": "C4"},
                {"id": 16, "pos": 600000,  "text": "D4"},
                {"id": 23, "pos": 4000000, "text": ""}
            ]
        },
        {
            "name": "slur",
            "type": "text",
            "boundaries": [
                {"id": 41, "pos": 0,       "text": "0"},
                {"id": 49, "pos": 4000000, "text": ""}
            ]
        },
        {
            "name": "f0",
            "type": "curve",
            "timestep": 11610,
            "values": [0, 261600, 293700]
        }
    ],
    "groups": [
        [1, 6, 15, 24, 41],
        [5, 14, 23, 40, 49]
    ],
    "meta": {
        "editedSteps": ["phoneme_review", "pitch_review"]
    }
}
```

### 4.2 audio

| 字段 | 类型 | 说明 |
|------|------|------|
| `path` | `string` | 音频文件相对路径（POSIX 正斜杠） |
| `in` | `integer` | 切片起始（微秒） |
| `out` | `integer` | 切片结束（微秒） |

### 4.3 layers

**v3 变更**：层定义中新增 `type` 字段，使 .dstext 自描述，不依赖 .dsproj 的 schema。

区间层（text/note）：

| 字段 | 类型 | 说明 |
|------|------|------|
| `name` | `string` | 层名称 |
| `type` | `string` | `"text"` 或 `"note"` |
| `boundaries` | `array<Boundary>` | 切割标记点 |

曲线层（curve）：

| 字段 | 类型 | 说明 |
|------|------|------|
| `name` | `string` | 层名称 |
| `type` | `string` | `"curve"` |
| `timestep` | `integer` | 采样间隔（微秒） |
| `values` | `array<integer>` | 采样值。F0：毫赫兹（mHz），0 = 无声帧 |

每个 Boundary：

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | `integer` | 文件内唯一编号，单调递增 |
| `pos` | `integer` | 切割时间点（微秒） |
| `text` | `string` | 该切点右侧区间的内容 |

自动边界规则：
- 最左边界 `pos` 恒为 0
- 最右边界 `pos` = 切片长度（`out - in`），`text` 恒为空

### 4.4 groups — 跨层边界绑定

`array<array<integer>>`。每个子数组内的切点 ID 构成绑定组，移动任一切点时组内其他切点跟随移动相同偏移。

groups 由 PipelineRunner 根据 schema 层间关系自动推导：
1. `subdivisionOf`：子层中与父层位置相同的切点绑定
2. `alignWith`：两层对应切点一一绑定

用户可手动解绑或重新绑定。

### 4.5 meta — 标注元数据

**v3 新增**。记录该切片的标注状态。

| 字段 | 类型 | 说明 |
|------|------|------|
| `editedSteps` | `array<string>` | 用户手动编辑过的步骤列表。用于判断导出行为（如 `"pitch_review"` 存在时生成 .ds 文件） |

---

## 5 PipelineContext（中间数据格式）

PipelineContext 是运行时层数据的暂存格式，持久化为 `dstemp/contexts/{sliceId}.json`。

### 5.1 与 .dstext 的关系

| 维度 | .dstext | PipelineContext |
|------|---------|-----------------|
| 用途 | 持久标注数据，纳入版本控制 | 运行时中间数据，不纳入版本控制 |
| 层格式 | 统一 Boundary 格式 `{id, pos, text}` | 按 category 的紧凑格式（见下） |
| 生命周期 | 用户保存时写入 | 每步完成后自动写入 |

### 5.2 层 category 格式

PipelineContext 中的层数据以 category 为 key、紧凑 JSON 为 value：

| category | JSON 形状 | 说明 |
|----------|----------|------|
| `sentence` | `[{text, pos}]` | 整句歌词。`pos` = 微秒 |
| `grapheme` | `[{text, pos}]` | 音节序列 |
| `phoneme` | `[{phone, start, end}]` | 音素时间序列 |
| `ph_num` | `[int]` | 每词音素个数 |
| `midi` | `[{note, duration, slur, glide}]` | MIDI 序列。`duration` = 微秒 |
| `pitch` | `{f0: [int], timestep: int}` | F0：毫赫兹，timestep：微秒 |

### 5.3 与 .dstext Boundary 的转换

加载 .dstext 到 PipelineContext 时：

```
phoneme 层 boundaries → [{phone: b[i].text, start: b[i].pos, end: b[i+1].pos}]
grapheme 层 boundaries → [{text: b[i].text, pos: b[i].pos}]
midi 层 boundaries → [{note: b[i].text, duration: b[i+1].pos - b[i].pos, slur: slur_layer[i].text, glide: "none"}]
```

保存 PipelineContext 到 .dstext 时做逆向转换，自动分配 Boundary ID。

### 5.4 Context JSON 格式

```json
{
    "sliceId": "001",
    "itemId": "a54c548a",
    "audioPath": "dstemp/slices/guangnian_001.wav",
    "status": "active",
    "discardReason": null,
    "discardedAt": null,
    "dirty": ["ph_num", "midi"],
    "stepHistory": [
        {
            "step": "phoneme_alignment",
            "processor": "hubert-fa",
            "time": "2026-05-02T10:30:05Z",
            "success": true,
            "error": null
        }
    ],
    "editedSteps": ["phoneme_review"],
    "layers": {
        "grapheme": [...],
        "phoneme": [...],
        "ph_num": [...],
        "midi": [...],
        "pitch": {...}
    }
}
```

**v3 变更**：
- 合并 `completedSteps` 和 `stepHistory` 为单一 `stepHistory`（completedSteps 可从 stepHistory 推导）
- 新增 `editedSteps`：记录用户手动编辑过哪些步骤，同步到 .dstext 的 `meta.editedSteps`
- 新增 `dirty`：标记哪些层因上游修改而过期（详见 §6）

---

## 6 层依赖与脏数据失效

### 6.1 层依赖 DAG

各层之间存在数据依赖关系。当用户修改上游层时，所有下游派生层自动失效（标记为 dirty）。

```
grapheme ──→ phoneme_alignment ──→ phoneme
                                      │
                      phoneme + grapheme ──→ add_ph_num ──→ ph_num
                                                              │
                                            phoneme + ph_num ──→ midi_transcription ──→ midi
                                                                                         │
                                                   (audio) ──→ pitch_extraction ──→ pitch ──→ (display with midi)
```

**精确依赖表**：

| 层 category | 依赖的上游层 | 产生该层的步骤 |
|-------------|------------|--------------|
| `sentence` | — | asr |
| `grapheme` | sentence（可选，ASR 辅助） | label_review（人工） |
| `phoneme` | grapheme | phoneme_alignment |
| `ph_num` | phoneme, grapheme | add_ph_num |
| `midi` | phoneme, ph_num | midi_transcription |
| `pitch` | (audio only) | pitch_extraction |

**关键观察**：
- `pitch` 不依赖 phoneme/midi，仅依赖音频。修改音素边界**不会**使 F0 曲线失效。
- `midi` 依赖 phoneme + ph_num。修改音素边界会使 midi 失效。
- `ph_num` 依赖 phoneme + grapheme。修改音素边界或歌词发音会使 ph_num 失效。

### 6.2 dirty 标记

PipelineContext 新增 `dirty` 字段，记录哪些层因上游修改而过期：

```json
{
    "dirty": ["ph_num", "midi"]
}
```

**dirty 标记是 per-slice 的**。修改某个切片的音素不影响其他切片。

### 6.3 失效传播规则

当用户在编辑器中修改某个层并保存时，系统自动计算受影响的下游层：

| 用户修改的层 | 自动标记 dirty 的层 |
|------------|-------------------|
| `grapheme`（歌词/发音修改） | `phoneme`, `ph_num`, `midi` |
| `phoneme`（音素边界/名称修改） | `ph_num`, `midi` |
| `midi`（MIDI 音符修改） | （无下游） |
| `pitch`（F0 曲线修改） | （无下游） |

**传播是传递性的**：grapheme 修改 → phoneme dirty → ph_num dirty → midi dirty。

### 6.4 自动重算时机

dirty 层在以下时机自动重新计算：

1. **切换到依赖该层的页面时**：
   - 进入 PitchLabeler 页 → 如果 `ph_num` 或 `midi` 是 dirty → 自动重算 add_ph_num + midi_transcription
   - 进入 PhonemeLabeler 页 → 如果 `phoneme` 是 dirty → 提示用户重新执行 FA（因为 grapheme 变了）

2. **导出时**：所有 dirty 层在导出前自动重算

3. **不会在编辑期间自动重算**：用户在 PhonemeLabeler 里编辑音素边界时，ph_num/midi 只是标记 dirty，不立即重算。等用户离开该页面时才处理。

### 6.5 Context JSON 中的 dirty 字段

```json
{
    "sliceId": "001",
    "itemId": "a54c548a",
    "dirty": ["ph_num", "midi"],
    "editedSteps": ["phoneme_review"],
    "stepHistory": [...],
    "layers": {
        "phoneme": [...],
        "ph_num": [2, 2, 1],
        "midi": [...]
    }
}
```

dirty 列表中的层数据**仍然保留**（是旧版本），但标记为不可信。重算后从 dirty 列表中移除。

### 6.6 用户回退场景详解

**场景：用户在 PitchLabeler 中发现音素边界不对，回到 PhonemeLabeler 修改**

```
1. 用户在 PitchLabeler 页面编辑切片 #001 的 pitch
2. 发现第 3 个音素的边界偏了
3. 切换到 PhonemeLabeler 页（侧边栏点击）
4. PhonemeLabeler 打开切片 #001，修正音素边界
5. 用户保存（Ctrl+S 或自动保存）
6. 系统检测 phoneme 层被修改 → 标记 ph_num, midi 为 dirty
   （pitch 不标记 dirty，因为 F0 不依赖 phoneme）
7. 用户切换回 PitchLabeler 页
8. PitchLabeler 检测到 ph_num, midi 是 dirty → 自动重算:
   a. add_ph_num（瞬时）
   b. midi_transcription（GAME 推理，几秒）
9. 弹出 Toast 通知："已重新计算 ph_num 和 MIDI（音素边界已修改）"
   Toast 3 秒后自动消失
10. PitchLabeler 刷新显示，用户继续编辑 pitch
```

**场景：用户在 PhonemeLabeler 中发现歌词发音不对，回到 MinLabel 修改**

```
1. 用户在 PhonemeLabeler 编辑切片 #005
2. 发现歌词发音错误（"shou" 应为 "sou"）
3. 切换到 MinLabel 页
4. 修正发音
5. 系统标记 phoneme, ph_num, midi 为 dirty
6. 用户切换回 PhonemeLabeler
7. PhonemeLabeler 检测 phoneme 是 dirty → 自动重新执行 FA
8. Toast: "已重新执行强制对齐（歌词发音已修改）"
9. 用户继续修正音素边界
```

---

## 7 其他设计说明

### 7.1 为什么移除 tasks[]

v2 的 `tasks[]` 在 .dsproj 中声明流水线步骤（inputs/outputs/granularity/optional/manual 等）。这在理论上支持用户自定义流水线，但实际上：

1. DsLabeler 的流水线是固定的（切片→ASR→歌词→对齐→音素修正→音高→导出）
2. 步骤的 I/O 契约在代码中硬编码（TaskProcessorRegistry 验证）
3. 用户唯一的配置自由度是**选用哪个处理器**（如 hubert-fa vs sofa），这通过 `AppSettings` 配置

因此 `tasks[]` 是死配置，增加复杂度但不提供价值。移除后：
- 步骤定义在代码中（`PipelineStepRegistry`）
- .dsproj 仅存储工程数据（items、slicer、export）
- 新增处理器只需注册到 Registry，无需修改 .dsproj schema

### 7.2 为什么 .dstext 新增 type 字段

v2 的 .dstext 层没有 `type` 字段，需要对照 .dsproj 的 `schema` 才知道某层是 text/note/curve。这导致：
- .dstext 文件不自描述
- LabelSuite（无工程文件）无法正确解析 .dstext

v3 在每个层定义中加入 `type`，使 .dstext 完全自描述。

### 7.3 为什么废弃 defaults 字段

v3.0.0 的 `defaults` 将模型路径、设备选择、UI 偏好等用户配置存储在工程文件中，导致：
- 换工程后需要重新配置模型路径和设备
- 团队协作时模型路径因机器不同而冲突
- 工程文件包含与数据无关的用户偏好

v3.1.0 将所有用户配置迁移到 `AppSettings`（用户目录），`.dsproj` 仅保留工程数据。旧版 `defaults` 在读取时自动迁移，不再写出。

### 7.4 editedSteps 的用途

`meta.editedSteps` 记录用户在哪些步骤中做了手动编辑。核心用途：

- **导出判定**：如果 `editedSteps` 不含 `"pitch_review"`，则该切片不生成 .ds 文件（未校正的 F0/MIDI 不适合训练）
- **进度追踪**：UI 可显示哪些切片需要人工审核

注意是 **per-slice** 而非全局标记。某些切片可能经过 PitchLabeler 编辑，某些没有。

### 7.5 路径的跨平台处理

所有 JSON 中的路径字段使用 POSIX 正斜杠 `/`：
- Windows 上 `QDir::toNativeSeparators()` 转为 `\`
- macOS/Linux 上不变
- 存储时 `QDir::fromNativeSeparators()` 转回 `/`

音频路径使用相对路径（相对于 .dsproj 或 .dstext 所在目录），避免绝对路径在不同机器上失效。

---

## 关联文档

- [task-processor-design.md](task-processor-design.md) — 流水线架构设计
- [unified-app-design.md](unified-app-design.md) — LabelSuite + DsLabeler 设计
- [framework-architecture.md](framework-architecture.md) — 框架架构

> 更新时间：2026-05-04
