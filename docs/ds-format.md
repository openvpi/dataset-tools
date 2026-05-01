# DiffSinger Dataset Tools 工程格式规范

## 1 工程目录结构

每个工程包含一个根目录及其内部所有被纳入管理的文件。典型的工程目录结构如下：

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
        38db2292_ZheGanJue/
            7a2b0e91_YiQieDou.dstext
    dictionaries/
        fe67_CHN/
            dict.txt
            phoneset.txt
        a61c_JPN/
            dict.txt
            phoneset.txt
    dstemp/
        slices/
        asr/
        alignment/
        csv/
        midi/
        ds/
        f0/
```

### 1.1 audio/

存储源音频文件。文件名为对应 item 的编号加扩展名（如 `a54c548a.wav`）。此目录由用户管理，应纳入版本控制。

### 1.2 items/

存储各 item 的标注文件。每个 item 对应一个子目录，命名格式为 `{itemId}_{itemName}`。子目录内存放该 item 下所有切片的标注文件（`.dstext`）。此目录由用户管理，应纳入版本控制。

### 1.3 dictionaries/

存储各语言的字典与音素表。每种语言对应一个子目录，命名格式为 `{languageId}_{languageName}`。

- `dict.txt`：字典文件，每行一条音节到音素序列的映射规则，格式为 `音节\t音素1 音素2 ...`
- `phoneset.txt`：音素表，包含该语言所有音素记号，以空格分隔

### 1.4 dstemp/

存储处理流水线各步骤产生的中间文件和临时文件。此目录不应纳入版本控制，可随时安全删除并重新生成。每个子目录下除了实际产出文件外，还包含 `.dsitem` 元数据文件，用于记录每个源文件在该步骤中的处理信息（使用的模型、参数、输入输出路径、时间戳等）。各子目录与流水线步骤对应：

| 子目录 | 对应步骤 | 内容 |
|---|---|---|
| `slices/` | AudioSlicer 切片 | 切片后的音频片段（.wav） |
| `asr/` | LyricFA 语音识别 | 识别结果（.lab, .json） |
| `alignment/` | HubertFA 音素对齐 | 对齐结果（TextGrid） |
| `csv/` | 转写汇总 | transcriptions.csv |
| `midi/` | GAME 音高估计 | MIDI 转写结果 |
| `ds/` | DiffSinger 打包 | .ds 训练文件 |
| `f0/` | RMVPE F0 提取 | F0 缓存数据 |

### 1.5 .gitignore

推荐的 `.gitignore` 内容：

```gitignore
# 中间文件与临时文件
dstemp/

# 用户配置
*.dsconf
```

## 2 工程描述文件（.dsproj）

工程描述文件采用 JSON 格式，扩展名为 `.dsproj`。完整示例：

```json
{
    "version": "1.0.0",
    "name": "Example Singing Dataset",
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
            "category": "pitch",
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
    ],
    "tasks": [
        {
            "name": "phoneme_alignment",
            "inputs": [
                { "slot": "sentence", "category": "sentence" },
                { "slot": "grapheme", "category": "grapheme" }
            ],
            "outputs": [
                { "slot": "phoneme", "category": "phoneme" }
            ]
        },
        {
            "name": "pitch_analysis",
            "inputs": [
                { "slot": "grapheme", "category": "grapheme" }
            ],
            "outputs": [
                { "slot": "pitch", "category": "pitch" }
            ]
        }
    ],
    "languages": [
        {
            "id": "fe67",
            "name": "CHN",
            "dictionary": "dict.txt",
            "phoneset": "phoneset.txt"
        },
        {
            "id": "a61c",
            "name": "JPN",
            "dictionary": "dict.txt",
            "phoneset": "phoneset.txt"
        }
    ],
    "speakers": [
        { "id": "32e9", "name": "Opencpop" },
        { "id": "08cd", "name": "ZhiBin" }
    ],
    "items": [
        {
            "id": "a54c548a",
            "name": "GuangNianZhiWai",
            "speaker": "32e9",
            "language": "fe67",
            "virtualPath": "",
            "audioSource": "audio/a54c548a.wav",
            "slices": [
                { "id": "527a23a9", "in": 1.14, "out": 5.14 },
                { "id": "88d4562a", "in": 8.17, "out": 19.26, "language": "a61c" }
            ]
        },
        {
            "id": "50deb91b",
            "name": "WoHuaiNianDe",
            "speaker": "32e9",
            "language": "fe67",
            "virtualPath": "foo/bar",
            "audioSource": "audio/50deb91b.wav",
            "slices": [
                { "id": "c8e1203f", "in": 0.00, "out": 12.50 }
            ]
        },
        {
            "id": "61cfc89a",
            "name": "BuWeiXia",
            "speaker": "08cd",
            "language": "fe67",
            "virtualPath": "foo",
            "audioSource": "audio/61cfc89a.wav",
            "slices": [
                { "id": "3e9d12a7", "in": 0.50, "out": 8.30 }
            ]
        },
        {
            "id": "38db2292",
            "name": "ZheGanJue",
            "speaker": "08cd",
            "language": "fe67",
            "virtualPath": "foo/bar",
            "audioSource": "audio/38db2292.wav",
            "slices": [
                { "id": "7a2b0e91", "in": 2.00, "out": 10.75 }
            ]
        }
    ]
}
```

### 2.1 version

- 类型：`string`
- 工程描述文件格式的版本号，遵循语义化版本规则。

### 2.2 defaults

- 类型：`object`（可选）
- 工程级默认配置。目前包含 `models` 子字段，用于为流水线各步骤指定默认模型。示例：

```json
{
    "defaults": {
        "models": {
            "asr": {
                "name": "AsrModel",
                "version": "v1.2",
                "path": "models/asr/funasr_cn.onnx"
            },
            "alignment": {
                "name": "HubertFA",
                "version": "v2.0",
                "path": "models/alignment/hubert_fa.onnx"
            },
            "midi": {
                "name": "GAME",
                "version": "v1.0",
                "path": "models/game/"
            },
            "build_ds": {
                "name": "RMVPE",
                "version": "v1.0",
                "path": "models/rmvpe/rmvpe.onnx"
            }
        }
    }
}
```

此字段为可选字段，不含此字段的 .dsproj 文件仍然合法。各步骤的模型配置详见 08-project-format.md。

### 2.3 name

- 类型：`string`
- 工程名称，由用户设置并可修改。

### 2.4 schema

- 类型：`array<Layer>`
- 标注层结构定义。层的下标从 0 开始。每个层对象包含以下字段：

| 字段 | 类型 | 必需 | 说明 |
|---|---|---|---|
| `type` | `string` | 是 | 层的数据类型。`"text"` 接受任意字符串，适用于歌词、拼音、音素等文本内容。`"note"` 接受国际谱音名（如 `C4`、`F#5`）或 MIDI 编号（如 `60`）。 |
| `name` | `string` | 是 | 层的名称，用于显示和标识。 |
| `category` | `string` | 否 | 层的类别，用于标识该层在标注流程中的角色，并作为任务绑定的键。类别不限定固定集合，常见值包括 `sentence`、`grapheme`、`phoneme`、`pitch`、`custom` 等。 |
| `subdivisionOf` | `integer` | 否 | 细分关系，值为目标层的下标。表示本层是目标层中标记的细分（如音素层是音节层的细分）。 |
| `alignWith` | `integer` | 否 | 对齐关系，值为目标层的下标。表示本层与目标层完全一对一头尾对齐。 |

未来计划增加 `"curve"` 类型，用于 F0 等连续数据的标注。

### 2.5 tasks

- 类型：`array<Task>`
- 自动标注任务声明。每个任务定义 m 个输入层到 n 个输出层的映射关系。

每个 Task 对象包含以下字段：

| 字段 | 类型 | 必需 | 说明 |
|---|---|---|---|
| `name` | `string` | 是 | 任务名称。 |
| `inputs` | `array<Slot>` | 是 | 输入槽位列表。 |
| `outputs` | `array<Slot>` | 是 | 输出槽位列表。 |

每个 Slot 对象包含以下字段：

| 字段 | 类型 | 必需 | 说明 |
|---|---|---|---|
| `slot` | `string` | 是 | 槽位名称，标识该输入/输出在任务中的角色。 |
| `category` | `string` | 是 | 绑定到的层类别。运行时通过 category 匹配 schema 中的具体层。 |

schema 只描述"数据是什么"，tasks 只描述"需要什么数据"，两者通过 category 在运行时动态绑定，互不耦合。新增自动标注流程时无需修改 schema，只需添加新的 task 声明。

### 2.6 languages

- 类型：`array<Language>`
- 工程中定义的所有语言。

| 字段 | 类型 | 必需 | 说明 |
|---|---|---|---|
| `id` | `string` | 是 | 语言编号，4 位随机十六进制字符串，一旦确定不可变更。 |
| `name` | `string` | 是 | 语言名称，由用户设置并可修改。 |
| `dictionary` | `string` | 是 | 该语言的字典文件名，位于对应 dictionaries 子目录下。 |
| `phoneset` | `string` | 是 | 该语言的音素表文件名，位于对应 dictionaries 子目录下。 |

### 2.7 speakers

- 类型：`array<Speaker>`
- 工程中定义的所有说话人。

| 字段 | 类型 | 必需 | 说明 |
|---|---|---|---|
| `id` | `string` | 是 | 说话人编号，4 位随机十六进制字符串，一旦确定不可变更。 |
| `name` | `string` | 是 | 说话人名称，由用户设置并可修改。 |

### 2.8 items

- 类型：`array<Item>`
- 纳入工程管理的所有项目。不再使用 Placeholder 概念，空虚拟路径直接用空字符串或省略 `virtualPath` 表示。

每个 Item 对象包含以下字段：

| 字段 | 类型 | 必需 | 说明 |
|---|---|---|---|
| `id` | `string` | 是 | 项目编号，8 位随机十六进制字符串，一旦确定不可变更。 |
| `name` | `string` | 是 | 项目名称，由用户设置并可修改。 |
| `speaker` | `string` | 是 | 所属说话人的编号，引用 speakers 中的 id。 |
| `language` | `string` | 是 | 默认语言编号，引用 languages 中的 id。 |
| `virtualPath` | `string` | 否 | 在资源浏览器中的虚拟路径。空字符串或省略表示位于根级。 |
| `audioSource` | `string` | 是 | 音频源文件的相对路径（相对于工程根目录）。 |
| `slices` | `array<Slice>` | 是 | 该项目下的所有切片。 |

每个 Slice 对象包含以下字段：

| 字段 | 类型 | 必需 | 说明 |
|---|---|---|---|
| `id` | `string` | 是 | 切片编号，8 位随机十六进制字符串，一旦确定不可变更。 |
| `in` | `number` | 是 | 切片在源音频中的入点，单位为秒。 |
| `out` | `number` | 是 | 切片在源音频中的出点，单位为秒。 |
| `language` | `string` | 否 | 覆盖所属 item 的语言设置。引用 languages 中的 id。 |

切片的标注文件存储在 `items/{itemId}_{itemName}/` 目录下，文件名为 `{sliceId}_{sliceName}.dstext`。

## 3 标注文件（.dstext）

标注文件采用 JSON 格式，扩展名为 `.dstext`。每个文件对应一个切片的标注数据。完整示例：

```json
{
    "version": "1.0.0",
    "audio": {
        "path": "../../audio/a54c548a.wav",
        "in": 1.14,
        "out": 5.14
    },
    "layers": [
        {
            "name": "lyrics",
            "boundaries": [
                { "id": 1, "position": 0.0000, "text": "感受" },
                { "id": 2, "position": 1.2000, "text": "停在" },
                { "id": 3, "position": 2.4000, "text": "我" },
                { "id": 4, "position": 3.0000, "text": "发端的" },
                { "id": 5, "position": 4.0000, "text": "" }
            ]
        },
        {
            "name": "pinyin",
            "boundaries": [
                { "id": 6, "position": 0.0000, "text": "gan" },
                { "id": 7, "position": 0.6000, "text": "shou" },
                { "id": 8, "position": 1.2000, "text": "ting" },
                { "id": 9, "position": 1.8000, "text": "zai" },
                { "id": 10, "position": 2.4000, "text": "wo" },
                { "id": 11, "position": 3.0000, "text": "fa" },
                { "id": 12, "position": 3.3500, "text": "duan" },
                { "id": 13, "position": 3.7000, "text": "de" },
                { "id": 14, "position": 4.0000, "text": "" }
            ]
        },
        {
            "name": "midi",
            "boundaries": [
                { "id": 15, "position": 0.0000, "text": "C4" },
                { "id": 16, "position": 0.6000, "text": "D4" },
                { "id": 17, "position": 1.2000, "text": "E4" },
                { "id": 18, "position": 1.8000, "text": "D4" },
                { "id": 19, "position": 2.4000, "text": "C4" },
                { "id": 20, "position": 3.0000, "text": "E4" },
                { "id": 21, "position": 3.3500, "text": "F4" },
                { "id": 22, "position": 3.7000, "text": "E4" },
                { "id": 23, "position": 4.0000, "text": "" }
            ]
        },
        {
            "name": "phone",
            "boundaries": [
                { "id": 24, "position": 0.0000, "text": "g" },
                { "id": 25, "position": 0.0800, "text": "an" },
                { "id": 26, "position": 0.6000, "text": "sh" },
                { "id": 27, "position": 0.7200, "text": "ou" },
                { "id": 28, "position": 1.2000, "text": "t" },
                { "id": 29, "position": 1.3000, "text": "ing" },
                { "id": 30, "position": 1.8000, "text": "z" },
                { "id": 31, "position": 1.8800, "text": "ai" },
                { "id": 32, "position": 2.4000, "text": "w" },
                { "id": 33, "position": 2.5000, "text": "o" },
                { "id": 34, "position": 3.0000, "text": "f" },
                { "id": 35, "position": 3.0600, "text": "a" },
                { "id": 36, "position": 3.3500, "text": "d" },
                { "id": 37, "position": 3.4500, "text": "uan" },
                { "id": 38, "position": 3.7000, "text": "d" },
                { "id": 39, "position": 3.7500, "text": "e" },
                { "id": 40, "position": 4.0000, "text": "" }
            ]
        },
        {
            "name": "slur",
            "boundaries": [
                { "id": 41, "position": 0.0000, "text": "0" },
                { "id": 42, "position": 0.6000, "text": "0" },
                { "id": 43, "position": 1.2000, "text": "0" },
                { "id": 44, "position": 1.8000, "text": "0" },
                { "id": 45, "position": 2.4000, "text": "0" },
                { "id": 46, "position": 3.0000, "text": "0" },
                { "id": 47, "position": 3.3500, "text": "0" },
                { "id": 48, "position": 3.7000, "text": "0" },
                { "id": 49, "position": 4.0000, "text": "" }
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

### 3.1 version

- 类型：`string`
- 标注文件格式的版本号。

### 3.2 audio

- 类型：`object`
- 对所属音频文件的反向引用，便于脱离工程独立编辑标注文件。

| 字段 | 类型 | 说明 |
|---|---|---|
| `path` | `string` | 音频文件的相对路径（相对于 .dstext 文件所在目录）。只接受本地文件系统路径。 |
| `in` | `number` | 切片起始时间点，单位为秒。 |
| `out` | `number` | 切片结束时间点，单位为秒。 |

标注内容超出 `in`/`out` 范围的部分在读取时将被截断。左边界早于首个切割点时，在 position 为 0 处补充空白标记。

### 3.3 layers

- 类型：`array<Layer>`
- 所有标注层，类比 Praat TextGrid 中的 Tier。层的顺序和名称应与工程描述文件 schema 定义一致。

每个 Layer 对象包含以下字段：

| 字段 | 类型 | 说明 |
|---|---|---|
| `name` | `string` | 层名称。 |
| `boundaries` | `array<Boundary>` | 该层中所有切割标记点。允许乱序存储。 |

每个 Boundary 对象包含以下字段：

| 字段 | 类型 | 说明 |
|---|---|---|
| `id` | `integer` | 切点在该文件中的唯一编号。自创建起不变，直到切点被删除。新切点的 id 必须大于文件中已有的最大 id。 |
| `position` | `number` | 切割时间点，单位为秒。 |
| `text` | `string` | 该切割点右侧区间的内容。对 `text` 类型层为任意字符串，对 `note` 类型层为国际谱音名或 MIDI 编号。 |

每层的最左边界 position 恒为 0。若最左边界不为 0 或被向右移动，将在 position 为 0 处自动生成新边界。每层最右边界（position 等于切片长度）的 text 恒为空且不可修改。若最右边界不存在或被向左移动，将在正确位置自动生成新边界。

id 的用途包括：切点间的绑定关系（groups），以及便于版本控制系统追踪单个切点的变动。

### 3.4 groups

- 类型：`array<array<integer>>`
- 切点绑定组。移动绑定组中的一个切点时，组内其他切点跟随移动相同的量。

每个子数组包含若干切点的 id。子数组中的非整型值或不对应任何切点的 id 将被忽略。若同一切点出现在多个组中，按读取顺序将其从原组中移除并归入新组。

## 4 设计说明

### 4.1 JSON 格式

项目 C++ 代码已依赖 nlohmann/json，所有运行时数据结构可直接序列化/反序列化。JSON 格式也与 DiffSinger 的 .ds 训练文件保持一致，减少格式转换开销。相比 XML，JSON 更紧凑、解析更快，且在现代工具链中有更好的生态支持。

### 4.2 dstemp/ 分离

将所有中间文件集中存放在 `dstemp/` 目录下，带来几个好处：

- 用户数据（audio/、items/）保持干净，不会被处理流程产生的临时文件污染
- 一行 `.gitignore` 规则即可排除全部中间产物
- 需要清理空间或重跑流水线时，删除 `dstemp/` 即可，不会误删用户标注

各子目录按流水线步骤划分，方便定位特定阶段的输出，也允许单独清理某个步骤的缓存。

### 4.3 取消 .lvitem 独立文件

旧设计中每个 item 有一个单独的 `.lvitem` 描述文件，存储 item 的属性和切片列表。新设计将这些信息直接内联到工程文件的 `items` 数组中，简化目录结构，减少文件数量，也避免了工程文件与 item 文件之间的同步问题。

### 4.4 .dsitem 处理记录

dstemp/ 下的每个步骤子目录中引入了 `.dsitem` 文件，按源文件粒度记录处理元数据。每个 .dsitem 文件是一个 JSON 文档，包含使用的模型、参数、输入输出文件列表、时间戳和处理状态。

这种设计的好处：

- 完整的可追溯性：任何产出文件都能追溯到生成它的模型和参数
- 增量处理：通过比较 .dsitem 时间戳与输入文件修改时间，判断是否需要重跑
- 多模型支持：不同文件可以使用不同模型，.dsitem 记录实际选择
- 人工步骤追踪：编辑步骤（Step 3, 5, 9）的 .dsitem 记录用户是否已审核

.dsitem 的完整格式规范和示例见 08-project-format.md。

### 4.5 未来扩展

- `"curve"` 层类型：用于 F0 曲线等连续数据标注，boundary 模型不适用于此类数据，需要定义新的采样点序列结构
- 新的 task 类型：只需在 `tasks` 数组中添加声明，schema 无需改动
- 多语言混合标注：切片级别的 language 覆盖机制已预留
