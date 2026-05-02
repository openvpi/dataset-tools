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

LabelSuite 是一个多页面 AppShell 应用，将原 MinLabel、PhonemeLabeler、PitchLabeler 组装为独立页面。每个页面直接打开/保存各自的文件格式，互不依赖，无工程概念。

适用场景：
- 任何需要标注歌词/音素/音高的音频项目
- 第三方强制对齐工具（MFA、SOFA）的结果修正
- 独立使用某个标注功能，不需要完整的 DiffSinger 流水线

### 2.2 页面布局

```
LabelSuite (AppShell, 多页面模式)
├── 🏷 歌词标注 (MinLabel)          ← 打开/保存 .lab 文件，G2P 转换
├── 📐 音素标注 (PhonemeLabeler)    ← 打开/保存 TextGrid 文件，波形/频谱编辑
├── 🎵 音高标注 (PitchLabeler)      ← 打开/保存 .ds 文件，F0/MIDI 编辑
```

侧边栏 3 个图标，对应 3 个页面。

### 2.3 各页面功能

各页面**完全继承**原独立应用的功能，无裁剪无新增：

| 页面 | 来源 | 输入格式 | 输出格式 |
|------|------|---------|---------|
| MinLabel | `src/apps/MinLabel/` | 音频 + .lab | .lab |
| PhonemeLabeler | `src/apps/PhonemeLabeler/` | 音频 + TextGrid | TextGrid |
| PitchLabeler | `src/apps/PitchLabeler/` | .ds | .ds |

### 2.4 main.cpp

```cpp
#include <QApplication>
#include <dsfw/AppShell.h>
#include <dsfw/Theme.h>

#include "MinLabelPage.h"
#include "PhonemeLabelerPage.h"
#include "PitchLabelerPage.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("LabelSuite");

    dsfw::Theme::instance().init(app);

    dsfw::AppShell shell;
    shell.setWindowTitle("LabelSuite");

    shell.addPage(new MinLabelPage(&shell),
                  "minlabel", QIcon(":/icons/label.svg"), "歌词");
    shell.addPage(new PhonemeLabelerPage(&shell),
                  "phoneme", QIcon(":/icons/phoneme.svg"), "音素");
    shell.addPage(new PitchLabelerPage(&shell),
                  "pitch", QIcon(":/icons/pitch.svg"), "音高");

    shell.resize(1280, 800);
    shell.show();

    return app.exec();
}
```

### 2.5 CMake

```cmake
dstools_add_executable(LabelSuite)
target_link_libraries(LabelSuite PRIVATE
    dstools-widgets
    dstools-domain
    dstools-audio
    cpp-pinyin
    cpp-kana
    textgrid
    FFTW3::fftw3
    SndFile::sndfile
    nlohmann_json::nlohmann_json
)
```

---

## 3 DsLabeler — DiffSinger 专用标注器

### 3.1 定位

DsLabeler 是 DiffSinger 数据集制作的完整工具。使用 `.dsproj` 工程文件驱动，打开即恢复上次进度。页面之间共享工程数据（PipelineContext），自动执行可省略的步骤。

### 3.2 页面布局

```
DsLabeler (AppShell, 多页面模式)
├── 🏠 欢迎/创建工程 (Welcome)       ← 新建工程 / 打开已有工程
├── ⚙ 初始设置 (Settings)            ← 横向 TabWidget，按步骤分页配置
├── 🏷 歌词标注 (MinLabel)            ← MinLabel + ASR/LyricFA 按钮
├── 📐 音素标注 (PhonemeLabeler)      ← PhonemeLabeler + 自动 FA
├── 🎵 音高标注 (PitchLabeler)        ← PitchLabeler + 自动 add_ph_num/F0/MIDI
├── 📦 导出 (Export)                  ← CSV + ds/ + wavs/ 输出
```

侧边栏 6 个图标。打开工程前仅显示欢迎页，其余页面灰显不可用。

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

**新建工程向导**（模态对话框，4 步）：

1. **基本信息**：工程保存位置、名称
2. **音频导入**：选择源音频文件（支持多文件/文件夹拖入）、选择语言和说话人
3. **切片配置**：内嵌切片参数（threshold、minLength 等），可使用默认值。这些参数同步写入 `defaults.slicer`
4. **执行切片**：进度条，自动切片 → 创建 `.dsproj` + `audio/` + `dstemp/slices/` + 初始 PipelineContext

向导完成后自动跳转到初始设置页。切片参数已写入工程，后续可在设置页修改并重切。

**打开工程**：选择 `.dsproj` 文件，加载工程数据，根据 `completedSteps` 恢复进度。

### 3.4 初始设置页 (Settings)

横向 TabWidget，每个 tab 对应流水线中一个步骤的配置。所有配置持久化到 `.dsproj` 的 `defaults` 字段。

| Tab | 配置项 | 对应 .dsproj defaults 路径 |
|-----|--------|---------------------------|
| **切片** | 切片参数（threshold、minLength、minInterval 等）、命名前缀 | `defaults.slicer` |
| **ASR** | ASR 模型路径、推理提供者 (CPU/DML/CUDA) | `defaults.models.asr` |
| **词典/G2P** | 各语言词典路径、音素表路径 | `languages[]` |
| **强制对齐** | FA 模型路径、推理提供者、☑ 预加载 [N] 个文件 | `defaults.models.phoneme_alignment` + `defaults.preload` |
| **音高/MIDI** | F0 提取方式 (RMVPE)、MIDI 提取方式 (GAME)、模型路径、推理提供者、☑ 预加载 [N] 个文件 | `defaults.models.pitch_extraction` + `defaults.models.midi_transcription` + `defaults.preload` |
| **导出** | 导出格式 (CSV/DS)、采样率、hop_size、重采样参数、是否包含丢弃切片 | `defaults.export` |
| **预处理** | 预处理器列表（响度归一化、降噪）、参数 | `defaults.preprocessors` |

#### 预加载机制

强制对齐 tab 和 音高/MIDI tab 提供"预加载"选项：

```
☑ 预加载    文件数: [10]
```

启用后，进入对应应用页时自动在后台：
- **强制对齐预加载**：对前 N 个未完成 FA 的切片执行 HuBERT-FA，渲染频谱/功率图
- **音高预加载**：对前 N 个未完成的切片执行 add_ph_num → RMVPE F0 提取 → GAME MIDI 转录，渲染 mel 图

预加载在后台线程执行，不阻塞 UI。结果写入 PipelineContext。

### 3.5 歌词标注页 (MinLabel)

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

### 3.6 音素标注页 (PhonemeLabeler)

#### 基础功能

继承 LabelSuite 的 PhonemeLabeler 页全部功能，数据来源为工程内切片。

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

### 3.7 音高标注页 (PitchLabeler)

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

### 3.8 导出页 (Export)

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

### 3.9 main.cpp

```cpp
#include <QApplication>
#include <dsfw/AppShell.h>
#include <dsfw/Theme.h>

#include "WelcomePage.h"
#include "SettingsPage.h"
#include "DsMinLabelPage.h"
#include "DsPhonemeLabelerPage.h"
#include "DsPitchLabelerPage.h"
#include "ExportPage.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("DsLabeler");

    dsfw::Theme::instance().init(app);

    dsfw::AppShell shell;
    shell.setWindowTitle("DsLabeler — DiffSinger Dataset Labeler");

    shell.addPage(new WelcomePage(&shell),
                  "welcome", QIcon(":/icons/home.svg"), "工程");
    shell.addPage(new SettingsPage(&shell),
                  "settings", QIcon(":/icons/settings.svg"), "设置");
    shell.addPage(new DsMinLabelPage(&shell),
                  "minlabel", QIcon(":/icons/label.svg"), "歌词");
    shell.addPage(new DsPhonemeLabelerPage(&shell),
                  "phoneme", QIcon(":/icons/phoneme.svg"), "音素");
    shell.addPage(new DsPitchLabelerPage(&shell),
                  "pitch", QIcon(":/icons/pitch.svg"), "音高");
    shell.addPage(new ExportPage(&shell),
                  "export", QIcon(":/icons/export.svg"), "导出");

    shell.resize(1400, 900);
    shell.show();

    return app.exec();
}
```

### 3.10 CMake

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

## 4 页面复用架构

LabelSuite 和 DsLabeler 的标注页面共享核心编辑器组件，通过 `IEditorDataSource` 接口解耦：

```cpp
/// 编辑器数据源接口 — 抽象文件系统 vs 工程数据访问
class IEditorDataSource {
public:
    virtual ~IEditorDataSource() = default;

    /// 获取当前切片列表（LabelSuite: 单文件; DsLabeler: 工程内全部切片）
    virtual QStringList sliceIds() const = 0;

    /// 加载指定切片的标注数据
    virtual Result<DsTextDocument> loadSlice(const QString &sliceId) = 0;

    /// 保存标注数据
    virtual Result<void> saveSlice(const QString &sliceId,
                                   const DsTextDocument &doc) = 0;

    /// 获取音频文件路径
    virtual QString audioPath(const QString &sliceId) const = 0;
};
```

```
                    ┌───────────────────────────┐
                    │   核心编辑器组件 (共享)      │
                    │   MinLabelEditor           │
                    │   PhonemeEditor            │
                    │   PitchEditor              │
                    │                            │
                    │   依赖 IEditorDataSource   │
                    └──────────┬────────────────┘
                               │
                 ┌─────────────┼─────────────┐
                 ▼                           ▼
    ┌─────────────────────┐    ┌─────────────────────┐
    │ FileDataSource       │    │ ProjectDataSource    │
    │ (LabelSuite)         │    │ (DsLabeler)          │
    │                      │    │                      │
    │ 打开/保存文件         │    │ 读写 .dstext +        │
    │ 单文件 = 单切片       │    │ PipelineContext      │
    └─────────────────────┘    └─────────────────────┘
```

| 组件 | 位置 | 说明 |
|------|------|------|
| IEditorDataSource | `src/domain/include/dstools/` | 数据源接口 |
| FileDataSource | `src/apps/LabelSuite/` | 文件系统实现 |
| ProjectDataSource | `src/apps/DsLabeler/` | 工程 + PipelineContext 实现 |
| MinLabelEditor | `src/widgets/` 或 `src/apps/shared/` | 核心歌词编辑逻辑，G2P |
| PhonemeEditor | 同上 | 核心 TextGrid 编辑逻辑，波形/频谱 |
| PitchEditor | 同上 | 核心 F0/MIDI 编辑逻辑，钢琴卷帘 |

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
[欢迎页] ─── 新建工程：选择音频 → 切片 → 创建 .dsproj
  │
  ▼
[初始设置] ─── 配置切片参数、模型路径、词典、预加载
  │
  ▼
[歌词标注页] ─── ASR(可选) → G2P → grapheme 层 → dstext
  │
  ▼ (可跳过)                    ┌──────────────────────┐
[音素标注页] ─── 自动FA → 手动修正 → phoneme 层 → dstext  │
  │                              │  修改后 → dirty:     │
  ▼ (可跳过)                    │  ph_num, midi        │
[音高标注页] ─── 自动 add_ph_num + F0 + MIDI → 手动修正   │
  │              ↑ dirty 时自动重算                      │
  │              └──────────────────────────────────────┘
  ▼
[导出页] ─── 补全缺失步骤 + 清除所有 dirty → CSV + ds/(可选) + wavs/
```

用户可在任意页面之间**自由跳转**。回退到上游页面修改数据后，下游自动标记 dirty，下次进入下游页面时自动重算并 Toast 提示。详见 [ds-format.md §6](ds-format.md)。

### 6.3 从 DiffSingerLabeler 迁移

| DiffSingerLabeler 步骤 | DsLabeler 对应 |
|------------------------|-------------|
| Step 1: Slice | 欢迎页 → 创建工程 |
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

### 7.1 歌词标注页

```
文件(F)   编辑(E)   处理(P)   视图(V)   帮助(H)
                     ├─ ASR 识别当前曲目
                     ├─ 歌词匹配
                     ├─ ─────────────
                     └─ 批量 ASR...
```

### 7.2 音素标注页

```
文件(F)   编辑(E)   处理(P)   视图(V)   帮助(H)
                     ├─ 强制对齐当前切片
                     ├─ ─────────────
                     └─ 批量强制对齐...
```

### 7.3 音高标注页

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

---

## 关联文档

- [architecture.md](architecture.md) — 项目架构概述
- [task-processor-design.md](task-processor-design.md) — 流水线架构设计
- [ds-format.md](ds-format.md) — 工程格式规范
- [refactoring-roadmap.md](refactoring-roadmap.md) — 重构路线图
- [migration-guide.md](migration-guide.md) — AppShell 迁移指南

> 更新时间：2026-05-02
