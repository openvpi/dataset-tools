# 重构路线图

> 2026-05-07（更新于 2026-05-07）
>
> 前置文档：[human-decisions.md](human-decisions.md)、[unified-app-design.md](unified-app-design.md)
>
> 架构参考：[framework-architecture.md](framework-architecture.md)

---

### 关键架构决策 (ADR)

| ADR | 决策 | 状态 |
|-----|------|------|
| 8 | 单仓库模式 | 有效 |
| 43 | int64 微秒时间精度 | 有效 |
| 46 | LabelSuite + DsLabeler 两个 exe | 有效 |
| 52 | IEditorDataSource 抽象数据源 | 有效 |
| 57 | 层依赖 DAG + per-slice dirty 标记 | 有效 |
| 62 | 右键直接播放，不弹菜单 | 有效 |
| 65 | IBoundaryModel → AudioVisualizerContainer 扩展 | 有效 |
| 66 | LabelSuite 底层统一 dstext | 有效 |
| 69 | ISettingsBackend → 废弃 ProjectSettingsBackend | 有效 |
| 70 | FileDataSource 内部 dstext + FormatAdapter | 有效 |
| 78 | Slicer/Phoneme 统一视图组合（D-30） | 有效 |
| 79 | Log 侧边栏改为独立 LogPage | 有效 |
| 80 | PlayWidget 禁止 ServiceLocator 共享 IAudioPlayer（D-31） | 有效 |
| 81 | invalidateModel 先发信号再卸载（D-32） | 有效 |
| 82 | AudioPlaybackManager 应用级音频仲裁（D-34） | ✅ 已完成 |
| 83 | 贯穿线鼠标光标反馈（D-35） | ✅ 已完成 |
| 84 | 日志持久化 FileLogSink（D-36） | ✅ 已完成 |

### 已完成任务

| 任务 | 完成日期 |
|------|---------|
| 14.1 FA 异步任务取消令牌加固 | 2026-05-07 |
| 11.1 贯穿线区域鼠标光标反馈 | 2026-05-07 |
| 12.1 FileLogSink 日志文件持久化 | 2026-05-07 |
| 10.1 AudioPlaybackManager 应用级音频仲裁器 | 2026-05-07 |
| 13.1 QSS 颜色变量提取与统一管理 | 2026-05-07 |

---

## 任务 15：FA 多层标签从属关系日志

> 设计依据：P-04（错误信息必须追溯到根因）、P-01（行为不分散）
> 审核优化：日志应体现 grapheme→phoneme 的包含关系，便于排查 FA 对齐结果。

### 15.1 FA 导出时记录多层标签从属关系

**问题**：
- `buildFaLayers()` 当前仅记录 binding group（边界对齐关系），不记录 grapheme 区间包含哪些 phoneme 区间的从属关系
- 用户无法从日志中看到 FA 输出的层级包含结构（如 grapheme "shi" 包含 phoneme "sh" + "i"）
- 排查 FA 对齐异常时缺少关键信息

**需求**：
- 在 `buildFaLayers()` 中，为每个 grapheme 区间记录其包含的 phoneme 区间列表
- 日志格式：`[INFO] fa: grapheme "shi" [0.00-0.50s] → phones: sh [0.00-0.20s], i [0.20-0.50s]`
- 在汇总日志中增加层级包含统计

**审核优化（P-01）**：
- 日志输出集中在 `buildFaLayers()` 中，不分散到其他函数
- 保持现有的 binding group 日志不变，新增从属关系日志

**文件**：
- 修改：`src/apps/shared/data-sources/PhonemeLabelerPage.cpp`（`buildFaLayers()` 函数）

---

## 任务 16：贯穿线拖动修复

> 设计依据：D-05（分割线纵向贯穿规则）、D-28（Active tier 只控制子图边界线显示，不限制拖动）
> 审核优化：P-01（行为不分散）——hit-test 逻辑应与 boundary model 设置一致。

### 16.1 Phoneme 贯穿线在子图区域不可拖动

**问题**：
- 标签区域（IntervalTierView）内边界线可正常拖动
- 子图区域（WaveformWidget/SpectrogramWidget/PowerWidget）的贯穿线可见（由 BoundaryOverlayWidget 绘制）但不可拖动
- 根因：子图 widget 的 `hitTestBoundary()` 检查 `m_boundaryOverlayEnabled`，但 PhonemeEditor 从未调用 `setBoundaryOverlayEnabled(true)`
- BoundaryOverlayWidget 设置了 `WA_TransparentForMouseEvents`，鼠标事件穿透到子图 widget，但子图 widget 因 overlay 未启用而无法 hit-test

**需求**：
- 子图区域的贯穿线应可拖动，与标签区域行为一致
- 拖动贯穿线时，BoundaryDragController 正常工作（含绑定伙伴同步移动）

**方案**：
- 将 `hitTestBoundary()` 中的 `m_boundaryOverlayEnabled` 检查移除，仅保留 `m_boundaryModel` 检查
- `m_boundaryOverlayEnabled` 仅控制 `drawBoundaryOverlay()` 的绘制行为
- 这样子图 widget 只要有 boundary model 就能 hit-test，绘制仍由 BoundaryOverlayWidget 统一负责

**审核优化（P-01/P-07）**：
- 不引入新的 flag，仅解耦 hit-test 与 draw 的条件判断
- 三个子图 widget（Waveform/Spectrogram/Power）统一修改

**文件**：
- 修改：`src/apps/shared/phoneme-editor/ui/WaveformWidget.cpp`（`hitTestBoundary()` 移除 overlay 检查）
- 修改：`src/apps/shared/phoneme-editor/ui/SpectrogramWidget.cpp`（同上）
- 修改：`src/apps/shared/phoneme-editor/ui/PowerWidget.cpp`（同上）

---

## 任务 17：Phoneme 空标签修复

> 设计依据：P-04（错误信息必须追溯到根因）、D-27（标签层级包含规则）
> 审核优化：P-07（简洁可靠）——空标签应统一替换为 SP，而非在各处分别处理。

### 17.1 FA 输出空标签内容 + phoneme 层静音区间默认值

**问题**：
- `buildFaLayers()` 末尾边界的 text 为空字符串，导致 phoneme 层末尾 interval 标签为 `""`
- `TextGridDocument::loadFromDsText()` 中首段静音区间、空层、分割后右半 interval 均使用空字符串
- FA 引擎（`WordList::add_SP()`）已在 word 间隙插入 SP word，但 `buildFaLayers()` 末尾边界未设置 SP
- 空标签导致边界绑定失败（BoundaryDragController 的时间匹配可能将空标签区间误关联）

**需求**：
- `buildFaLayers()` 中 phoneme 层末尾 interval 的 text 设为 `"SP"`
- `TextGridDocument::loadFromDsText()` 中，phoneme 层的空文本 interval 自动替换为 `"SP"`
- `TextGridDocument::insertBoundary()` 中，phoneme 层分割后右半 interval 默认为 `"SP"`
- FA 逻辑不应输出空标签内容的 interval

**审核优化（P-01）**：
- phoneme 层空标签→SP 的逻辑集中在 `TextGridDocument::loadFromDsText()` 中处理
- `buildFaLayers()` 作为数据源头也应正确设置 SP
- 层名称判断：当层名为 `"phoneme"` 或层索引对应 phoneme 层时，空文本替换为 `"SP"`

**文件**：
- 修改：`src/apps/shared/data-sources/PhonemeLabelerPage.cpp`（`buildFaLayers()` 末尾边界 text 设为 "SP"）
- 修改：`src/apps/shared/phoneme-editor/ui/TextGridDocument.cpp`（`loadFromDsText()` 和 `insertBoundary()` 空标签→SP）

---

## 任务 18：文件列表脏状态与保存提醒优化

> 设计依据：D-19（所有页面统一使用全功能文件/切片列表）、P-01（行为不分散）
> 审核优化：P-07（简洁可靠）——切换条目时自动保存，仅在退出/切页时提醒。

### 18.1 文件列表体现未修改状态

**问题**：
- SliceListPanel 不显示切片的修改/未修改状态
- 用户无法从列表中识别哪些切片已修改未保存

**需求**：
- SliceListPanel 中已修改的切片项显示视觉标记（如项目文字加粗或前缀 `*`）
- 编辑器 `isDirty()` 变化时，SliceListPanel 对应项实时更新

### 18.2 切换条目时不提醒未保存

**问题**：
- `EditorPageBase::onSliceSelected()` 调用 `maybeSave()` 弹出保存确认对话框
- 频繁切换切片时，每次都弹窗打断工作流

**需求**：
- 切换切片时自动保存当前切片（静默保存，不弹窗）
- 仅在以下场景弹出未保存提醒：
  - 页面失活（`onDeactivating()`）
  - 应用关闭（`AppShell::closeEvent()`）
- 移除 `onSliceSelected()` 中的 `maybeSave()` 调用，改为自动保存

**审核优化（P-01/P-07）**：
- 自动保存逻辑封装在 `EditorPageBase::autoSaveCurrentSlice()` 中
- 子类只需实现 `saveCurrentSlice()`，自动保存由基类统一调用
- SliceListPanel 的脏状态更新由 `EditorPageBase` 统一管理，不分散到各子类

**文件**：
- 修改：`src/apps/shared/data-sources/EditorPageBase.h`（添加 `autoSaveCurrentSlice()`、脏状态通知）
- 修改：`src/apps/shared/data-sources/EditorPageBase.cpp`（`onSliceSelected()` 改为自动保存）
- 修改：`src/apps/shared/data-sources/SliceListPanel.h`（添加 `setSliceDirty()` 方法）
- 修改：`src/apps/shared/data-sources/SliceListPanel.cpp`（脏状态视觉标记）

---

## 任务 19：设置页面自动保存

> 设计依据：D-01（配置全部移出工程文件）、P-07（简洁可靠）
> 审核优化：自动保存是通用功能，应在 Settings 通用 tab 中配置。

### 19.1 设置页面添加自动保存配置

**问题**：
- 当前无自动保存机制，用户修改后需手动保存
- 异常退出时可能丢失大量编辑

**需求**：
- SettingsPage "通用" tab 新增自动保存配置：
  - ☑ 启用自动保存（默认启用）
  - 间隔：[30] 秒（最小 10s，最大 300s）
- 自动保存范围：当前活跃页面的当前切片（调用 `saveCurrentSlice()`）
- 自动保存仅在页面有脏数据时触发
- 配置持久化到 `AppSettings`：`General/autoSaveEnabled`、`General/autoSaveIntervalMs`

**审核优化（P-01）**：
- 自动保存定时器由 `EditorPageBase` 统一管理
- 定时器在 `onActivated()` 时启动，`onDeactivated()` 时停止
- 各子类无需关心自动保存逻辑

**文件**：
- 修改：`src/apps/shared/settings/SettingsPage.cpp`（通用 tab 新增自动保存配置）
- 修改：`src/apps/shared/data-sources/EditorPageBase.h`（添加自动保存定时器）
- 修改：`src/apps/shared/data-sources/EditorPageBase.cpp`（定时器逻辑）

---

## 任务 20：文件列表面板统一

> 设计依据：D-19（所有页面统一使用全功能文件/切片列表）、P-01（行为不分散）
> 审核优化：SliceListPanel 已有两种模式，需确保行为一致。

### 20.1 多页面文件列表行为统一

**问题**：
- SliceListPanel 有 Editor 模式和 Slicer 模式，但各页面的使用方式不完全一致
- MinLabelPage、PhonemeLabelerPage、PitchLabelerPage 均使用 Editor 模式，但功能开关不统一
- SlicerPage/DsSlicerPage 使用 AudioFileListPanel（DroppableFileListPanel 子类），与 SliceListPanel 是完全不同的类
- D-19 要求所有页面统一使用 SliceListPanel

**需求**：
- 审查 SliceListPanel 的 Editor 模式在三个页面（MinLabel/Phoneme/Pitch）中的使用差异
- 统一功能开关：进度条、右键菜单、丢弃/恢复等
- SlicerPage/DsSlicerPage 的 AudioFileListPanel 保留（D-19 明确"AudioFileListPanel 仅在 Slicer 页面保留"）
- 确保三个 Editor 页面的 SliceListPanel 实例行为完全一致

**审核优化（P-01）**：
- SliceListPanel 通过构造参数或 setter 选择性开启功能
- 功能列表：进度条、脏状态标记、右键菜单项、导航按钮
- 各页面传入相同的功能配置

**文件**：
- 修改：`src/apps/shared/data-sources/SliceListPanel.h`（功能配置接口）
- 修改：`src/apps/shared/data-sources/SliceListPanel.cpp`（功能开关实现）
- 修改：`src/apps/shared/data-sources/MinLabelPage.cpp`（统一配置）
- 修改：`src/apps/shared/data-sources/PhonemeLabelerPage.cpp`（统一配置）
- 修改：`src/apps/shared/data-sources/PitchLabelerPage.cpp`（统一配置）

---

## 任务 21：MinLabel 播放按钮图标修复

> 设计依据：P-01（行为不分散）——PhonemeEditor 和 PitchEditor 已有动态图标切换。
> 审核优化：P-07（简洁可靠）——参照已有实现，最小改动。

### 21.1 MinLabel 音频播放按钮 SVG 图标不显示

**问题**：
- MinLabelEditor 内嵌的 PlayWidget 有 play/stop/dev 三个 QPushButton 并设置了 SVG 图标
- 但在 MinLabel 页面中，这些按钮的图标不可见
- 可能原因：QSS 中 `play-widget` 或 `play-button`/`stop-button`/`dev-button` 的样式覆盖了图标

**需求**：
- 确保 PlayWidget 的播放/停止/设备按钮正确显示 SVG 图标
- 播放状态切换时图标动态更新（play.svg ↔ pause.svg）
- 行为与 PhonemeEditor/PitchEditor 一致

**审核优化（P-01）**：
- PlayWidget 内部已有 `reloadButtonStatus()` 做图标切换
- 问题可能在 QSS 样式覆盖，需检查 dark.qss / light.qss 中相关选择器
- 参照 PhonemeEditor 的工具栏 QAction 图标显示方式

**文件**：
- 修改：`src/framework/ui-core/res/themes/dark.qss`（检查/修复 play-button 样式）
- 修改：`src/framework/ui-core/res/themes/light.qss`（同上）

---

## 任务 22：图表分割线颜色优化

> 设计依据：P-06（接口稳定）——仅调整颜色值，不改变 QSS 结构。
> 审核优化：P-07（简洁可靠）——最小改动，仅调整 splitter handle 颜色。

### 22.1 波形图和 Power 图等子图间分割线颜色不明显

**问题**：
- QSplitter handle 颜色使用 `{{border}}`（暗色 `#43454A`，亮色 `#EBECF0`）
- 在深色波形图/频谱图背景上，分割线几乎不可见
- 用户难以感知图表边界，拖动调整比例困难

**需求**：
- 图表区域的 QSplitter handle 使用更明显的颜色
- 暗色主题：使用 `{{borderLight}}`（`#4E5157`）或稍亮的颜色
- 亮色主题：使用 `{{borderLight}}`（`#C9CCD6`）或稍深的颜色
- 可选：增大 handle 高度（从 2px → 3px）提升可拖动性

**审核优化（P-07）**：
- 不新增 Palette 字段，复用 `borderLight`
- 仅修改 QSS 中 `QSplitter::handle` 的 `background-color`
- 如果 chartSplitter 需要与其他 QSplitter 区分，可通过 objectName 设置专属样式

**文件**：
- 修改：`src/framework/ui-core/res/themes/dark.qss`（QSplitter::handle 颜色）
- 修改：`src/framework/ui-core/res/themes/light.qss`（同上）

---

## 任务依赖关系

```
15.1 (FA从属关系日志) ── 独立，无依赖
16.1 (贯穿线拖动修复) ── 独立，无依赖
17.1 (空标签修复)     ── 独立，无依赖
18.1 (文件列表脏状态) ── 独立，无依赖
18.2 (切换不提醒保存) ── 依赖 18.1
19.1 (自动保存设置)   ── 依赖 18.2
20.1 (文件列表统一)   ── 依赖 18.1
21.1 (播放按钮图标)   ── 独立，无依赖
22.1 (分割线颜色)     ── 独立，无依赖
```

**执行顺序**：15.1 → 16.1 → 17.1 → 21.1 → 22.1 → 18.1 → 18.2 → 20.1 → 19.1
（先修 bug 再增强，先独立后依赖，先简单后复杂）

---

## 关联文档

- [human-decisions.md](human-decisions.md) — 人工决策记录（最高优先级）
- [unified-app-design.md](unified-app-design.md) — 应用设计方案
- [framework-architecture.md](framework-architecture.md) — 项目架构
- [conventions-and-standards.md](conventions-and-standards.md) — 编码规范
- [log-and-i18n-design.md](log-and-i18n-design.md) — Log 系统与本地化设计
