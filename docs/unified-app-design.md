# 统一应用设计方案 — LabelSuite + DsLabeler

> 版本 2.0.0 — 2026-05-02
>
> 将所有独立应用合并为两个可执行文件：
> - **LabelSuite** — 通用音频标注工具集，各页面独立工作，不绑定 DiffSinger
> - **DsLabeler** — DiffSinger 专用标注器，使用 .dsproj 工程文件，流程化数据集制作

---

## 1 动机与目标

### 1.1 现状问题

- 6 个独立 exe，共享大量动态库，重复部署体积大
- DiffSingerLabeler 的 9 步 wizard 将各应用功能内嵌拷贝，维护双份代码
- MinLabel、PhonemeLabeler、PitchLabeler 本身是通用工具，不应绑定 DiffSinger
- 制作 DiffSinger 数据集时需在多个应用间切换，工作流割裂

### 1.2 目标

1. **LabelSuite**：通用标注工具集，各页面接受各自格式的文件（.lab、TextGrid、.ds 等），可独立用于任何音频标注项目
2. **DsLabeler**：DiffSinger 数据集制作专用，统一工程文件驱动，隐去可自动执行的步骤
3. 删除所有原独立 exe

---

## 2 LabelSuite — 通用标注工具集

### 2.1 定位

LabelSuite 与 DsLabeler **底层完全统一**，使用相同的 dstext/PipelineContext 数据模型和相同的页面组件代码。区别：

- **无 .dsproj 工程文件管理**（File 菜单无 Open/Save Project）
- **保留全部 10 个页面**（含 Align、MIDI 等 DsLabeler 中被合并/自动化的步骤的独立界面）
- **数据源为 FileDataSource**：以工作目录为根，各页面通过 FormatAdapter 导入/导出旧版格式（TextGrid、.lab、.ds、CSV）
- **享受自动补全**：进入 Phone 页时若有 grapheme 层则自动 FA，进入 Pitch 页时自动 add_ph_num/F0/MIDI（与 DsLabeler 行为一致）

适用场景：
- DiffSinger 数据集制作（不需要工程文件管理时）
- 任何需要标注歌词/音素/音高的音频项目
- 第三方强制对齐工具（MFA、SOFA）的结果修正
- 独立使用某个标注/处理功能
- 用户希望显式控制每个步骤（如单独运行 FA、单独生成 CSV）

### 2.2 页面布局

```
LabelSuite (AppShell, 多页面模式)
├── Slice        (AudioSlicer)       ← 音频切片
├── ASR          (LyricFA)           ← ASR 歌词识别
├── Label        (MinLabel)          ← 歌词标注 + G2P
├── Align        (HubertFA)          ← HuBERT 强制对齐（独立界面）
├── Phone        (PhonemeLabeler)    ← TextGrid 音素编辑（+自动 FA 补全）
├── CSV          (BuildCsv)          ← 生成 transcriptions.csv
├── MIDI         (GameAlign)         ← GAME MIDI 转录（独立界面）
├── DS           (BuildDs)           ← 生成 .ds 文件（含 RMVPE 音高提取）
├── Pitch        (PitchLabeler)      ← F0/MIDI 编辑（+自动补全）
├── Settings     (Settings)          ← 模型路径、词典等配置
```

侧边栏 10 个图标。Settings 页与 DsLabeler 共享同一组件，但配置持久化到 `dsfw::AppSettings`（而非 .dsproj）。

### 2.3 与 DsLabeler 的关系

LabelSuite 与 DsLabeler **共享全部页面组件和底层数据模型（dstext/PipelineContext）**。两者的区别：

| | LabelSuite | DsLabeler |
|---|---|---|
| 底层数据模型 | ✅ dstext/PipelineContext（统一） | ✅ dstext/PipelineContext（统一） |
| 核心编辑器 (MinLabel/Phoneme/Pitch Editor) | ✅ 共享 | ✅ 共享 |
| Settings 页（模型配置） | ✅ 有（持久化到 AppSettings） | ✅ 有（持久化到 .dsproj） |
| 自动补全（进入页面自动 FA/F0/MIDI） | ✅ 有 | ✅ 有 |
| Align / MIDI / CSV / DS 独立页面 | ✅ 保留（可显式控制每步） | ❌ 合并到自动流程 |
| .dsproj 工程文件管理 | ❌ 无 | ✅ 有 |
| Welcome 页 / Export 页 | ❌ 无 | ✅ 有 |
| 文件格式兼容 | ✅ FormatAdapter 导入导出旧格式 | 仅 dstext |
| 页面数 | 10 | 7 |

### 2.4 数据流与旧格式兼容

LabelSuite 的 `FileDataSource` 内部使用 `DsTextDocument`（dstext）作为统一数据模型，通过 FormatAdapter 实现旧格式的导入/导出：

```
导入（打开文件时）：
  .TextGrid ──→ TextGridAdapter::import() ──→ DsTextDocument（内存 dstext）
  .lab      ──→ LabAdapter::import()      ──→ DsTextDocument
  .ds       ──→ DsFileAdapter::import()   ──→ DsTextDocument

编辑：
  DsTextDocument ──→ PhonemeEditor / PitchEditor / MinLabelEditor（共享 UI）

导出（保存时）：
  DsTextDocument ──→ TextGridAdapter::export() ──→ .TextGrid 文件
  DsTextDocument ──→ LabAdapter::export()      ──→ .lab 文件
  DsTextDocument ──→ DsFileAdapter::export()   ──→ .ds 文件
  DsTextDocument ──→ CsvAdapter::export()      ──→ transcriptions.csv
```

用户无需感知内部数据模型的变化。打开旧版文件 → 自动转入 dstext → 编辑 → 保存回原格式。

### 2.5 LabelSuite 的自动补全行为

LabelSuite 享受与 DsLabeler 相同的自动补全机制，但用户也可通过独立页面手动执行：

| 进入页面 | 自动补全行为 | 独立页面（手动控制） |
|---------|------------|-------------------|
| Phone（音素编辑） | 若有 grapheme 层且无 phoneme 层 → 自动 FA | Align 页可单独运行 HubertFA |
| Pitch（音高编辑） | 自动 add_ph_num + F0 + MIDI（缺失时） | MIDI 页可单独运行 GAME |
| CSV 页 | — | 可显式生成 transcriptions.csv |
| DS 页 | — | 可显式生成 .ds 文件 |

### 2.6 File 菜单

```
文件(F)
├── Set Working Directory...
├── ─────────────
├── Clean Working Directory...
├── ─────────────
└── Exit
```

---

## 3 DsLabeler — DiffSinger 专用标注器

### 3.1 定位

DsLabeler 是 DiffSinger 数据集制作的完整工具。使用 `.dsproj` 工程文件驱动，打开即恢复上次进度。页面之间共享工程数据（PipelineContext），自动执行可省略的步骤。

### 3.2 页面布局

```
DsLabeler (AppShell, 多页面模式)
├── 🏠 欢迎/创建工程 (Welcome)       ← 新建工程 / 打开已有工程
├── ✂️ 切片 (Slicer)                 ← 自动切片 + 手动切片 + 参数内嵌 + 切片导出
├── 🏷 歌词标注 (MinLabel)            ← MinLabel + ASR/LyricFA 按钮
├── 📐 音素标注 (PhonemeLabeler)      ← PhonemeLabeler + 自动 FA
├── 🎵 音高标注 (PitchLabeler)        ← PitchLabeler + 自动 add_ph_num/F0/MIDI
├── 📦 导出 (Export)                  ← CSV + ds/ + wavs/ 输出
├── ⚙ 设置 (Settings)                ← 模型路径、词典等高级配置
```

侧边栏 7 个图标。打开工程前仅显示欢迎页，其余页面灰显不可用。

> **设计决策**：切片参数不多，直接内嵌在切片页面，不需要单独的设置页 tab。Settings 页移至末尾，仅用于模型路径、词典、导出等需要独立配置的项。

### 3.3 欢迎/创建工程页 (Welcome)

```
┌─────────────────────────────────────────────────────┐
│                                                      │
│              DsLabeler                               │
│              DiffSinger 数据集标注工具                 │
│                                                      │
│  ┌────────────────┐    ┌────────────────┐            │
│  │  📁 新建工程    │    │  📂 打开工程    │            │
│  └────────────────┘    └────────────────┘            │
│                                                      │
│  ── 最近工程 ─────────────────────────────────────   │
│  📄 GuangNianZhiWai.dsproj    2026-05-01  D:\data\  │
│  📄 WoHuaiNianDe.dsproj      2026-04-28  D:\data\  │
│  📄 LemonTree.dsproj         2026-04-25  E:\ds\    │
│                                                      │
└─────────────────────────────────────────────────────┘
```

**新建工程向导**（模态对话框）：

1. **基本信息**：工程保存位置、名称、语言、说话人

> **设计决策**：创建工程时**不需要添加音频文件**。音频文件在切片页面中打开和处理。切片导出后，切片自动成为工程的 item。

向导完成后创建空的 `.dsproj`，自动跳转到**切片页面**。

**打开工程**：选择 `.dsproj` 文件，加载工程数据，根据 `completedSteps` 恢复进度。

### 3.4 切片页 (Slicer)

切片页面是 DsLabeler 的核心预处理步骤，提供自动切片、手动切片、切片导出功能。**切片参数直接内嵌在此页面**，无需跳转设置页。

#### 架构关系

**切片页面本质上是单层的、不带音素名而是带数字序号的 PhonemeLabeler**。底层复用共享的 `WaveformPanel` 组件（见 §11），与 PhonemeLabeler 共享波形/频谱渲染和播放交互。额外功能仅为：导出切片音频（支持 WAV 格式选项）。

#### 界面布局

```
┌──────────────────────────────────────────────────────────────────────────┐
│  ┌─ 左侧栏：音频文件列表 ──┐  ┌─ 右侧内容区 ──────────────────────────┐│
│  │ (支持拖拽文件/文件夹)     │  │                                       ││
│  │ file1.wav                │  │  ┌─ 切片参数（内嵌面板） ──────────┐   ││
│  │ file2.wav                │  │  │ Threshold / Min Length / etc.   │   ││
│  │ file3.flac               │  │  │ [导入] [自动切片] [重新] [导出]  │   ││
│  │                          │  │  └─────────────────────────────────┘   ││
│  │ [+] [📁] [−]            │  │                                       ││
│  └──────────────────────────┘  │  ┌─ 波形图 + Mel频谱 ──────────────┐  ││
│                                │  │  (左键添加切点，Delete删除)       │  ││
│                                │  └──────────────────────────────────┘  ││
│                                │                                       ││
│                                │  ┌─ 切片列表（下方面板） ──────────┐  ││
│                                │  │  右键菜单：添加切点/删除边界/    │  ││
│                                │  │  丢弃/恢复切片                   │  ││
│                                │  └──────────────────────────────────┘  ││
│                                └───────────────────────────────────────┘│
└──────────────────────────────────────────────────────────────────────────┘
```

#### 音频文件管理

切片页面左侧为音频文件列表面板 (`AudioFileListPanel`)，支持以下方式添加音频：

| 方式 | 说明 |
|------|------|
| 菜单 文件 → 打开音频文件 | 选择一个或多个音频文件 |
| 菜单 文件 → 打开音频目录 | 选择目录，自动导入目录内所有音频文件 |
| 左侧面板 + 按钮 | 同上（打开文件选择对话框） |
| 左侧面板 📁 按钮 | 同上（打开目录选择对话框） |
| 拖拽 | 直接将文件或文件夹拖入左侧面板 |

支持的音频格式：`.wav`, `.mp3`, `.flac`, `.m4a`, `.ogg`, `.opus`, `.wma`, `.aac`

**自动切片**：导入音频时自动使用当前参数对所有新增音频运行切片算法，记录切点。

**切片参数**：采用紧凑单行布局，参数和按钮在同一行内排列。

**进度条**：文件列表底部进度条按已处理（有切点）的文件数自动更新。

#### 切点变更提醒

当修改已有导出项目的切点时（自动切片或导入切点），弹窗显示受影响的音频文件：
- 复选框选择需要重新切片的整首音频
- 警告已标注的歌词、音素、音高数据将丢失
- 确认后移除旧的切片和 dsitem 文件

#### AudioSlicer 可选输入层

AudioSlicer 的 I/O 契约（见 task-processor-design.md §3.1）定义了 **可选的 `slices` 输入层**：

- **有输入**（导入 au 切点文件 / 已有 slices 层）→ 按导入的切点位置切分音频。波形图显示导入的切割线，用户可手动微调。"自动切片"按钮变为"仅对选中片段自动切片"（用于处理单个过长片段）。
- **无输入** → 按 RMS 参数自动检测静音边界。结果显示在波形图上。

#### 交互行为

| 操作 | 行为 |
|------|------|
| 左键单击波形图空白处 | 在点击位置添加切割线 |
| 左键拖动切割线 | 移动切割线位置（QUndoStack） |
| Delete / 右键切割线 | 删除切割线 |
| **右键单击波形图区域** | **直接播放**点击位置左右两个切割线之间的片段（不弹出任何菜单） |
| Ctrl+滚轮 | 缩放波形图 |
| 滚轮 | 横向滚动 |
| 双击切片列表项 | 跳转到对应切片在波形图中的位置 |
| 右键切片列表项 | 弹出菜单：添加切点（中点）/ 删除左边界 / 删除右边界 / 丢弃 / 恢复 |

#### 手动切点编辑

除了在波形图上左键单击添加、Delete 键删除外，下方切片列表面板的右键菜单也提供手动切点操作：

| 菜单项 | 说明 |
|--------|------|
| 添加切点（中点） | 在当前片段的中点位置插入一个新切割线 |
| 删除左边界 | 删除当前片段左侧的切割线（合并与左邻片段） |
| 删除右边界 | 删除当前片段右侧的切割线（合并与右邻片段） |
| 丢弃切片 | 标记当前片段为已丢弃（导出时跳过） |
| 恢复切片 | 取消丢弃标记 |

所有切点操作通过 QUndoStack 支持 Ctrl+Z 撤销 / Ctrl+Shift+Z 重做。

#### 导出切片音频

点击"导出音频..."按钮，弹出导出对话框：

| 参数 | 选项 | 默认值 |
|------|------|--------|
| 格式 | WAV | WAV |
| 位深 | 16-bit PCM / 24-bit PCM / 32-bit PCM / 32-bit float | **16-bit PCM** |
| 声道 | 单声道 / 保持原始 | **单声道** |
| 输出目录 | 路径选择 | `dstemp/slices/` |
| 命名前缀 | 文本框 | 音频文件名词干 |
| 序号位数 | 1-10 | 3 |

> **全流程建议单声道**：DiffSinger 训练数据为单声道。切片导出默认单声道；后续所有步骤均以单声道数据工作。双声道音频的处理见 §11.3。

#### 切点文件格式

支持保存和读取 Audacity 标记格式（.au / .txt）的切点文件，兼容 main 分支 AudioSlicer 的 marker 功能。

#### 与 NewProjectDialog 的关系

创建工程时不添加音频文件，工程初始为空。用户在切片页面打开音频（通过菜单、按钮或拖拽），完成切片操作后导出：
- 创建 `dstemp/slices/*.wav`
- 为每个切片创建 PipelineContext JSON（`dstemp/contexts/*.json`）
- **将切片作为 Item 写入工程**（每个切片 = 一个 Item，含 Slice 信息）
- 保存工程文件

切片导出后，后续页面（歌词、音素、音高）的 item 列表即为切片列表。

### 3.5 设置页 (Settings)

> 注：Settings 已移至页面列表末尾（第 7 页），仅包含模型配置等高级选项。

横向 TabWidget，每个 tab 对应需要独立配置的项目。所有配置持久化到 `.dsproj` 的 `defaults` 字段。

| Tab | 配置项 | 对应 .dsproj defaults 路径 |
|-----|--------|---------------------------|
| **设备** | 全局推理提供者 (CPU/DML/CUDA暂禁用)、具体设备（名称+显存，过滤<1GB） | `defaults.globalProvider` + `defaults.deviceIndex` |
| **ASR** | ASR 模型路径、[Test] 按钮、☑ CPU 强制 | `defaults.models.asr` |
| **词典/G2P** | 各语言词典路径、音素表路径 | `languages[]` |
| **强制对齐** | FA 模型路径、[Test] 按钮、☑ CPU 强制、☑ 预加载 [N] 个文件 | `defaults.models.phoneme_alignment` + `defaults.preload` |
| **音高/MIDI** | F0/MIDI 模型路径、[Test] 按钮、☑ CPU 强制、☑ 预加载 [N] 个文件 | `defaults.models.pitch_extraction` + `defaults.models.midi_transcription` + `defaults.preload` |
| **预处理** | 预处理器列表（响度归一化、降噪）、参数 | `defaults.preprocessors` |

> **已移除**：导出 tab（导出参数直接在导出页面配置）和切片 tab（切片参数内嵌在切片页面）。

#### 设备选择

设备 tab 提供统一的推理后端设置：
- **推理提供者**：CPU / DirectML / CUDA（暂不可用，灰显禁用）
- **设备下拉**：通过 DXGI 枚举所有 GPU，显示设备名+显存大小，自动过滤显存<1GB 的设备
- 选择 CPU 时设备下拉禁用
- 各模型 tab 提供「CPU 强制」复选框，可覆盖全局设备设置

#### 模型配置

每个模型配置行包含：
- 模型路径 + [浏览...] + [Test] 按钮（加载第一个 .onnx 文件验证）
- ☑ CPU 强制（此模型在 CPU 上运行，覆盖全局设备设置）
- 设置变更后自动触发 `modelReloadRequested` 信号重新加载模型

#### 预加载机制

强制对齐 tab 和 音高/MIDI tab 提供"预加载"选项：

```
☑ 预加载    文件数: [10]
```

启用后，进入对应应用页时自动在后台：
- **强制对齐预加载**：对前 N 个未完成 FA 的切片执行 HuBERT-FA，渲染频谱/功率图
- **音高预加载**：对前 N 个未完成的切片执行 add_ph_num → RMVPE F0 提取 → GAME MIDI 转录，渲染 mel 图

预加载在后台线程执行，不阻塞 UI。结果写入 PipelineContext。

### 3.6 歌词标注页 (MinLabel)

#### 基础功能

继承 LabelSuite 的 MinLabel 页全部功能，但数据来源从文件系统切换为工程内切片列表。

#### 新增：ASR + LyricFA 按钮

在工具栏或页面顶部添加两个操作按钮：

| 按钮 | 功能 | 说明 |
|------|------|------|
| **ASR 识别** | 对当前 item 整首歌运行 FunASR，识别结果填入 sentence 层 | 使用配置页 ASR 模型 |
| **歌词匹配** | 将 ASR 结果与原歌词 txt 做最大匹配，分配到各切片 | 可选，需提供歌词文件 |

#### 菜单：批处理

菜单栏 → **处理(P)** → **批量 ASR...**

### 3.7 音素标注页 (PhonemeLabeler)

#### 基础功能

继承 LabelSuite 的 PhonemeLabeler 页全部功能，数据来源为工程内切片。

#### 交互变更（共享 WaveformPanel）

音素标注页使用共享的 `WaveformPanel` 组件（见 §11），交互行为统一：

- **取消播放工具栏 (PlayWidget toolbar)**：不再显示独立的音频播放工具栏。PlayWidget 实例保留但不显示 UI（仅作为播放后端）
- **右键单击标注区域任意位置**（波形图、频谱图、音素编辑区）：**直接播放**点击位置左右两个边界之间的片段，不弹出任何菜单
- 播放使用 `PlayWidget::setPlayRange() + seek() + setPlaying(true)` 实现

> **设计原则**：标注工作中最频繁的操作是"听一下当前片段"。两次点击（右键→菜单项→播放）降低效率。改为右键直接播放，零延迟反馈。

#### 自动执行 FA

打开一个切片时，如果该切片尚无 `phoneme` 层数据，或 `phoneme` 被标记为 dirty（因 grapheme 变更）：
1. 检查是否有 `grapheme` 层（来自歌词标注页的 G2P 输出）
2. 如有，自动使用配置页的 FA 模型执行强制对齐
3. 结果直接填入 phoneme 层，用户可立即修正
4. 如果是 dirty 重算，弹出 Toast："已重新执行强制对齐（歌词发音已修改）"，3 秒自动消失

#### 脏数据处理

用户在此页编辑音素边界或音素名称后保存时，系统自动将该切片的 `ph_num` 和 `midi` 标记为 dirty。不影响其他切片。

#### 菜单：批量预处理

菜单栏 → **处理(P)** → **批量强制对齐...**

#### 可跳过

用户可完全跳过此页。导出时自动补全 FA + add_ph_num。

### 3.8 音高标注页 (PitchLabeler)

#### 基础功能

继承 LabelSuite 的 PitchLabeler 页全部功能，数据来源为工程内切片。

#### 自动前置处理

打开一个切片时，自动执行以下前置步骤（如果数据缺失**或标记为 dirty**）：

| 步骤 | 触发条件 | 操作 |
|------|---------|------|
| add_ph_num | 缺少 `ph_num` 层，或 `ph_num` 在 dirty 列表中 | 自动计算（纯算法，瞬时） |
| F0 提取 | 缺少 `pitch` 层 | 使用配置页模型（默认 RMVPE） |
| MIDI 转录 | 缺少 `midi` 层，或 `midi` 在 dirty 列表中 | 使用配置页模型（默认 GAME） |

**注意**：F0 提取不受音素变更影响（F0 仅依赖音频），因此 phoneme 修改不会触发 pitch 重算。

重算完成后弹出 Toast 通知（仅当有 dirty 重算时）：
> "已重新计算 ph_num 和 MIDI（音素边界已修改）"

Toast 使用 `dsfw::ToastNotification`（Phase J 已实现），3 秒自动消失，不阻塞编辑。

#### 菜单：批处理

菜单栏 → **处理(P)** → **批量提取音高 + MIDI...**

#### 可跳过

用户可完全跳过此页。导出时：
- 自动补全 add_ph_num + GAME MIDI
- **不生成 ds/ 文件夹**（未校正的 F0/MIDI 不写入 .ds）

### 3.9 导出页 (Export)

导出页直接包含所有导出参数（不在设置页配置），包括导出内容、音频设置、高级选项。

#### 界面设计

```
┌─────────────────────────────────────────────────────┐
│  导出 DiffSinger 数据集                               │
├─────────────────────────────────────────────────────┤
│                                                      │
│  目标文件夹: [____________________________] [浏览]    │
│                                                      │
│  ── 导出内容 ──────────────────────────────────────   │
│  ☑ transcriptions.csv                                │
│  ☑ ds/ 文件夹（.ds 训练文件）                          │
│  ☑ wavs/ 文件夹                                      │
│                                                      │
│  ── 音频设置 ──────────────────────────────────────   │
│  采样率:  [44100] Hz                                  │
│  ☑ 需要时重采样（使用 soxr）                           │
│                                                      │
│  ── 高级 ─────────────────────────────────────────   │
│  hop_size: [512]                                     │
│  ☑ 包含丢弃的切片                                     │
│                                                      │
│              [开始导出]                                │
│                                                      │
│  进度: ████████████░░░░░░░░ 60%  (120/200)           │
│  状态: 正在重采样音频...                               │
└─────────────────────────────────────────────────────┘
```

#### 导出目标结构

```
<export_dir>/
    transcriptions.csv          ← 所有 active 切片的声学数据
    ds/                         ← 仅当用户经过 PitchLabeler 时生成
        guangnian_001.ds
        guangnian_002.ds
    wavs/
        guangnian_001.wav       ← 可能经过重采样
        guangnian_002.wav
```

#### 导出前置校验

导出前对每个 active 切片检查：

| 缺失 | 可自动补全？ | 条件 |
|------|------------|------|
| grapheme 层 | **否** | 必须先完成歌词标注。缺失时标红提示 |
| phoneme 层 | 是 | 需要 grapheme 层存在 → 自动 FA |
| ph_num 层 | 是 | 需要 phoneme + grapheme → 自动计算 |
| midi 层 | 是 | 需要 phoneme + ph_num → 自动 GAME |
| pitch 层 | 仅当需要 ds/ | 需要音频 → 自动 RMVPE |

**grapheme 层是唯一的硬性前提**。如果任何 active 切片缺少 grapheme 层，导出按钮禁用并提示"请先完成歌词标注"。

#### 导出逻辑（补全 + 输出）

```
导出流程:
    1. 遍历所有 active 切片
    2. 对每个切片检查并补全:
       ├─ 缺 phoneme 层? → 自动执行 FA（需要 grapheme 层）
       ├─ 缺 ph_num 层?  → 自动计算 add_ph_num
       ├─ 缺 midi 层?    → 自动执行 GAME 转录
       └─ 需要重采样?    → 使用 soxr 重采样到目标采样率
    3. 生成 transcriptions.csv (CsvAdapter)
    4. 如果用户经过了 PitchLabeler（pitch 层有手动编辑数据）:
       ├─ 生成 ds/ 文件夹
       └─ 每个切片一个 .ds 文件 (DsFileAdapter)
    5. 复制/重采样音频到 wavs/
```

#### 跳过场景汇总

| 跳过的步骤 | 导出时自动补全 | 输出内容 |
|-----------|--------------|---------|
| 无跳过（完整流程） | 无 | CSV + ds/ + wavs/ |
| 跳过 PhonemeLabeler | 自动 FA + add_ph_num + 可能的重采样 | CSV + ds/（如有 pitch）+ wavs/ |
| 跳过 PitchLabeler | 自动 add_ph_num + GAME MIDI + 可能的重采样 | CSV + wavs/（**无 ds/**） |
| 跳过 PhonemeLabeler + PitchLabeler | 自动 FA + add_ph_num + GAME MIDI + 可能的重采样 | CSV + wavs/（**无 ds/**） |

**关键规则**：
- `ds/` 仅当切片的 `meta.editedSteps` 包含 `"pitch_review"` 时才为该切片生成 .ds 文件（per-slice 判定，非全局）
- 部分切片有 .ds、部分没有是正常情况
- CSV 始终生成，包含声学数据

### 3.10 main.cpp

```cpp
#include <QApplication>
#include <dsfw/AppShell.h>
#include <dsfw/Theme.h>

#include "WelcomePage.h"
#include "DsSlicerPage.h"
#include "DsMinLabelPage.h"
#include "DsPhonemeLabelerPage.h"
#include "DsPitchLabelerPage.h"
#include "ExportPage.h"
#include "SettingsPage.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("DsLabeler");

    dsfw::Theme::instance().init(app);

    dsfw::AppShell shell;
    shell.setWindowTitle("DsLabeler — DiffSinger Dataset Labeler");

    shell.addPage(new WelcomePage(&shell),
                  "welcome", QIcon(":/icons/home.svg"), "工程");
    shell.addPage(new DsSlicerPage(&shell),
                  "slicer", QIcon(":/icons/slice.svg"), "切片");
    shell.addPage(new DsMinLabelPage(&shell),
                  "minlabel", QIcon(":/icons/label.svg"), "歌词");
    shell.addPage(new DsPhonemeLabelerPage(&shell),
                  "phoneme", QIcon(":/icons/phoneme.svg"), "音素");
    shell.addPage(new DsPitchLabelerPage(&shell),
                  "pitch", QIcon(":/icons/pitch.svg"), "音高");
    shell.addPage(new ExportPage(&shell),
                  "export", QIcon(":/icons/export.svg"), "导出");
    shell.addPage(new SettingsPage(&shell),
                  "settings", QIcon(":/icons/settings.svg"), "设置");

    shell.resize(1400, 900);
    shell.show();

    return app.exec();
}
```

### 3.11 CMake

```cmake
dstools_add_executable(DsLabeler)
target_link_libraries(DsLabeler PRIVATE
    dstools-widgets
    dstools-domain
    dstools-audio
    audio-util
    hubert-infer
    game-infer
    rmvpe-infer
    FunAsr
    cpp-pinyin
    cpp-kana
    textgrid
    FFTW3::fftw3
    SndFile::sndfile
    nlohmann_json::nlohmann_json
    Qt::Concurrent
)
```

---

## 4 页面复用架构 — 高度统一

### 4.1 统一数据模型

**核心决策（ADR-66）**：LabelSuite 和 DsLabeler 底层使用**完全相同的数据模型** — dstext/PipelineContext。两者共享同一套页面组件代码。区别仅在数据源（`FileDataSource` vs `ProjectDataSource`）和应用壳层（页面选择、菜单、配置持久化）。

原先 LabelSuite 直接操作 TextGrid/.lab/.ds 文件，现在改为：
1. 导入时通过 FormatAdapter 转为 DsTextDocument（内存 dstext）
2. 编辑时使用统一的 dstext 数据模型
3. 保存时通过 FormatAdapter 导出回原格式

### 4.2 IEditorDataSource 统一接口

```cpp
class IEditorDataSource : public QObject {
public:
    virtual QStringList sliceIds() const = 0;
    virtual Result<DsTextDocument> loadSlice(const QString &sliceId) = 0;
    virtual Result<void> saveSlice(const QString &sliceId, const DsTextDocument &doc) = 0;
    virtual QString audioPath(const QString &sliceId) const = 0;
};
```

| 数据源 | 用途 | 导入 | 导出 |
|--------|------|------|------|
| `FileDataSource` | LabelSuite | FormatAdapter → dstext | dstext → FormatAdapter → 原格式 |
| `ProjectDataSource` | DsLabeler | .dsproj + PipelineContext → dstext | dstext → PipelineContext |

### 4.3 页面统一

LabelSuite 和 DsLabeler **使用完全相同的页面组件类**。每个页面接收 `IEditorDataSource*` 注入，不感知数据来源。

| 页面组件 | LabelSuite 页面 | DsLabeler 页面 | 说明 |
|----------|-----------------|----------------|------|
| SlicerPage | ✅ Slice | ✅ Slicer | 共享切片逻辑，DsLabeler 额外绑定工程 |
| LyricFAPage | ✅ ASR | — | LabelSuite 独有（DsLabeler 内嵌到 MinLabel） |
| MinLabelPage | ✅ Label | ✅ 歌词 | 共享编辑器，DsLabeler 额外显示 ASR 按钮 |
| HubertFAPage | ✅ Align | — | LabelSuite 独有（DsLabeler 自动 FA） |
| PhonemeLabelerPage | ✅ Phone | ✅ 音素 | 共享编辑器 + 自动 FA 补全 |
| BuildCsvPage | ✅ CSV | — | LabelSuite 独有（DsLabeler 合并到导出） |
| GameAlignPage | ✅ MIDI | — | LabelSuite 独有（DsLabeler 自动 MIDI） |
| BuildDsPage | ✅ DS | — | LabelSuite 独有（DsLabeler 合并到导出） |
| PitchLabelerPage | ✅ Pitch | ✅ 音高 | 共享编辑器 + 自动补全 |
| SettingsPage | ✅ Settings | ✅ 设置 | 共享 UI；LabelSuite 持久化到 AppSettings |
| WelcomePage | — | ✅ 工程 | DsLabeler 独有 |
| ExportPage | — | ✅ 导出 | DsLabeler 独有 |

### 4.4 自动补全行为（两个应用统一）

进入特定页面时，若前置数据缺失（或标记 dirty），自动补全。此行为在 LabelSuite 和 DsLabeler 中完全一致：

| 进入页面 | 触发条件 | 自动执行 |
|---------|---------|---------|
| Phone（音素编辑） | 有 grapheme 层、无 phoneme 层或 dirty | 自动 HubertFA |
| Pitch（音高编辑） | 缺 ph_num / pitch / midi 或 dirty | 自动 add_ph_num + RMVPE + GAME |

LabelSuite 的区别：用户仍可通过 Align/MIDI 独立页面手动控制这些步骤。

### 4.5 共享编辑器组件

```
PhonemeEditor（共享 UI 组件，已存在）
├── WaveformWidget       ← 波形渲染
├── SpectrogramWidget    ← 频谱渲染（懒加载 FFT）
├── PowerWidget          ← 功率曲线
├── TierEditWidget       ← 多层 TextGrid 编辑
├── EntryListPanel       ← 条目列表
├── TimeRulerWidget      ← 时间刻度尺
├── BoundaryOverlayWidget
└── TextGridDocument     ← ★ 文档模型（内存中的 TextGrid）
```

PhonemeEditor 内部使用 `TextGridDocument` 作为统一编辑模型。页面层负责 dstext ↔ TextGridDocument 转换（已由 `TextGridDocument::loadFromDsText()` / `toDsText()` 实现）。

#### 需要实现的转换接口

```cpp
/// TextGridDocument 扩展 — dstext 互转
class TextGridDocument {
public:
    void loadFromDsText(const QList<IntervalLayer> &layers, TimePos duration);
    QList<IntervalLayer> toDsText() const;
};
```

| 组件 | 位置 | 说明 |
|------|------|------|
| TextGridDocument | `src/apps/shared/phoneme-editor/ui/` | 内存 TextGrid 模型，支持 .TextGrid 文件 I/O 和 dstext 互转 |
| FileDataSource | `src/apps/shared/data-sources/` | 文件系统数据源 + FormatAdapter（LabelSuite 用） |
| ProjectDataSource | `src/apps/ds-labeler/` | 工程 + PipelineContext 实现（DsLabeler 用） |
| PhonemeEditor | `src/apps/shared/phoneme-editor/` | 核心 TextGrid 编辑逻辑，波形/频谱 |
| PitchEditor | `src/apps/shared/pitch-editor/` | 核心 F0/MIDI 编辑逻辑，钢琴卷帘 |
| WaveformPanel | `src/apps/shared/waveform-panel/` | 波形+时间轴+滚动，切片页和音素页共享 |
| SettingsWidget | `src/apps/shared/settings/` | 模型配置 UI，两个应用共享 |

### 4.6 FileDataSource 的 FormatAdapter 实现

`FileDataSource` 改造为内部使用 dstext，通过 FormatAdapter 做旧格式转换：

```cpp
class FileDataSource : public IEditorDataSource {
public:
    /// 加载文件（自动检测格式，转为 dstext）
    Result<DsTextDocument> loadSlice(const QString &sliceId) override {
        // 1. 根据文件扩展名选择 FormatAdapter
        // 2. adapter->import(filePath) → DsTextDocument
        // 3. 返回 DsTextDocument
    }

    /// 保存文件（从 dstext 导出为原格式）
    Result<void> saveSlice(const QString &sliceId, const DsTextDocument &doc) override {
        // 1. 根据目标格式选择 FormatAdapter
        // 2. adapter->export(doc, filePath)
    }

    /// 支持的导入格式
    void setImportFormat(FormatType type); // Auto / TextGrid / Lab / Ds

    /// 支持的导出格式（默认与导入格式一致）
    void setExportFormat(FormatType type);
};
```

已有的 FormatAdapter 基础设施（Phase L）：`TextGridAdapter`、`DsFileAdapter`、`CsvAdapter`、`LabAdapter`。

### 4.7 Settings 页共享

Settings 页面 UI 完全共享。差异仅在配置持久化后端：

| | LabelSuite | DsLabeler |
|---|---|---|
| Settings UI | ✅ 共享 SettingsWidget | ✅ 共享 SettingsWidget |
| 持久化 | `dsfw::AppSettings`（QSettings） | `.dsproj` defaults 字段 |
| 接口 | `ISettingsBackend::load/save()` | `ISettingsBackend::load/save()` |

```cpp
class ISettingsBackend {
public:
    virtual QJsonObject load() = 0;
    virtual void save(const QJsonObject &settings) = 0;
};

class AppSettingsBackend : public ISettingsBackend { /* QSettings */ };
class ProjectSettingsBackend : public ISettingsBackend { /* .dsproj */ };
```

---

## 5 被废弃的独立应用

| 原应用 | 去向 |
|--------|------|
| DatasetPipeline | AudioSlicer → DsLabeler 创建工程流程；LyricFA → DsLabeler MinLabel 页 ASR 按钮；HubertFA → DsLabeler PhonemeLabeler 页自动 FA |
| MinLabel | → LabelSuite MinLabel 页 / DsLabeler 歌词标注页 |
| PhonemeLabeler | → LabelSuite PhonemeLabeler 页 / DsLabeler 音素标注页 |
| PitchLabeler | → LabelSuite PitchLabeler 页 / DsLabeler 音高标注页 |
| GameInfer | GAME 推理嵌入 DsLabeler PitchLabeler 自动流程 |
| DiffSingerLabeler | 被 DsLabeler 整体替代 |

保留 `dstools-cli.exe`（CLI 工具）和 `WidgetGallery.exe`（开发调试用）。

---

## 6 DsLabeler 数据流

### 6.1 BuildCsv 步骤省略

原 DiffSingerLabeler 的"制作声学数据集"步骤被省略。各步骤产出以层数据形式暂存在 PipelineContext（`dstemp/contexts/*.json`）。导出 CSV 时按需自动执行 add_ph_num，通过 CsvAdapter 生成。

### 6.2 完整数据流

```
[欢迎页] ─── 新建工程：设置名称/位置/语言 → 创建空 .dsproj（无音频）
  │
  ▼
[切片页] ─── 打开音频文件（菜单/拖拽/按钮）→ 自动切片 + 手动切片 + 导出
  │         ├─ 左侧面板：音频文件列表（支持拖拽文件/目录）
  │         ├─ 参数内嵌（无需设置页）
  │         ├─ 右键切片列表：手动增删切点
  │         ├─ 可选输入层（导入切点文件）
  │         ├─ 右键播放片段
  │         └─ 导出时切片写入工程 items
  │
  ▼
[歌词标注页] ─── ASR(可选) → G2P → grapheme 层 → dstext
  │
  ▼ (可跳过)                    ┌──────────────────────┐
[音素标注页] ─── 自动FA → 手动修正 → phoneme 层 → dstext  │
  │              右键直接播放      │  修改后 → dirty:     │
  ▼ (可跳过)                    │  ph_num, midi        │
[音高标注页] ─── 自动 add_ph_num + F0 + MIDI → 手动修正   │
  │              ↑ dirty 时自动重算                      │
  │              └──────────────────────────────────────┘
  ▼
[导出页] ─── 补全缺失步骤 + 清除所有 dirty → CSV + ds/(可选) + wavs/
  │
  ▼ (低频操作)
[设置页] ─── 模型路径、词典、导出参数、预加载
```

用户可在任意页面之间**自由跳转**。回退到上游页面修改数据后，下游自动标记 dirty，下次进入下游页面时自动重算并 Toast 提示。详见 [ds-format.md §6](ds-format.md)。

### 6.3 从 DiffSingerLabeler 迁移

| DiffSingerLabeler 步骤 | DsLabeler 对应 |
|------------------------|-------------|
| Step 1: Slice | 切片页 |
| Step 2: ASR | 歌词标注页 ASR 按钮 |
| Step 3: Label | 歌词标注页 |
| Step 4: Align (HubertFA) | 音素标注页自动 FA |
| Step 5: Phone | 音素标注页 |
| Step 6: BuildCsv (MakeDS) | **省略** — 数据暂存 PipelineContext |
| Step 7: MIDI (GAME) | 音高标注页自动 MIDI |
| Step 8: BuildDs | **省略** — 合并到导出页 |
| Step 9: Pitch | 音高标注页 |
| (无) | 导出页 |

---

## 7 DsLabeler 菜单结构

每个应用页有自己的菜单栏（AppShell 自动切换）：

### 7.1 切片页

```
文件(F)   处理(P)   帮助(H)
├─ 打开音频文件...
├─ 打开音频目录...
├─ ─────────────
└─ 退出

          ├─ 自动切片
          ├─ 重新切片
          ├─ ─────────────
          ├─ 导入切点...
          ├─ 保存切点...
          ├─ ─────────────
          └─ 导出切片音频...
```

### 7.2 歌词标注页

```
文件(F)   编辑(E)   处理(P)   视图(V)   帮助(H)
                     ├─ ASR 识别当前曲目
                     ├─ 歌词匹配
                     ├─ ─────────────
                     └─ 批量 ASR...
```

### 7.3 音素标注页

```
文件(F)   编辑(E)   处理(P)   视图(V)   帮助(H)
                     ├─ 强制对齐当前切片
                     ├─ ─────────────
                     └─ 批量强制对齐...
```

### 7.4 音高标注页

```
文件(F)   编辑(E)   处理(P)   视图(V)   帮助(H)
                     ├─ 提取音高 (当前切片)
                     ├─ 提取 MIDI (当前切片)
                     ├─ ─────────────
                     └─ 批量提取音高 + MIDI...
```

---

## 8 与 MakeDiffSinger 的兼容

DsLabeler 导出页生成的目录结构完全兼容 MakeDiffSinger：

```
<export_dir>/
    transcriptions.csv      ← 与 MDS build_dataset.py 输出格式一致
    ds/                     ← 与 MDS convert_ds.py csv2ds 输出格式一致
        *.ds
    wavs/                   ← 与 MDS 目录约定一致
        *.wav
```

---

## 9 配置持久化

DsLabeler 所有配置存储在 `.dsproj` 文件中：

```json
{
    "version": "2.0.0",
    "name": "My Dataset",
    "defaults": {
        "slicer": { ... },
        "models": {
            "asr": {"processor": "funasr", "path": "...", "provider": "dml"},
            "phoneme_alignment": {"processor": "hubert-fa", "path": "...", "provider": "dml"},
            "pitch_extraction": {"processor": "rmvpe", "path": "...", "provider": "cpu"},
            "midi_transcription": {"processor": "game", "path": "...", "provider": "dml"}
        },
        "export": {
            "formats": ["csv", "ds"],
            "dsHopSize": 512,
            "dsSampleRate": 44100,
            "resampleRate": 44100,
            "includeDiscarded": false
        },
        "preload": {
            "phoneme_alignment": {"enabled": true, "count": 10},
            "pitch_extraction": {"enabled": true, "count": 10}
        }
    },
    "languages": [...],
    "items": [...]
}
```

LabelSuite 使用 `dsfw::AppSettings` 存储用户偏好（窗口大小、最近文件等），不使用工程文件。

---

## 10 ADR 补充

| ADR | 决策 | 理由 |
|-----|------|------|
| ADR-46 | 分为 LabelSuite（通用）+ DsLabeler（专用）两个 exe | 通用标注工具不应绑定 DiffSinger；DiffSinger 工作流需要工程文件驱动 |
| ADR-47 | 省略 BuildCsv 步骤，数据暂存 PipelineContext | CSV 是导出格式而非中间格式，按需生成 |
| ADR-48 | PitchLabeler 自动执行 add_ph_num | add_ph_num 是纯算法无需用户干预 |
| ADR-49 | 导出时按需补全缺失步骤 | 允许用户跳过手动步骤，降低使用门槛 |
| ADR-50 | 无 PitchLabeler 编辑则不生成 ds/ | 未校正的 F0/MIDI 不适合直接用于训练 |
| ADR-51 | 预加载机制异步后台执行 | 消除切换切片时的等待，提升体验 |
| ADR-52 | 核心编辑器组件共享，IEditorDataSource 抽象 | 避免 LabelSuite 和 DsLabeler 维护双份编辑器代码 |
| ADR-53 | 层依赖 dirty 标记 + 页面切换时自动重算 | per-slice 粒度，不阻塞编辑，Toast 通知用户 |
| ADR-54 | 切片页 = 单层 PhonemeLabeler + 导出 | 最大化复用 WaveformPanel，减少代码重复 |
| ADR-55 | 右键直接播放，不弹菜单 | 标注中最频繁操作应零延迟；波形/标注区域不触发右键菜单 |
| ADR-56 | 全流程单声道，双声道可切换显示 | DiffSinger 训练数据为单声道；保留双声道显示能力供验证 |
| ADR-57 | Settings 移至末尾 | 切片参数内嵌切片页；模型配置低频操作放末尾不占核心位置 |
| ADR-58 | 创建工程不添加音频，切片导出后生成 items | 切片是工程的第一步，音频来源在切片页管理而非工程创建时 |
| ADR-59 | DroppableFileListPanel 为框架层通用控件 | 文件列表拖拽、过滤、进度条是多处复用的通用需求 |
| ADR-60 | PathUtils 统一跨平台路径编码 | Windows 中文路径需要 wchar，macOS 需 NFC；所有文件 I/O 走统一入口 |
| ADR-66 | LabelSuite 底层统一使用 dstext/PipelineContext | 消除 LabelSuite 与 DsLabeler 的数据模型分裂；旧格式通过 FormatAdapter 兼容 |
| ADR-67 | LabelSuite 增加 Settings 页 | 统一模型配置体验；共享 SettingsWidget，持久化到 AppSettings |
| ADR-68 | LabelSuite 享受自动补全 | 进入 Phone/Pitch 页自动 FA/F0/MIDI，与 DsLabeler 一致；同时保留独立页面供手动控制 |
| ADR-69 | ISettingsBackend 抽象持久化 | Settings UI 共享，后端可切换 QSettings / .dsproj |
| ADR-70 | FileDataSource 内部使用 dstext + FormatAdapter | 打开旧格式文件时自动转入 dstext，保存时转回原格式；用户无感知 |

---

## 11 WaveformPanel 共享组件

### 11.1 设计动机

切片页面和音素标注页面共享大量 UI 组件：波形图、时间刻度尺、频谱图、边界线、播放逻辑。为避免代码重复，提取 `WaveformPanel` 作为共享基础组件。

### 11.2 组件层次

```
WaveformPanel (src/apps/shared/waveform-panel/)
├── TimeRulerWidget          ← 波形图上方的时间刻度尺
├── WaveformWidget           ← 波形渲染 + 边界线叠加 + 拖动编辑
├── MelSpectrogramWidget     ← 波形图下方的 Mel 频谱图（可折叠）
├── ViewportController       ← 缩放/滚动状态同步
└── PlaybackController       ← 右键播放逻辑（无 UI 播放条）
```

#### 与 PhonemeEditor 的关系

```
PhonemeEditor（音素标注）
├── WaveformPanel            ← 共享组件
│   ├── TimeRulerWidget
│   ├── WaveformWidget
│   └── MelSpectrogramWidget (可选)
├── TierEditWidget           ← 多层 TextGrid 编辑（带音素名）
├── PowerWidget              ← 功率图
├── SpectrogramWidget        ← 全频谱图（区别于 Mel）
└── EntryListPanel           ← 条目列表

DsSlicerPage（切片）
├── WaveformPanel            ← 共享组件
│   ├── TimeRulerWidget
│   ├── WaveformWidget
│   └── MelSpectrogramWidget（默认展开显示）
├── SliceNumberLayer         ← 单层序号标注（001, 002, 003...）
├── SliceListPanel           ← 切片列表 + 状态
└── SliceParamsPanel         ← 切片参数（紧凑单行） + 按钮
```

### 11.3 双声道处理策略

**全流程建议单声道**，但需要支持双声道音频的显示和验证：

| 场景 | 行为 |
|------|------|
| 加载双声道音频 | **默认显示为单声道**（左右声道混合），顶部显示提示条："⚠ 双声道音频，当前显示混合波形 [切换显示]" |
| 用户点击"切换显示" | 切换为双声道分离显示（上下叠放两个波形） |
| 切片导出 | 默认输出单声道（混合）；可选"保持原始"输出双声道 |
| 后续步骤（FA/F0/MIDI） | 始终使用单声道数据（自动混合），不受显示模式影响 |

#### 实现方式

```cpp
/// WaveformWidget 扩展
class WaveformWidget {
    // ...
    enum class ChannelMode { Mono, StereoSplit };
    void setChannelMode(ChannelMode mode);

    // 内部始终保存原始多声道数据
    std::vector<std::vector<float>> m_channelSamples; // [channel][sample]
    std::vector<float> m_monoMix;                      // 混合后的单声道

    // 提供给处理流水线的始终是 mono
    const std::vector<float>& monoSamples() const { return m_monoMix; }
};
```

### 11.4 统一右键播放行为

**所有使用 WaveformPanel 的页面统一交互**：

```
右键单击波形图/频谱图/标注区域任意位置：
  1. 确定点击时间 t
  2. 找到 t 左右最近的两条边界线 (segStart, segEnd)
  3. 直接调用 PlayWidget::setPlayRange(segStart, segEnd) + seek + play
  4. 不弹出任何上下文菜单（波形区域 contextMenuEvent 被拦截）
```

**PlayWidget**（音频播放后端）仍然存在，但**不显示为工具栏**。仅作为内部播放控制器使用。停止播放通过再次右键单击（已在播放时）或 Escape 键。

### 11.5 时间刻度尺

波形图上方固定显示时间刻度尺（`TimeRulerWidget`，已在 PhonemeEditor 中实现）。刻度随缩放级别自适应：

- 缩放较小时：显示分:秒（0:00, 0:05, 0:10...）
- 缩放较大时：显示秒.毫秒（1.000, 1.100, 1.200...）

---

## 12 通用控件与框架层增强

### 12.1 DroppableFileListPanel（框架层通用控件）

位置：`src/framework/widgets/include/dsfw/widgets/DroppableFileListPanel.h`

可配置文件过滤、支持拖拽文件/目录、带底部进度条的通用文件列表面板。

```cpp
auto *panel = new dsfw::widgets::DroppableFileListPanel(this);
panel->setFileFilters({"*.wav", "*.flac"});  // 仅接受 wav 和 flac
panel->setShowProgress(true);
panel->progressTracker()->setFormat("%1 / %2 已标注 (%3%)");
```

| 特性 | 说明 |
|------|------|
| 文件过滤 | `setFileFilters({"*.wav", "*.flac"})` — 拖入和扫描目录时自动过滤 |
| 拖拽支持 | 接受文件和文件夹拖放，自动过滤不匹配的文件 |
| 按钮 | `+`（添加文件）、`📁`（添加目录）、`−`（移除选中） |
| 进度条 | 内置 `FileProgressTracker`，默认隐藏，`setShowProgress(true)` 启用 |
| 去重 | 自动避免相同路径重复添加 |

DsLabeler 中 `AudioFileListPanel` 继承此控件，仅设置音频格式过滤器。

### 12.2 PathSelector 拖拽过滤

`PathSelector` 在 OpenFile/SaveFile 模式下拖入文件时，现在会检查文件扩展名是否匹配配置的 filter 字符串。不匹配的文件会被拒绝（光标变为禁止图标）。

### 12.3 PathUtils（跨平台路径编码）

位置：`src/framework/base/include/dsfw/PathUtils.h`

统一解决不同操作系统下中文/日文路径编码问题：

| 平台 | 问题 | PathUtils 方案 |
|------|------|---------------|
| Windows | ANSI API（fopen/ifstream）无法处理中文路径 | `toStdPath()` → `std::filesystem::path`（wchar_t）; `openFile()` → `_wfopen` |
| macOS | HFS+ 使用 NFD 分解 Unicode | `normalize()` → NFC 规范化 |
| Linux | 文件名为 UTF-8 字节序列 | 直接使用 `toStdString()` |

**使用规范**：
- 所有底层文件 I/O 统一通过 `PathUtils::openFile()` 或 `PathUtils::toStdPath()`
- 与 C 库（sndfile、ffmpeg、onnxruntime）交互时使用 `PathUtils::toStdPath()`
- 路径比较前必须 `PathUtils::normalize()`
- 禁止直接使用 `path.toStdString()` 传给 Windows C API

### 12.4 SliceListPanel 标注进度条

DsLabeler 各标注页面的左侧 `SliceListPanel` 底部集成进度条，显示当前页面的标注完成数：

```
┌───────────────────┐
│ guangnian_001     │
│ guangnian_002  ✓  │
│ guangnian_003  ✓  │
│ guangnian_004     │
│ ...               │
├───────────────────┤
│ 2 / 50 已标注 (4%)│
└───────────────────┘
```

各页面通过 `sliceListPanel->setProgress(completed, total)` 更新进度。

---

## 关联文档

- [architecture.md](architecture.md) — 项目架构概述
- [task-processor-design.md](task-processor-design.md) — 流水线架构设计
- [ds-format.md](ds-format.md) — 工程格式规范
- [refactoring-roadmap.md](refactoring-roadmap.md) — 重构路线图
- [migration-guide.md](migration-guide.md) — AppShell 迁移指南

> 更新时间：2026-05-03 — §2/§4 重写：LabelSuite 底层统一使用 dstext/PipelineContext，通过 FormatAdapter 兼容旧格式；增加 Settings 页和自动补全；页面组件完全共享，消除双份代码
