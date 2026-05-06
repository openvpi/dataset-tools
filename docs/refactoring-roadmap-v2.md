# 重构路线图

> 2026-05-06（更新于 2026-05-06）
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
| 77 | Log 侧边栏全局化（AppShell 层） | 有效 |
| 78 | Slicer/Phoneme 统一视图组合（D-30）：同一 UI + 组件自由显隐排序 | 有效 |

---

## 新任务：Slicer/Phoneme 统一视图组合系统

> 设计依据：[human-decisions.md §D-30](human-decisions.md)

### 8.1 AudioVisualizerContainer 添加 chart 显隐 API

**需求**：container 增加 `setChartVisible(id, bool)` / `chartVisible(id)` 方法，chart 注册后不自动 show，由配置决定。

**文件**：`AudioVisualizerContainer.h/cpp`

### 8.2 Per-page 默认 chart 视图配置

**需求**：
- `SlicerPage`：waveform ✅、spectrogram ✅、power ❌（添加但不显示）
- `PhonemeEditor`：waveform ✅、power ✅、spectrogram ✅

**文件**：`SlicerPage.cpp`、`PhonemeEditor.cpp`

### 8.3 Settings 页面"视图布局"区域

**需求**：
- 带 checkbox 的 chart 列表（☑ waveform / ☑ power / ☑ spectrogram）
- 支持拖拽排序（QListWidget + DragDropMode）
- 排序结果即时持久化到 AppSettings → 实时通知 container 刷新布局
- "恢复默认"按钮清除用户覆盖

**文件**：Settings 页面（label-suite 和 ds-labeler 各一份）

### 8.4 chartOrder/chartVisible 全局配置持久化

**需求**：
- AppSettings key：`ViewLayout/chartOrder`（逗号分隔字符串）
- AppSettings key：`ViewLayout/chartVisible`（仅列出可见 chart）
- `AudioVisualizerContainer` 读取配置并应用 chart 显隐 + 顺序

**文件**：`AudioVisualizerContainer.cpp`

### 8.5 Phoneme 现有 toggle action 适配配置系统

**需求**：`m_actTogglePower` / `m_actToggleSpectrogram` 改为读写统一 AppSettings 配置，而非独立 toggle。

**文件**：`PhonemeEditor.cpp`

---

## 待修复

### B-02 Phoneme 非活跃层边界贯穿线显示

**问题**：未选中的层级的边界线在下方图表（waveform/spectrogram/power）中也显示为贯穿线。

**分析**：代码审查显示 `BoundaryOverlayWidget` 的逻辑正确（非活跃层 `lineBottom = tiers * tierRowH` 止于 tierLabel 区域内）。chart widget 自身 `paintEvent` 不绘制边界线。需要实际运行验证是否为渲染时序或 geometry 问题。

### B-03 Phoneme 波形图不显示

**问题**：PhonemeLabeler 页面加载切片后波形图没有显示。

**分析**：代码逻辑正确（`setAudioData` + `rebuildMinMaxCache` + deferred `fitToWindow`）。可能原因：(1) 保存的 splitter state 将 waveform 高度压为 0；(2) `fitToWindow` 延迟执行时序问题。需要清除 QSettings 中的 `Layout/editorSplitterState` 验证。

### ~~B-04 Slicer 默认比例尺 3000 未生效~~ ✅ 已修复

**根因**：`fitToWindow()` 曾调用 `restoreResolution()` 从 QSettings 恢复旧会话保存的 50，覆盖了 `setDefaultResolution(3000)`。

**修复**：`fitToWindow()` 改为使用 `m_defaultResolution`；`restoreResolution()` 仅在 `onActivated()`（页面切换回来）时调用，新旧音频加载始终从默认分辨率开始。

### ~~B-05 Phoneme 右键播放未连接~~ ✅ 已修复

**实现**：`WaveformWidget::contextMenuEvent()` 和 `SpectrogramWidget::contextMenuEvent()` 均已实现 ADR-62（右键直接播放分段，不弹菜单）。两个 widget 都通过 `m_playWidget` 直接设置播放范围并启动播放。

### ~~B-06 Phoneme Bind/Snap 按钮选中状态区分不明确~~ ✅ 已修复

**修复**：Bind/Snap 按钮通过 `QToolButton` 的 `widgetForAction()` 获取 widget 并应用显式 QSS 样式（`:checked` 蓝色背景 + 白色文字，`!checked` 透明 + 灰色文字），视觉区分清晰。

### ~~6.3 快捷键系统~~ ✅ 已完成

| 页面 | 规划快捷键 | 实际状态 |
|------|-----------|---------|
| Slicer | S=自动切片, E=导出, I=导入切点, P=指针, K=切刀 | ✅ S/Shift+S/Ctrl+I/Ctrl+S/Ctrl+E 已补全（`createMenuBar()` 中 `setShortcut()`） |
| PhonemeLabeler | F=FA, ←/→=切换切片 | ✅ |
| PitchLabeler | F=提取音高, M=MIDI, ←/→=切换切片 | ✅ |
| MinLabel | R=ASR, ←/→=切换切片 | ✅ |

> ~~注：LabelSuite 版 SlicerPage 缺少 S/E/I 快捷键；DsLabeler 版 DsSlicerPage 已通过 `createMenuBar()` 中的 `setShortcut()` 补全。~~ 已同步补全。

### ~~7.7 Slicer/Phoneme 界面行为不统一~~ ✅ 已完成

- ✅ playhead 连接已统一到 `setPlayWidget()`
- ✅ splitter 状态延迟恢复已实现
- ✅ dragController 连接已统一：`addChart()` 中通过 QMetaMethod 自动传播到 chart widget
- ✅ Phoneme 右键分段播放（`contextMenuEvent` 在 WaveformWidget/SpectrogramWidget 中直接实现）

---

## 已完成任务记录

| 编号 | 任务 | 完成方式 |
|------|------|---------|
| 1.1 | TierEditWidget 位置调整 | `setEditorWidget()` 插入 chartSplitter 之前 |
| 1.2 | Slicer 同步缩放 | `connectViewportToWidget()` 自动连接 |
| 1.3 | PhonemeEditor 波形显示 | splitter 延迟恢复 |
| 1.4 | 视图菜单联动 | `visibleStateChanged` 信号 |
| 2.1 | resolution 比例尺重构 | ViewportState 统一 resolution + sampleRate |
| 2.2 | TimeRuler 查表法 | `kLevels[]` 14 档 + `findLevel()` |
| 2.3 | wheelEvent 统一 | Ctrl+wheel=zoom, plain=scroll |
| 2.4 | BoundaryDragController | 统一拖动逻辑，删除 BoundaryBindingManager |
| 3.1 | 最右边界拖动 | `GetMaxTime()` 作为上界 |
| 3.2 | 无数据提示 | "No label data" + `setFixedHeight` |
| 3.3 | 最近工程灰色标记 | gray + strikeOut |
| 3.4 | SliceNumberLayer 删除 | 文件已删除 |
| 3.5 | 音频有效性管理 | `audioExists()` / `validatedAudioPath()` / 信号 |
| 3.6 | audioSource 路径 fallback | wavs/ 启发式 + qWarning |
| 3.7 | dsproj 数据模型修复 | 去重 / erase defaults / 版本检查 |
| 4.1 | EditorPageBase 合并 | 三页面继承，公共生命周期 |
| 4.2 | 旧实现清理 | 无残留 |
| 5.1 | 刷新框架 | `invalidateBoundaryModel()` |
| 5.2 | FA 异步化 | `ensureHfaEngineAsync` + QtConcurrent |
| 5.3 | 推理非阻塞 | ExportPage `continueExport()` 异步 |
| 6.1 | PathCompat | `openSndfile()` / `openIfstream()` |
| 6.2 | QTableView 主题 | dark.qss + light.qss 完整样式 |
| 6.4 | 图表顺序 UI | SettingsPage QListWidget + 上下按钮 |
| 6.5 | CSV 预览 | ExportPage QTabWidget "预览数据" |
| 6.6 | Log 全局侧边栏 | LogPanelWidget + AppShell Ctrl+L 切换 |
| 7.1 | TimeRuler 刻度 | `syncStateFields()` 在 `clampAndEmit()` 前 |
| 7.2 | 播放游标 | `setPlayWidget()` 统一 playhead 连接 |
| 7.3 | 跨层 clamp | D-29 已移除（拖动期间位置失效导致不可用） |
| 7.4 | 跨层 hitTest | D-28，搜索所有层级 |
| 7.5 | splitter 恢复 | 延迟到 resizeEvent |
| 7.6 | 波形图缺失 | 同 7.5 |
| B-01 | 绑定边界拖动 | 移除跨层 clamp；binding 使用 word.start 保证时间对齐 |
| B-05 | FA SP 过滤 | 不再过滤 SP/AP，完整保留 FA 输出 |
| B-04 | Slicer 默认比例尺 | `fitToWindow()` 使用 `m_defaultResolution`，`restoreResolution()` 仅 onActivated |
| B-05 | Phoneme 右键播放 | `contextMenuEvent` 在 WaveformWidget/SpectrogramWidget 中直接播放分段 |
| B-06 | Bind/Snap 按钮样式 | QToolButton QSS `:checked`/`!checked` 样式 |
| 6.3 | Slicer 快捷键补全 | `createMenuBar()` 中 S/Shift+S/Ctrl+I/Ctrl+S/Ctrl+E 快捷键 |
| 7.7 | dragController 统一 | `addChart()` QMetaMethod 自动传播；移除 PhonemeEditor 显式调用 |

---

## 关联文档

- [human-decisions.md](human-decisions.md) — 人工决策记录（最高优先级）
- [unified-app-design.md](unified-app-design.md) — 应用设计方案
- [framework-architecture.md](framework-architecture.md) — 项目架构
- [conventions-and-standards.md](conventions-and-standards.md) — 编码规范
- [log-and-i18n-design.md](log-and-i18n-design.md) — Log 系统与本地化设计
