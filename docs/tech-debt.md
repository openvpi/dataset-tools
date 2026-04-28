# 技术债清单

**Version**: 1.2 | **Date**: 2026-04-28

> 本文档记录项目的**架构级技术债务**和**战略改进项**。
> 具体 Bug 和代码质量问题详见 `bugs.md`。

---

## 架构债

### AD-01: hubert-infer 未提取

HubertFA 的推理代码仍然留在应用层，没有下沉到 `src/infer/` 层。

- **位置**: `src/apps/pipeline/hubertfa/` 下的推理相关代码
- **目标**: 提取到 `src/infer/hubert-infer/`，与 game-infer、some-infer、rmvpe-infer 对齐
- **工作量**: M（1~2天）
- **阻塞**: 无
- **被阻塞**: 无

### AD-02: GameInfer 未迁移到 GpuSelector

GameInfer 仍在使用私有的 DmlGpuUtils，没有切换到 dstools-widgets 中统一的 GpuSelector。

- **位置**: `src/apps/GameInfer/`
- **目标**: 移除私有 DmlGpuUtils，改用 `src/widgets/` 中的 GpuSelector
- **工作量**: S（半天）
- **阻塞**: 无

### AD-03: ErrorHandling 模块缺失

整个项目没有统一的错误处理框架。缺少 Result/Status 类型，各模块自行处理错误，风格不一。

- **目标**: 实现 ErrorHandling 模块，提供 `Result<T>` / `Status` 类型
- **工作量**: M（1~2天设计+实现）
- **阻塞**: CQ-009（详见 bugs.md）依赖此项

### AD-04: ThreadUtils 模块缺失

缺少 `invokeOnMain()` 等跨线程调用工具。各处自行实现 Qt 线程间通信。

- **目标**: 实现 ThreadUtils 模块
- **工作量**: S（半天）
- **阻塞**: 无

### AD-05: Theme System 不完整

当前只有 Dark/Light 两种硬编码模式，没有可扩展的主题系统。

- **位置**: `src/core/` Theme 相关
- **目标**: 实现可扩展的主题模式
- **工作量**: M（1~2天）
- **被阻塞于**: ~~CQ-002~~（已解决 -- PitchLabeler 全新架构）

### AD-06: SVG 图标系统未实现

项目仍使用位图图标，没有 SVG 图标支持。

- **工作量**: S（半天）
- **阻塞**: 无

### AD-07: DPI 适配未完成

高 DPI 显示器上界面缩放有问题。

- **工作量**: M（1~2天，需逐个窗口调整）
- **阻塞**: 无

### AD-08: 音高工具函数重复实现

Hz↔MIDI、音名↔MIDI 转换在两处独立实现，存在微妙不一致，可能导致同一 .ds 文件在不同应用中音高显示偏差。

- **位置**:
  - `PitchLabeler/gui/ui/PianoRollView.cpp` — `freqToMidi`、`midiToFreq`、`parseNoteName`（正则法，支持 cents）
  - `PitchLabeler/gui/DSFile.cpp` — 加载/保存时使用 `parseNoteName` 进行 note_seq 解析，内联 Hz↔MIDI 转换
- **历史**: 原 SlurCutter/F0Widget.cpp 中有第三份实现（查表法），SlurCutter 已删除
- **风险**: DSFile 与 PianoRollView 的转换逻辑若存在差异，同一文件内部可能产生不一致
- **目标**: 在 `dstools-core` 新建 `PitchUtils.h`，统一 `freqToMidi`/`midiToFreq`/`midiToNoteName`/`noteNameToMidi`（含 cents 支持），两处全部改为调用公共实现
- **工作量**: S（半天）
- **阻塞**: 无

### AD-09: 音频文件关联逻辑不一致

两个应用各自实现「根据数据文件找对应音频」的逻辑，且存在兼容性问题。

- **位置**:
  - `MinLabel/Common.cpp:17-44` — `labFileToAudioFile()`：扫描目录匹配同名 wav/mp3/m4a/flac
  - `PitchLabeler/MainWindow.cpp:673-682` — 内联代码：简单替换 .ds 后缀尝试各种音频扩展名
- **历史**: 原 SlurCutter/MainWindow.cpp 中有 `audioFileToDsFile()` 反向映射，将音频后缀编码进 .ds 文件名（`song_mp3.ds`）。SlurCutter 已删除，但此约定产生的 .ds 文件可能仍存在于用户数据中
- **风险**: PitchLabeler 用简单扩展名替换，**无法找到 SlurCutter 产生的非 wav 音频文件**（如 `song_mp3.ds` 对应 `song.mp3`）。这是一个潜在的兼容性 Bug
- **目标**: 在 `dstools-core` 新增 `AudioFileResolver`，同时支持两种约定（简单同名匹配 + SlurCutter 后缀编码），保证所有应用行为一致
- **工作量**: S（半天）
- **阻塞**: 无

### AD-10: MinLabel JSON I/O 未使用 JsonHelper

MinLabel 自行实现了 `readJsonFile`/`writeJsonFile`（`Common.cpp:223-262`），使用 `QFile` 直接写入，没有原子写入保护。

- **位置**: `src/apps/MinLabel/gui/Common.cpp:223-262`
- **风险**: 保存过程中崩溃或断电会导致文件损坏。`JsonHelper::saveFile` 已通过 temp+rename 解决此问题
- **目标**: MinLabel 的 JSON I/O 改为调用 `JsonHelper`（或直接使用 `DsDocument`），获得原子写入和统一错误处理
- **工作量**: XS（改动量小，收益明确）
- **阻塞**: 无

### AD-11: 快捷键管理缺少集中绑定机制

三个应用都手动维护 action↔快捷键绑定——定义 action、打开编辑器、手动逐个回写快捷键。每个应用 ~30 行重复的绑定/回写代码。

- **位置**:
  - `MinLabel/MainWindow.cpp:56-75` — 快捷键注册
  - `PitchLabeler/MainWindow.cpp:144-182` — 快捷键注册（17 个快捷键条目，已集成 ShortcutEditorWidget）
  - `PhonemeLabeler/` — 使用 `AppSettings.shortcut()` 但尚未集成 ShortcutEditorWidget
  - 各应用的 `applyConfig()` 和 `closeEvent()` 中手动回写
- **风险**: 新增 action 时容易忘记在 `applyConfig`/`closeEvent` 中同步。已经实际发生过一次（commit `3d67219` — `fix(widgets): shortcut edits not taking effect on dialog close`）
- **目标**: 在 `dstools-widgets` 新增 `ShortcutManager`，持有 action↔key 映射表，提供 `applyAll()`/`saveAll()` 一次性生效，消除手动逐个回写的遗漏风险
- **工作量**: S（半天）
- **阻塞**: 无

### AD-12: MinLabel 文件浏览树可提取

MinLabel 的 `QTreeView + QFileSystemModel + 列隐藏 + FileStatusDelegate + 目录打开` 逻辑可提取为公共组件。

- **位置**:
  - `MinLabel/MainWindow.cpp:113-146` — QTreeView + QFileSystemModel 设置，隐藏列、设置过滤器（`*.wav/*.mp3/*.m4a/*.flac`）、创建 `FileStatusDelegate`
- **历史**: 原 SlurCutter 有几乎相同的实现（~60 行逐行重复），SlurCutter 已删除
- **目标**: 在 `dstools-widgets` 抽取为 `AudioFileBrowser` 组件，配置项为文件过滤器和状态检查回调
- **注意**: PitchLabeler 的 `FileListPanel` 是不同设计（QListWidget，浏览 .ds 文件），PhonemeLabeler 也有 `FileListPanel`（QListWidget），两者均不应与 QTreeView 方案强制统一
- **工作量**: S（半天）
- **阻塞**: 无

### 不建议抽取的部分

以下经过评估后认为不值得抽取，记录于此避免重复讨论：

| 项 | 原因 |
|---|---|
| DS 字段解析（note_seq 拆分等） | PitchLabeler 需完整解析为 typed struct，与其他应用需求层次不同，强行统一会过度设计 |
| 主窗口基类 `BaseLabelerWindow` | 三个应用的菜单结构、状态栏、布局差异较大，抽基类会过度耦合 |
| 拖拽打开目录 | 仅 ~20 行实际逻辑（MinLabel 的 dragEnterEvent/dropEvent），提取为 mixin 的成本约等于收益 |

---

## 代码卫生

### CH-01: HubertFA 旧版目录未清理

`src/apps/HubertFA/` 保留了完整的旧版独立应用（main.cpp + GUI + util/），但已不参与构建（`src/apps/CMakeLists.txt` 不含 `add_subdirectory(HubertFA)`）。

- **位置**: `src/apps/HubertFA/`（约 20 个文件）
- **目标**: AD-01 完成后删除整个目录
- **工作量**: XS（确认 pipeline/hubertfa 不再引用后直接删除）
- **阻塞**: AD-01（hubert-infer 提取）
- **附带问题**: 含备份文件 `util/HfaThread.cpp~`，应立即删除

### CH-02: smoke-test.ps1 检查旧版 EXE

`scripts/smoke-test.ps1` 的 L3 测试仍检查 `AudioSlicer.exe`、`LyricFA.exe`、`HubertFA.exe` 是否存在，但这三个已合并为 `DatasetPipeline.exe`。

- **位置**: `scripts/smoke-test.ps1`
- **目标**: 更新为检查 `DatasetPipeline.exe`
- **工作量**: XS（修改约 5 行）
- **阻塞**: 无

### CH-03: resources/ 主题和图标目录为空

`resources/icons/` 和 `resources/themes/` 目录已创建但未填充任何文件。dark.qss/light.qss 和 SVG 图标文件均不存在。当前 QSS 主题由 `src/core/res/` 内的资源提供。

- **位置**: `resources/`
- **目标**: 填充 QSS 主题文件和 SVG 图标，或删除空目录避免误解
- **工作量**: 填充 QSS: S（半天）；SVG 图标: M（1~2天）
- **阻塞**: AD-06（SVG 图标系统）

### CH-04: scripts/ 含空子目录

`scripts/cmake/` 和 `scripts/vcpkg-manifest/` 为空目录。实际 cmake 工具脚本在 `cmake/`，vcpkg 清单在 `scripts/vcpkg-manifest/`（也为空）。

- **位置**: `scripts/cmake/`, `scripts/vcpkg-manifest/`
- **目标**: 删除空目录或迁移内容
- **工作量**: XS
- **阻塞**: 无

---

## 代码质量 & 库级别问题（交叉引用）

以下项目同时出现在 `bugs.md` 中，此处仅列出 ID 供依赖关系和优先级排序引用。完整描述详见 `bugs.md`。

**代码质量:**
~~CQ-002~~（已解决）, CQ-003, CQ-005, CQ-006a, ~~CQ-006b~~（已解决）, ~~CQ-007~~（已解决）, CQ-008, CQ-009, CQ-010, CQ-011

**库级别 (Major):**
M-01, M-02, M-03, M-05, M-06, M-11

**库级别 (Minor):**
L-02, L-03, L-04, L-05, L-06, L-07, L-08, L-13

---

## FunAsr 专项

FunAsr 模块集中了全项目约 **36%** 的问题，代码风格为 C 风格，与项目整体的现代 C++ 方向严重不符。

**问题清单汇总：**

1. **CQ-008**: 无 RAII，裸指针满天飞（详见 bugs.md）
2. **CQ-010**: 魔法数字（详见 bugs.md）
3. **L-05**: 死代码 `tmp.h`（详见 bugs.md）
4. **L-06**: 宏定义在命名空间内部（详见 bugs.md）
5. 无异常安全保证
6. 无边界检查
7. 无 NULL 检查

**建议方案：**

- **短期**: 修复 L-05、L-06 等低成本项，添加基本的 NULL 检查
- **长期**: 评估替换为 whisper.cpp 等替代 ASR 库的可行性。如果替换成本可接受，整体替换比逐项修复更划算

**替换评估要点：**
- whisper.cpp 是否满足当前 ASR 需求（中文语音识别精度、ONNX Runtime 兼容性）
- 接口适配工作量
- 模型转换成本

---

## 依赖关系

```
CQ-002 (F0Widget拆分) ← 已解决（PitchLabeler 全新架构）
  ├── 解锁 → AD-05 (Theme适配) — 不再阻塞
  └── 顺带 → CQ-007 (不当注释) — 已解决

AD-03 (ErrorHandling模块)
  └── 解锁 → CQ-009 (统一错误处理)

AD-08 (PitchUtils)
  └── 解锁 → PitchLabeler 内部音高显示一致性

AD-09 (AudioFileResolver)
  └── 修复 → PitchLabeler 无法打开 SlurCutter 历史产生的非 wav 文件

CQ-003 (i18n) ← 详见 bugs.md
  └── 包含 → L-02, M-03
```

---

## 按优先级排序

### 工作量估算说明

| 级别 | 时间 |
|------|------|
| XS | < 30分钟 |
| S | 半天 |
| M | 1~2天 |
| L | 2~3天 |
| XL | 3~5天 |

### P0 (高优先，快速修复)

| 项目 | 工作量 | 说明 |
|------|--------|------|
| CQ-005 | XS | 详见 bugs.md |
| CQ-006b | ✅ | 已解决 -- SlurCutter 已删除 |
| CQ-007 | ✅ | 已解决 -- SlurCutter 已删除 |
| CQ-011 | XS | 详见 bugs.md |
| M-01, M-02, M-06, M-11 | XS each | 详见 bugs.md |
| L-04, L-05, L-07 | XS each | 详见 bugs.md |
| AD-10 | XS | MinLabel JSON I/O 迁移到 JsonHelper |
| CH-01 (HfaThread.cpp~ 备份) | XS | 立即删除备份文件 |
| CH-02 | XS | smoke-test.ps1 更新 |
| CH-04 | XS | 删除空目录 |

### P1 (中优先，有架构价值)

| 项目 | 工作量 | 说明 |
|------|--------|------|
| AD-08 | S | 音高工具函数统一（3处重复，有一致性风险） |
| AD-09 | S | 音频文件关联统一（跨应用兼容性 Bug） |
| AD-11 | S | 快捷键集中管理（已出现过遗漏 Bug） |
| AD-12 | S | 文件浏览树提取（~60行逐行重复） |
| AD-02 | S | GameInfer 迁移 GpuSelector |
| AD-01 | M | hubert-infer 提取 |
| CQ-006a | S | 详见 bugs.md |
| M-05 | S | 详见 bugs.md |
| L-13 | XS | 详见 bugs.md |
| CH-01 (HubertFA 目录清理) | XS | 依赖 AD-01 |
| CH-03 | S~M | 填充 resources/ |

### P2 (重要，工作量大)

| 项目 | 工作量 | 依赖 |
|------|--------|------|
| CQ-002 | ✅ | 已解决 -- PitchLabeler 全新架构，不再阻塞 AD-05 |
| AD-03 | M | 无，但阻塞 CQ-009 |
| CQ-008 | L | 部分依赖 AD-03（详见 bugs.md） |
| CQ-009 | L | 依赖 AD-03（详见 bugs.md） |

### P3 (长线规划)

| 项目 | 工作量 | 说明 |
|------|--------|------|
| CQ-003 | XL | 详见 bugs.md |
| AD-05 | M | 不再被 CQ-002 阻塞 |
| AD-06 | S | SVG 图标系统 |
| AD-07 | M | DPI 适配 |
| FunAsr 替换评估 | M | 需要调研+原型验证 |

---

## 总量统计

| 类别 | 项目数 | 预估总工作量 |
|------|--------|-------------|
| 架构债 (本文档独有) | 12 | ~11天 |
| 代码质量 (详见 bugs.md) | 7 (原10，3项已解决) | ~9天 |
| 库级别 (详见 bugs.md) | 14 | ~3天 |
| FunAsr 专项 | (含上述) | 另需评估 |
| 代码卫生 (本文档新增) | 4 | ~2天 |
| **合计** | **37项** | **~25天** |

> 以上为单人全职工作量估算，实际可并行处理。P0 项目建议在日常开发中穿插完成。
> AD-08~AD-12 为 2026-04-28 新增，源自跨应用代码重复分析。
