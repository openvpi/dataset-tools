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
| 85 | applyFaResult 不覆盖 grapheme 层，grapheme 保持为 minlabel 设置的输入源 | 有效 |

### 已完成任务

| 任务 | 完成日期 |
|------|---------|
| 14.1 FA 异步任务取消令牌加固 | 2026-05-07 |
| 11.1 贯穿线区域鼠标光标反馈 | 2026-05-07 |
| 12.1 FileLogSink 日志文件持久化 | 2026-05-07 |
| 10.1 AudioPlaybackManager 应用级音频仲裁器 | 2026-05-07 |
| 13.1 QSS 颜色变量提取与统一管理 | 2026-05-07 |
| 15.1 FA 多层标签从属关系日志 | 2026-05-07 |
| 16.1 Phoneme 贯穿线在子图区域可拖动 | 2026-05-07 |
| 17.1 FA 输出空标签内容 + phoneme 层静音区间默认值 | 2026-05-07 |
| 18.1 文件列表脏状态视觉标记 | 2026-05-07 |
| 18.2 切换条目时自动保存（不弹窗） | 2026-05-07 |
| 19.1 设置页面添加自动保存配置 | 2026-05-07 |
| 20.1 多页面文件列表行为统一 | 2026-05-07 |
| 21.1 MinLabel 播放按钮 SVG 图标显示修复 | 2026-05-07 |
| 22.1 波形图和 Power 图等子图间分割线颜色优化 | 2026-05-07 |
| 25.1 FA 输入源修正：读取 raw_text 层，G2P 中文后输入 HFA | 2026-05-07 |

---

## 任务 23：MinLabel ASR 回写流程修正 + LyricFA 集成

> 设计依据：P-07（简洁可靠）——ASR 结果应进入输入框，由 autoG2P 转换到结果框。

### 23.1 MinLabel ASR 结果回写目标修正

**问题**：
- `MinLabelPage::setAsrResult()` 调用 `loadData({}, text)` 将 ASR 文本写入 `contentText`（结果框）
- 之后 `autoG2P()` 读取 `wordsText`（输入框，为空）产生空输出，覆盖了 ASR 结果
- 正确流程：ASR → `wordsText` → autoG2P → `contentText`

**需求**：
- ASR 结果写入 `wordsText`（输入框），不清空 `contentText`
- 自动执行 G2P：读取 `wordsText` 内容，转换后写入 `contentText`

**文件**：
- 修改：`src/apps/shared/data-sources/MinLabelPage.cpp`（`setAsrResult()` 参数交换：`loadData(text, {})` → `loadData(QString(), text)`）

### 23.2 MinLabel 添加 LyricFA 匹配按钮

**问题**：
- MinLabelPage 没有 LyricFA 按钮，无法使用歌词匹配功能
- 用户需要 LyricFA 从输入框读取内容、匹配原文、覆盖输入框、自动 G2P

**需求**：
- 在菜单栏添加 "LyricFA" 菜单项（单条 + 批处理）
- LyricFA 流程：从 `wordsText` 读取 → 匹配原文歌词库 → 覆盖 `wordsText` → autoG2P → `contentText`

**方案**：
- 复用 `src/libs/lyric-fa/` 的 `MatchLyric` 和 `LyricMatcher` 组件
- LyricFA 需要歌词库路径配置（在设置中配置或对话框选择）

**文件**：
- 修改：`src/apps/shared/data-sources/MinLabelPage.h`（添加 LyricFA 相关成员）
- 修改：`src/apps/shared/data-sources/MinLabelPage.cpp`（菜单、按钮、流程逻辑）

---

## 任务 24：BatchProcessDialog 完善

> 设计依据：P-01（行为不分散）——批量操作应统一使用改进的 BatchProcessDialog。

### 24.1 BatchProcessDialog 参数区域结构化

**需求**：
- 添加 `addParamGroup(title)` 方法创建分组标题
- 支持分组折叠/展开
- 参数间添加分隔线

### 24.2 日志输出增强

**需求**：
- 每条日志前加时间戳 `[HH:MM:SS]`
- 日志级别前缀 `[INFO]` / `[WARN]` / `[ERROR]`
- 错误日志用红色显示

### 24.3 进度可视化增强

**需求**：
- 添加 ETA 预估
- 状态文本显示速度信息

**文件**：
- 修改：`src/apps/shared/data-sources/BatchProcessDialog.h`
- 修改：`src/apps/shared/data-sources/BatchProcessDialog.cpp`

---

## 任务 25：HubertFA 输入源修正 + 跨步骤污染审计

> 设计依据：SP 在 FA 流程中不可能经过 DictionaryG2P（SP 是算法后期插入的间隙标记）。
> 每步只允许覆盖本步骤的输出层，不得逆工序修改上一步结果。

### 25.1 applyFaResult 保 grapheme + readFaInput 无回退（已完成）

**问题**：
- `applyFaResult()` 用 `buildFaLayers()` 的输出（含 SP/AP）覆盖了 grapheme 层
- 再次运行 FA 时，SP 随 grapheme 文本传入 `DictionaryG2P::convert()` → 报错
- 根因：grapheme 层是上一步（minlabel）的输出，FA 逆工序修改了它

**正确流程**：
- grapheme 层由 minlabel 步骤输出，是 FA 的唯一输入源（等价于 main 分支的 .lab 文件）
- `readFaInput()` 只读取 grapheme 层，**没有任何回退**
- `applyFaResult()` 写入时跳过 grapheme 层，仅保存 phoneme 层 + binding groups
- buildFaLayers 不做任何过滤（SP/AP 仍是正确的算法输出）

**修正**：
- `applyFaResult()` 写入时跳过 grapheme 层，仅保存 phoneme 层
- `readFaInput()` 只从 grapheme 层读取（已被保护，不含 SP），无回退逻辑
- `runFaForSlice()` 和 `onBatchFA()` 统一使用 `readFaInput()`

**文件**：
- 修改：`src/apps/shared/data-sources/PhonemeLabelerPage.cpp`

### 25.2 ExportPage 使用 4-param recognize 从 .lab 文件读取（已完成）

**问题**：
- `ExportPage::doExportSlice()` 使用 4-param `recognize()` 从 `.lab` 文件读取输入
- 构建的哑 `HFA::WordList` 被完全忽略，实际输入来自磁盘上的 `.lab` 文件
- 重构版不应依赖 `.lab` 文件，应直接将 grapheme 文本作为 `lyricsText` 传入

**修正**：
- `ExportPage::doExportSlice()` 改为 5-param `recognize()`，传递 grapheme 层文本
- `ExportService` 已正确使用 5-param，无需修改

**文件**：
- 修改：`src/apps/ds-labeler/ExportPage.cpp`

### 25.3 跨步骤污染审计

| 步骤 | 输出层 | 是否被后续步骤逆工序修改 | 状态 |
|------|--------|------------------------|------|
| Slicer | audio + slice 元数据 | 无步骤修改 | ✅ |
| MinLabel | `grapheme` + `raw_text` | ~~FA 用 buildFaLayers 覆盖 grapheme~~ → 已修复 | ✅ 已修复 |
| PhonemeLabeler (FA) | `phoneme` + binding groups | ExportPage 读取 phoneme（只读，不修改） | ✅ |
| PitchLabeler | `pitch` curve | 无步骤修改 | ✅ |
| Export | `ph_num` + pitch + phoneme（如缺失） | 只写入本步骤产出层，不修改其他层 | ✅ |

**规则**：每一层只应由产生它的步骤写入。后续步骤只能读取，不能修改。违例情况：
1. ~~`applyFaResult` 覆盖 grapheme 层~~ → 已修复（跳过 grapheme 写入）
2. ~~`ExportPage` 使用 4-param `recognize()` 忽略 grapheme 文本~~ → 已修复（改用 5-param）

---

## 任务 26：Phoneme 层内容验证与过滤

### 26.1 MinLabel 保存时验证 grapheme 层内容

**问题**：
- 用户在 MinLabel 结果框（`contentText`）输入中文 → 保存为 `grapheme` 层边界
- Phoneme 编辑器加载后显示中文标签

**需求**：
- 保存时检查 `grapheme` 层内容是否含中文字符
- 如果含中文，弹出警告

### 26.2 Phoneme 加载时过滤非拼音内容

**需求**：
- `TextGridDocument::loadFromDsText()` 检测 grapheme 层边界文本是否包含 CJK 字符
- 如果包含，将层名标记为 `"grapheme (含中文)"` 并记录警告日志

**文件**：
- 修改：`src/apps/shared/data-sources/MinLabelPage.cpp`（`saveCurrentSlice()` 添加内容验证）
- 修改：`src/apps/shared/data-sources/PhonemeLabelerPage.cpp`（`onSliceSelectedImpl()` 添加内容过滤检测）

---

## 任务 27：Phoneme 标签区域多层级支持

### 27.1 PhonemeTextGridTierLabel 多行层级显示

**需求**：
- 标签区域改为多行显示，每行对应一个 tier
- 高度 = `kTierRowHeight * tierCount()`
- 层级按以下顺序排列：高层级在上方，低层级在下方
- 活跃层高亮，点击行切换活跃层

### 27.2 层级间关联辅助线

**需求**：
- 当拖动某层边界时，其他层同一时间位置的边界显示关联标记

### 27.3 IntervalTierView 层级视图同步

**需求**：
- 每个 tier 的 IntervalTierView 行高与标签行高一致
- 活跃层对应的 IntervalTierView 高亮

**文件**：
- 修改：`src/apps/shared/phoneme-editor/ui/PhonemeTextGridTierLabel.h/cpp`
- 修改：`src/apps/shared/phoneme-editor/ui/TierLabelArea.h/cpp`
- 修改：`src/apps/shared/phoneme-editor/PhonemeEditor.cpp`
- 修改：`src/apps/shared/phoneme-editor/ui/IntervalTierView.cpp`

---

## 任务依赖关系

```
23.1 (ASR回写修正)     ── 独立，无依赖
23.2 (LyricFA按钮)     ── 依赖 23.1
24.1 (参数区域结构化)  ── 独立，无依赖
24.2 (日志输出增强)    ── 独立，无依赖
24.3 (进度可视化增强)  ── 独立，无依赖
25.1 (FA输入源修正)    ── ✅ 已完成
26.1 (MinLabel内容验证) ── 依赖 23.1
26.2 (Phoneme内容过滤) ── 独立，无依赖
27.1 (标签区域多行)    ── 独立，无依赖
27.2 (层级关联辅助线)  ── 依赖 27.1
27.3 (层级视图同步)    ── 依赖 27.1
```

**执行顺序**：23.1 → [23.2, 24.1, 24.2, 24.3, 27.1] → [26.1, 27.2, 27.3] → 26.2

---

## 关联文档

- [human-decisions.md](human-decisions.md) — 人工决策记录（最高优先级）
- [unified-app-design.md](unified-app-design.md) — 应用设计方案
- [framework-architecture.md](framework-architecture.md) — 项目架构
- [conventions-and-standards.md](conventions-and-standards.md) — 编码规范
- [log-and-i18n-design.md](log-and-i18n-design.md) — Log 系统与本地化设计
