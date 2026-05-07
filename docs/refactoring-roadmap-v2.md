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
| 86 | AudioVisualizerContainer 支持移除 TierLabelArea（D-37） | 有效 |
| 87 | 文件列表面板分层设计，按钮风格统一（D-38） | 有效 |

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
| 23.1 MinLabel ASR 结果回写目标修正 | 2026-05-07 |
| 23.2 MinLabel 添加 LyricFA 匹配按钮 | 2026-05-07 |
| 24.1 BatchProcessDialog 参数区域结构化 | 2026-05-07 |
| 24.2 BatchProcessDialog 日志输出增强 | 2026-05-07 |
| 24.3 BatchProcessDialog 进度可视化增强 | 2026-05-07 |
| 27.1 PhonemeTextGridTierLabel 多行层级显示 | 2026-05-07 |
| 26.1 MinLabel 保存时验证 grapheme 层内容 | 2026-05-07 |
| 27.2 层级间关联辅助线 | 2026-05-07 |
| 27.3 IntervalTierView 层级视图同步 | 2026-05-07 |
| 26.2 Phoneme 加载时过滤非拼音内容 | 2026-05-07 |

---

## 任务 28：Phoneme 界面删除 tierline 区域

**需求**：phoneme 界面的 `PhonemeTextGridTierLabel` 层级标签区域应从框架中移除。

**设计依据**：D-37

**方案**：
- `AudioVisualizerContainer` 新增 `removeTierLabelArea()` 方法，从布局中移除 `m_tierLabelArea`
- `PhonemeEditor::buildLayout()` 末尾调用 `removeTierLabelArea()`
- `updateOverlayTopOffset()` 处理 `m_tierLabelArea == nullptr`

**文件**：
- `src/apps/shared/audio-visualizer/AudioVisualizerContainer.{h,cpp}`
- `src/apps/shared/phoneme-editor/PhonemeEditor.cpp`

---

## 任务 29：HFA 对齐结果修复——与 main 分支对齐

**需求**：HFA 对齐结果严重错误，参考 main 分支代码修复。数据处理过程、算法和模型推理部分要求完全一致。

**分析**：
- `hubert-infer` 核心算法（HfaModel → AlignmentDecoder → NonLexicalDecoder → 后处理）与 main 分支一致
- 问题可能在 `buildFaLayers()` 对 `WordList` 边界解释方式，需逐函数对比验证
- 需要确认 `AlignmentDecoder::decode()`、`NonLexicalDecoder::decode()`、`fill_small_gaps()`、`add_SP()` 与 main 分支完全一致

**文件**：
- `src/infer/hubert-infer/src/Hfa.cpp`
- `src/infer/hubert-infer/src/AlignmentDecoder.cpp`
- `src/infer/hubert-infer/src/NonLexicalDecoder.cpp`
- `src/apps/shared/data-sources/PhonemeLabelerPage.cpp`（`buildFaLayers()`）

---

## 任务 30：修复无边框窗口标题截断

**需求**：部分页面标题过长，无边框窗口不能显示完整标题，最前面几个字缺失。

**根因**：`TitleBar::resizeEvent()` 中 `m_titleLabel->setGeometry(0, 0, width(), height())` 覆盖全宽，标题居中对齐后与左侧 menuBar 和右侧窗口按钮重叠。

**方案**：在 `TitleBar::resizeEvent()` 中计算可用宽度：
1. 获取 menuBar 的 `sizeHint().width()`（若存在）
2. 获取右侧按钮总宽度（`m_minBtn + m_maxBtn + m_closeBtn` 各 46px）
3. 标题居中在 `[menuBarEnd, buttonsStart]` 区域内

**文件**：
- `src/framework/ui-core/src/FramelessHelper.cpp`（`TitleBar` 类）

---

## 任务 31：音高页面无标注时也能播放音频

**需求**：音高界面切换条目时，右侧一致显示"请打开文件"。无标注信息时、也应该能打开播放音频。

**根因**：`PitchLabelerPage::onSliceSelectedImpl()` 中如果 `m_currentFile` 为空（无 pitch/MIDI 数据），虽然调用了 `m_editor->loadAudio()`，但音频加载逻辑受条件守卫限制。

**方案**：确保 `loadAudio()` 在音频路径可用时无条件执行，不依赖 `m_currentFile` 状态。

**文件**：
- `src/apps/shared/data-sources/PitchLabelerPage.cpp`

---

## 任务 32：导出页面 CSV 预览深浅色主题修复

**需求**：导出页面的 CSV 预览表格，浅色主题下是肉色背景 + 灰色文字，参考 CLion 样式修复深浅色主题。

**问题**：
1. 全局 QSS 给 `QTableView` 的 `alternate-background-color: #F0F2F5` 在浅色主题下呈现肉色感
2. 缺失数据行使用硬编码 `QColor(255, 200, 200)` 浅红，不跟随主题

**方案**：
- 为 `m_previewTable` 设置 `setObjectName("csvPreviewTable")`，在 QSS 中添加专属样式
- 使用 `Theme::Palette` 的颜色变量替代硬编码
- 参考 CLion 配色：深色 `#2B2B2B` + `#A9B7C6`，浅色 `#FFFFFF` + `#313131`
- 缺失数据行改用 `palette.warning` 的半透明叠加

**文件**：
- `src/apps/ds-labeler/ExportPage.cpp`
- `src/framework/ui-core/res/themes/dark.qss`
- `src/framework/ui-core/res/themes/light.qss`

---

## 任务 33：Slicer 文件列表按钮修复（无文字显示）

**需求**：slicer 文件列表上方按钮背后多了几个无文字显示的按钮。

**根因**：`DroppableFileListPanel` 中 `m_btnAddDir` 使用 emoji "📁"、`m_btnRemove` 使用 "−" (U+2212)，在部分 Windows 字体/主题下不渲染。

**方案**：
- `m_btnAddDir`、`m_btnAdd`、`m_btnRemove` 改为 `QToolButton`，加载 SVG 资源图标
- `m_btnDiscard`/`m_btnClear` 保留文本，设为中文（"丢弃"/"清除"）
- `AudioFileListPanel` 可重写按钮文本行为

**文件**：
- `src/framework/widgets/src/DroppableFileListPanel.cpp`
- `src/apps/shared/data-sources/AudioFileListPanel.h`

---

## 任务 34：文件列表统一与分页面功能启用

**需求**：各页面的文件列表应该是一个类的不同实例，根据各页面特点启用部分功能。

**设计依据**：D-38

**方案**：
- `DroppableFileListPanel` 新增 `setButtonVisible(QString id, bool)` 控制各按钮显隐
- `SliceListPanel` 是否启用按钮栏由模式参数控制
- `AudioFileListPanel` 使用新图标按钮

**文件**：
- `src/framework/widgets/include/dsfw/widgets/DroppableFileListPanel.h`
- `src/framework/widgets/src/DroppableFileListPanel.cpp`
- `src/apps/shared/data-sources/SliceListPanel.{h,cpp}`
- `src/apps/shared/data-sources/AudioFileListPanel.h`

---

## 任务 35：Phoneme 刻度线显示修复

**需求**：phoneme 页面的刻度线显示有问题，默认比例尺看不懂文字。要求 slicer 和 phoneme 在底层是同一个类的不同实例（已满足 D-30），为何行为不一致。

**分析**：两者使用同一 `AudioVisualizerContainer` + 同一 `TimeRulerWidget` + 同一 `ViewportController`。差异点：
- Slicer 默认 resolution = 3000 spx
- PhonemeEditor 默认 resolution = 800 spx
- `updateScaleIndicator()` 计算 `msPerDiv = resolution / sampleRate * 80 * 1000`

**可能原因**：
1. `TimeRulerWidget::findLevel()` 对 800 spx (44100Hz → ~55 PPS) 选择了不合适的刻度级别
2. `updateScaleIndicator()` 的格式化输出（"ms/div" 数字看不懂）

**方案**：
- 检查 `TimeRulerWidget::kLevels[]` 刻度级别表是否覆盖 800 spx 场景
- 调整 `kMinMinorStepPx` 阈值（当前 60px）或添加中间级别
- `updateScaleIndicator()` 格式化增加更多精度或改用 "spx" 为主标注

**文件**：
- `src/framework/widgets/src/TimeRulerWidget.cpp`
- `src/apps/shared/audio-visualizer/AudioVisualizerContainer.cpp`

---

## 任务依赖关系

```
28 (tierline移除)     ── 独立，无依赖
29 (HFA修复)          ── 独立，无依赖
30 (标题截断)          ── 独立，无依赖
31 (音高播放)          ── 独立，无依赖
32 (CSV主题)           ── 独立，无依赖
33 (slicer按钮)        ── 独立，无依赖
34 (文件列表统一)      ── 依赖 33（按钮改造前置）
35 (刻度线显示)        ── 独立，无依赖
```

**执行顺序**：33 → 34, 其余 [28, 29, 30, 31, 32, 35] 并行。

---

## 关联文档

- [human-decisions.md](human-decisions.md) — 人工决策记录（最高优先级）
- [unified-app-design.md](unified-app-design.md) — 应用设计方案
- [framework-architecture.md](framework-architecture.md) — 项目架构
- [conventions-and-standards.md](conventions-and-standards.md) — 编码规范
- [log-and-i18n-design.md](log-and-i18n-design.md) — Log 系统与本地化设计
- [refactoring-plan-v3.md](refactoring-plan-v3.md) — 本次重构详细方案
