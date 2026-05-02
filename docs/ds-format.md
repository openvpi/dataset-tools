# DiffSinger Dataset Tools 工程格式规范

> 版本 2.0.0 — 2026-05-02

---

## 1 时间精度约定

### 1.1 内部表示

所有时间值在运行时以 **`int64_t` 微秒**（μs）存储。

```cpp
using TimePos = int64_t;  // 微秒，1 秒 = 1'000'000
```

- 精度：1 μs（0.000001 秒），足够覆盖 192 kHz 采样率下的单采样精度（~5.2 μs）
- 范围：±9.2 × 10¹² μs ≈ ±106 天，远超任何单次录音长度
- 运算：整数加减乘除，**无浮点累积误差**

### 1.2 文件存储

在 JSON 文件（.dsproj、.dstext）中，时间值以**整数微秒**存储：

```json
{ "position": 1200000 }   // 表示 1.200000 秒
```

### 1.3 导入/导出转换

与外部格式交互时进行转换：

| 外部格式 | 存储精度 | 导入转换 | 导出转换 |
|---------|---------|---------|---------|
| MakeDiffSinger CSV | 6 位小数秒 | `round(sec × 1e6)` → `int64` | `int64 / 1e6` → `"%.6f"` |
| TextGrid | 双精度秒 | `round(sec × 1e6)` → `int64` | `int64 / 1e6` → `double` |
| .ds JSON | 6 位小数字符串 | 同 CSV | 同 CSV |
| 界面显示 | 用户可切换 | — | 秒：`"%.6f"` / 毫秒：`"%.3f"` |

### 1.4 比较与容差

两个时间点的相等判断：**直接整数比较，无容差**。

```cpp
bool equal(TimePos a, TimePos b) { return a == b; }
```

容差判断仅用于跨层绑定发现（见 §3.4）和外部格式导入（浮点→整数四舍五入后即精确）。

---

## 2 工程目录结构

```
myproject/
    myproject.dsproj
    .gitignore
    audio/
        a54c548a.wav
        38db2292.wav
    items/
        a54c548a_GuangNianZhiWai/
            527a23a9_GanShouTingZai.dstext
            88d4562a_RuHeShunJianDongJie.dstext
        50deb91b_WoHuaiNianDe/
            c8e1203f_NiDeXiaoRong.dstext
    dictionaries/
        fe67_CHN/
            dict.txt
            phoneset.txt
    dstemp/
        preprocess/
        slices/
        contexts/
        snapshots/
        export/
```

### 2.1 audio/

存储源音频文件。文件名为对应 item 的编号加扩展名。此目录由用户管理，应纳入版本控制。

### 2.2 items/

存储各 item 的标注文件（.dstext）。每个 item 对应一个子目录 `{itemId}_{itemName}`。

### 2.3 dictionaries/

每种语言一个子目录 `{languageId}_{languageName}`，包含：
- `dict.txt`：字典文件，格式 `音节\t音素1 [音素2]`
- `phoneset.txt`：音素表，空格分隔

### 2.4 dstemp/

中间文件，不纳入版本控制。详见 [task-processor-design.md §13](task-processor-design.md)。

| 子目录 | 内容 |
|--------|------|
| `preprocess/` | 预处理后的音频 |
| `slices/` | 切片音频（{prefix}_{NNN}.wav） |
| `contexts/` | 每个切片的 PipelineContext JSON |
| `snapshots/` | 自动步骤快照（用于撤销） |
| `export/` | 导出结果（CSV、DS 文件等） |

---

## 3 工程描述文件（.dsproj）

JSON 格式，扩展名 `.dsproj`。

### 3.1 顶层结构

```json
{
    "version": "2.0.0",
    "name": "Example Singing Dataset",
    "schema": [...],
    "tasks": [...],
    "defaults": {...},
    "languages": [...],
    "speakers": [...],
    "items": [...]
}
```

### 3.2 schema — 标注层结构定义

```json
"schema": [
    {
        "type": "text",
        "name": "lyrics",
        "category": "sentence"
    },
    {
        "type": "text",
        "name": "pinyin",
        "category": "grapheme",
        "subdivisionOf": 0
    },
    {
        "type": "note",
        "name": "midi",
        "category": "midi",
        "subdivisionOf": 1
    },
    {
        "type": "text",
        "name": "phone",
        "category": "phoneme",
        "subdivisionOf": 1
    },
    {
        "type": "text",
        "name": "slur",
        "category": "custom",
        "alignWith": 3
    }
]
```

每个 Layer 定义：

| 字段 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `type` | `string` | 是 | 数据类型：`"text"`（任意字符串）、`"note"`（音名/MIDI编号）、`"curve"`（采样点序列，用于 F0 等） |
| `name` | `string` | 是 | 层名称，显示和标识用 |
| `category` | `string` | 否 | 层类别，作为任务绑定和 PipelineContext 路由的键 |
| `subdivisionOf` | `integer` | 否 | 细分关系。本层是目标层的细分（如 phoneme 是 grapheme 的细分） |
| `alignWith` | `integer` | 否 | 对齐关系。本层与目标层一对一头尾对齐 |

**与 TextGrid 层的对应关系**：

| schema 层 | category | TextGrid tier | 说明 |
|-----------|----------|--------------|------|
| lyrics | `sentence` | `sentences` | 整句歌词。3-tier TextGrid 中用于句级切分，mark 为空表示丢弃该段 |
| pinyin | `grapheme` | `words` | 音节/发音序列（如拼音） |
| phone | `phoneme` | `phones` | 音素序列 |
| midi | `midi` | — | MIDI 音符（TextGrid 中无对应 tier） |
| slur | `custom` | — | 连音标记 |

2-tier TextGrid（`words` + `phones`）对应切片级标注。3-tier TextGrid（`sentences` + `words` + `phones`）对应整首歌级标注，用于 MakeDiffSinger 的 combine_tg / slice_tg 手动精修工作流。

**层间关系语义**：

| 关系 | 含义 | 绑定行为 |
|------|------|---------|
| `subdivisionOf` | 子层是父层的细分。子层的每个区间完全落在父层某个区间内。 | 父层的第一个子边界 = 父层边界本身。移动父层边界时，对应的子层首边界跟随移动。 |
| `alignWith` | 两层完全一对一对齐。 | 两层边界数量相同，位置相同。移动一层的边界时，另一层对应边界跟随移动。 |

### 3.3 tasks — 任务声明

```json
"tasks": [
    {
        "name": "phoneme_alignment",
        "inputs": [
            {"slot": "grapheme", "category": "grapheme"}
        ],
        "outputs": [
            {"slot": "phoneme", "category": "phoneme"}
        ],
        "granularity": "per_slice",
        "importFormats": ["textgrid"],
        "exportFormats": ["textgrid"]
    }
]
```

| 字段 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `name` | `string` | 是 | 任务名称 |
| `inputs` | `array<Slot>` | 是 | 输入槽位 |
| `outputs` | `array<Slot>` | 是 | 输出槽位 |
| `granularity` | `string` | 否 | `"whole_audio"` 或 `"per_slice"`（默认） |
| `optional` | `boolean` | 否 | 可选步骤（默认 false） |
| `manual` | `boolean` | 否 | 人工步骤（默认 false） |
| `importFormats` | `array<string>` | 否 | 该步骤支持导入的外部格式 |
| `exportFormats` | `array<string>` | 否 | 该步骤支持导出的外部格式 |

### 3.4 defaults — 工程级默认配置

```json
"defaults": {
    "preprocessors": [
        {"id": "loudness_norm", "targetLufs": -23.0}
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
        "asr":                {"processor": "funasr",    "path": "models/asr/funasr_cn.onnx",      "provider": "dml"},
        "phoneme_alignment":  {"processor": "hubert-fa", "path": "models/alignment/hubert_fa.onnx", "provider": "dml"},
        "midi_transcription": {"processor": "game",      "path": "models/game/",                    "provider": "dml"},
        "pitch_extraction":   {"processor": "rmvpe",     "path": "models/rmvpe/rmvpe.onnx",         "provider": "cpu"}
    },
    "export": {
        "formats": ["csv", "ds"],
        "dsHopSize": 512,
        "dsSampleRate": 44100,
        "includeDiscarded": false
    }
}
```

### 3.5 items — 项目与切片

```json
"items": [
    {
        "id": "a54c548a",
        "name": "GuangNianZhiWai",
        "speaker": "32e9",
        "language": "fe67",
        "audioSource": "audio/a54c548a.wav",
        "slices": [
            {"id": "001", "in": 1140000, "out": 5140000, "status": "active"},
            {"id": "002", "in": 8170000, "out": 19260000, "status": "active"},
            {"id": "003", "in": 22500000, "out": 23100000, "status": "discarded",
             "discardReason": "Too short (0.6s)", "discardedAtStep": "audio_slice"}
        ]
    }
]
```

Slice 字段：

| 字段 | 类型 | 必需 | 说明 |
|------|------|------|------|
| `id` | `string` | 是 | 切片编号（序号或随机 ID） |
| `in` | `integer` | 是 | 入点（微秒） |
| `out` | `integer` | 是 | 出点（微秒） |
| `language` | `string` | 否 | 覆盖 item 的语言设置 |
| `status` | `string` | 否 | `"active"`（默认）/ `"discarded"` / `"error"` |
| `discardReason` | `string` | 否 | 丢弃原因 |
| `discardedAtStep` | `string` | 否 | 在哪步被丢弃 |

---

## 4 标注文件（.dstext）

JSON 格式，扩展名 `.dstext`，每个文件对应一个切片的标注数据。

### 4.1 完整示例

```json
{
    "version": "2.0.0",
    "audio": {
        "path": "../../audio/a54c548a.wav",
        "in": 1140000,
        "out": 5140000
    },
    "layers": [
        {
            "name": "lyrics",
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
            "boundaries": [
                {"id": 6,  "pos": 0,       "text": "gan"},
                {"id": 7,  "pos": 600000,  "text": "shou"},
                {"id": 8,  "pos": 1200000, "text": "ting"},
                {"id": 9,  "pos": 1800000, "text": "zai"},
                {"id": 10, "pos": 2400000, "text": "wo"},
                {"id": 11, "pos": 3000000, "text": "fa"},
                {"id": 12, "pos": 3350000, "text": "duan"},
                {"id": 13, "pos": 3700000, "text": "de"},
                {"id": 14, "pos": 4000000, "text": ""}
            ]
        },
        {
            "name": "midi",
            "boundaries": [
                {"id": 15, "pos": 0,       "text": "C4"},
                {"id": 16, "pos": 600000,  "text": "D4"},
                {"id": 17, "pos": 1200000, "text": "E4"},
                {"id": 18, "pos": 1800000, "text": "D4"},
                {"id": 19, "pos": 2400000, "text": "C4"},
                {"id": 20, "pos": 3000000, "text": "E4"},
                {"id": 21, "pos": 3350000, "text": "F4"},
                {"id": 22, "pos": 3700000, "text": "E4"},
                {"id": 23, "pos": 4000000, "text": ""}
            ]
        },
        {
            "name": "phone",
            "boundaries": [
                {"id": 24, "pos": 0,       "text": "g"},
                {"id": 25, "pos": 80000,   "text": "an"},
                {"id": 26, "pos": 600000,  "text": "sh"},
                {"id": 27, "pos": 720000,  "text": "ou"},
                {"id": 28, "pos": 1200000, "text": "t"},
                {"id": 29, "pos": 1300000, "text": "ing"},
                {"id": 30, "pos": 1800000, "text": "z"},
                {"id": 31, "pos": 1880000, "text": "ai"},
                {"id": 32, "pos": 2400000, "text": "w"},
                {"id": 33, "pos": 2500000, "text": "o"},
                {"id": 34, "pos": 3000000, "text": "f"},
                {"id": 35, "pos": 3060000, "text": "a"},
                {"id": 36, "pos": 3350000, "text": "d"},
                {"id": 37, "pos": 3450000, "text": "uan"},
                {"id": 38, "pos": 3700000, "text": "d"},
                {"id": 39, "pos": 3750000, "text": "e"},
                {"id": 40, "pos": 4000000, "text": ""}
            ]
        },
        {
            "name": "slur",
            "boundaries": [
                {"id": 41, "pos": 0,       "text": "0"},
                {"id": 42, "pos": 600000,  "text": "0"},
                {"id": 43, "pos": 1200000, "text": "0"},
                {"id": 44, "pos": 1800000, "text": "0"},
                {"id": 45, "pos": 2400000, "text": "0"},
                {"id": 46, "pos": 3000000, "text": "0"},
                {"id": 47, "pos": 3350000, "text": "0"},
                {"id": 48, "pos": 3700000, "text": "0"},
                {"id": 49, "pos": 4000000, "text": ""}
            ]
        }
    ],
    "groups": [
        [1, 6, 15, 24, 41],
        [7, 16, 26, 42],
        [2, 8, 17, 28, 43],
        [9, 18, 30, 44],
        [3, 10, 19, 32, 45],
        [4, 11, 20, 34, 46],
        [12, 21, 36, 47],
        [13, 22, 38, 48],
        [5, 14, 23, 40, 49]
    ]
}
```

### 4.2 audio

| 字段 | 类型 | 说明 |
|------|------|------|
| `path` | `string` | 音频文件的相对路径（相对于 .dstext 所在目录） |
| `in` | `integer` | 切片起始时间点（微秒） |
| `out` | `integer` | 切片结束时间点（微秒） |

### 4.3 layers

每个 Layer：

| 字段 | 类型 | 说明 |
|------|------|------|
| `name` | `string` | 层名称，与 schema 定义一致 |
| `boundaries` | `array<Boundary>` | 切割标记点，允许乱序存储 |

每个 Boundary：

| 字段 | 类型 | 说明 |
|------|------|------|
| `id` | `integer` | 文件内唯一编号。自创建起不变。新切点的 id 必须大于文件中已有最大 id。 |
| `pos` | `integer` | 切割时间点（**微秒**） |
| `text` | `string` | 该切点右侧区间的内容 |

**自动边界**：
- 最左边界 `pos` 恒为 0。若不存在或被移走，自动生成。
- 最右边界 `pos` = 切片长度（`out - in`），`text` 恒为空。

### 4.4 groups — 跨层边界绑定

- 类型：`array<array<integer>>`
- 每个子数组包含若干切点的 `id`。这些切点构成一个**绑定组**。

**绑定语义**：移动绑定组中的任一切点时，组内所有其他切点**跟随移动相同的偏移量（delta）**。

**与 schema 层间关系的配合**：

schema 中的 `subdivisionOf` 和 `alignWith` 声明了**结构约束**，groups 声明了**编辑约束**。两者互补：

```
schema 声明：phone subdivisionOf pinyin
    → 结构约束：phone 的每个区间完全落在 pinyin 的某个区间内

groups 声明：[8, 28] — pinyin 的第 3 个边界 和 phone 的第 5 个边界绑定
    → 编辑约束：移动 pinyin 的 "ting" 起点时，phone 的 "t" 起点跟随移动
```

**groups 的自动推导**：

当模型或算法输出新的层数据时，PipelineRunner 根据 schema 的层间关系自动生成 groups：

1. 对 `subdivisionOf` 关系：子层中每个与父层边界位置相同的切点，与父层对应切点绑定。
2. 对 `alignWith` 关系：两层的所有对应切点一一绑定。

用户可在编辑器中手动解绑或重新绑定。

**规则**：
- 子数组中不存在的 id 将被忽略。
- 同一切点出现在多个组中，按读取顺序归入最后一个组。
- 同一层的两个切点不应出现在同一组中（无意义：它们已在同一条时间轴上）。

### 4.5 curve 层类型

用于 F0 等连续数据：

```json
{
    "name": "f0",
    "type": "curve",
    "timestep": 11610,
    "values": [0, 261600, 293700, ...]
}
```

| 字段 | 类型 | 说明 |
|------|------|------|
| `timestep` | `integer` | 采样间隔（微秒），如 `hop_size / sample_rate × 1e6` |
| `values` | `array<integer>` | 采样值。对 F0：毫赫兹（mHz），`261.6 Hz → 261600`。0 = 无声帧。 |

curve 层不使用 boundaries 和 groups。

---

## 5 设计说明

### 5.1 为什么用整数微秒

| 方案 | 问题 |
|------|------|
| `double` 秒 | 累积运算有浮点误差。`0.1 + 0.2 ≠ 0.3`。边界绑定比较时需要容差，容差值难以选定。 |
| `int64` 纳秒 | 精度过剩（音频采样率 192 kHz 下一个采样 ≈ 5208 ns），数值过大不易人读。 |
| `int64` 微秒 | 精度足够（1 μs < 1 个 192 kHz 采样），数值合理（1 秒 = 1,000,000），**整数运算无累积误差**。 |

**关键收益**：跨层绑定时，两个切点是否"在同一位置"变成 `a == b` 的整数比较，不再需要容差。

### 5.2 界面显示

界面提供显示模式切换：

| 模式 | 格式 | 示例 |
|------|------|------|
| 秒 | `%.6f s` | `1.200000 s` |
| 毫秒 | `%.3f ms` | `1200.000 ms` |

用户切换仅影响显示，不影响存储精度。

### 5.3 MakeDiffSinger 兼容

MDS 的 `ph_dur` 等字段使用 6 位小数秒字符串（如 `"0.080000"`）。

导入：`round(parseDouble(str) × 1e6)` → `int64`。

导出：`int64 / 1e6` → `snprintf("%.6f")`。

由于 MDS 的精度恰好是微秒级（6 位小数 = 10⁻⁶），转换是**无损的**。

### 5.4 跨层绑定的运行时行为

`BoundaryBindingManager`（已有实现）的运行时行为：

1. 用户拖动切点 A（层 i，边界 j）到新位置 `newPos`
2. 计算偏移量 `delta = newPos - oldPos`
3. 查找 A 所在的绑定组 → 得到绑定切点列表 `[B, C, D, ...]`
4. 创建 `LinkedMoveBoundaryCommand`：
   - A → `newPos`
   - B → `B.pos + delta`
   - C → `C.pos + delta`
   - ...
5. 推入 QUndoStack

**约束检查**：移动后如果违反了 `subdivisionOf` 约束（子层切点跑出父层区间），阻止移动并显示提示。

---

## 6 版本迁移

### 6.1 v1 → v2 迁移

| 变更 | v1 | v2 | 迁移 |
|------|----|----|------|
| 时间值类型 | `number`（浮点秒） | `integer`（微秒） | `round(sec × 1e6)` |
| 字段名 | `position` | `pos` | 重命名 |
| slice.in/out | 浮点秒 | 整数微秒 | 同上 |
| audio.in/out | 浮点秒 | 整数微秒 | 同上 |
| curve 层 | 不存在 | 新增 | 无需迁移 |
| slice.status | 不存在 | 新增 | 默认 `"active"` |
| version | `"1.0.0"` | `"2.0.0"` | 更新 |

读取 v1 文件时自动检测 version 字段并执行迁移。

---

## 关联文档

- [task-processor-design.md](task-processor-design.md) — 流水线架构设计
- [framework-architecture.md](framework-architecture.md) — 框架架构
- [framework-getting-started.md](framework-getting-started.md) — 框架入门
- [migration-guide.md](migration-guide.md) — 迁移指南
