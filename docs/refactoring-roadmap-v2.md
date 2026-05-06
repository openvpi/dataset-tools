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
| **79** | **Log 侧边栏改为独立 LogPage（与 ADR-77 替代）** | **新** |
| 78 | Slicer/Phoneme 统一视图组合（D-30）：同一 UI + 组件自由显隐排序 | 有效 |

---

## 新任务：Slicer/Phoneme 统一视图组合系统

> 设计依据：[human-decisions.md §D-30](human-decisions.md)

### 8.1 AudioVisualizerContainer 添加 chart 显隐 API ✅

**需求**：container 增加 `setChartVisible(id, bool)` / `chartVisible(id)` 方法，chart 注册后不自动 show，由配置决定。

**文件**：`AudioVisualizerContainer.h/cpp`

### 8.2 Per-page 默认 chart 视图配置 ✅

**需求**：
- `SlicerPage`：waveform ✅、spectrogram ✅、power ❌（添加但不显示）
- `PhonemeEditor`：waveform ✅、power ✅、spectrogram ✅

**文件**：`SlicerPage.cpp`、`PhonemeEditor.cpp`

### 8.3 Settings 页面"视图布局"区域 ✅

**需求**：
- 带 checkbox 的 chart 列表（☑ waveform / ☑ power / ☑ spectrogram）
- 支持拖拽排序（QListWidget + DragDropMode）
- 排序结果即时持久化到 AppSettings → 实时通知 container 刷新布局
- "恢复默认"按钮清除用户覆盖

**文件**：Settings 页面（label-suite 和 ds-labeler 各一份）

### 8.4 chartOrder/chartVisible 全局配置持久化 ✅

**需求**：
- AppSettings key：`ViewLayout/chartOrder`（逗号分隔字符串）
- AppSettings key：`ViewLayout/chartVisible`（仅列出可见 chart）
- `AudioVisualizerContainer` 读取配置并应用 chart 显隐 + 顺序

**文件**：`AudioVisualizerContainer.cpp`

### 8.5 Phoneme 现有 toggle action 适配配置系统 ✅

**需求**：`m_actTogglePower` / `m_actToggleSpectrogram` 改为读写统一 AppSettings 配置，而非独立 toggle。

**文件**：`PhonemeEditor.cpp`

---

## 待修复

### B-02 Phoneme 非活跃层边界贯穿线显示 ✅

**问题**：未选中的层级的边界线在下方图表（waveform/spectrogram/power）中也显示为贯穿线。

**分析**：代码审查显示 `BoundaryOverlayWidget` 的逻辑正确（非活跃层 `lineBottom = tiers * tierRowH` 止于 tierLabel 区域内）。chart widget 自身 `paintEvent` 不绘制边界线。需要实际运行验证是否为渲染时序或 geometry 问题。

### B-03 Phoneme 波形图不显示

**问题**：PhonemeLabeler 页面加载切片后波形图没有显示。

**分析**：代码逻辑正确（`setAudioData` + `rebuildMinMaxCache` + deferred `fitToWindow`）。可能原因：(1) 保存的 splitter state 将 waveform 高度压为 0；(2) `fitToWindow` 延迟执行时序问题。需要清除 QSettings 中的 `Layout/editorSplitterState` 验证。

---

## 新任务：Log 系统升级 + Phoneme 边界修正（2026-05-07）

> 范围：4 个子任务，覆盖 Log UI 重构、FA 分层日志、边界重叠方案、拖拽实时刷新。

### 9.1 Log 侧边栏改为独立 LogPage（ADR-79） ✅

**现状**：`LogPanelWidget` 是 AppShell 内部 QSplitter 右侧折叠面板，由一个 QToolButton（Ctrl+L）控制显隐。Sidebar 仅显示 Log toggle 按钮。

**需求**：
- 新建 `LogPage`（实现 `IPageActions` + `IPageLifecycle`），作为独立页面注册到 AppShell 的图标导航栏
- 移除 AppShell 中的 `m_logPanel`、`m_contentSplitter`、`m_toggleLogAction`、corner widget、`toggleLogPanel()`、`isLogPanelVisible()`
- `LogPage` 复用现有 `LogViewer` + 筛选栏（category filter / clear / export）
- 注册到 LabelSuite 和 DsLabeler 两个 main.cpp（与其他页面并列）
- 保留 `Logger` / `LogNotifier` / ringBufferSink 基础设施不变

**文件**：
- 新建：`src/apps/shared/log-page/LogPage.h`、`LogPage.cpp`
- 修改：`src/framework/ui-core/include/dsfw/AppShell.h`、`src/framework/ui-core/src/AppShell.cpp`
- 修改：`src/apps/label-suite/main.cpp`、`src/apps/ds-labeler/main.cpp`
- 可废弃：`src/framework/ui-core/include/dsfw/LogPanelWidget.h`、`src/framework/ui-core/src/LogPanelWidget.cpp`

**ADR-79 生效后，ADR-77（Log 侧边栏全局化）视为废弃。**

---

### 9.2 FA 分层日志输出（层级从属信息追踪） ✅

**现状**：`runFaForSlice()` 仅输出 "FA started/completed/failed" + sliceId + phoneme 总数。`buildFaLayers()` 静默构建 tier，无日志。

**需求**：
- 新增 category `"fa"`，用于 FA 详细日志
- FA 过程中输出层级从属关系：
  - Grapheme：`"FA: [grapheme] word='你好' → [0.500s-1.200s]"`
  - Phoneme：`"FA: [phoneme] phone='n-ix' @ 0.500s (id=3, tier=phoneme)"`
  - Binding：`"FA: [bind] grapheme#2 ↔ phoneme#3 @ 0.500s"`
- 错误追踪：记录 FA pipeline 哪一步出错（音频加载、grapheme 解析、HFA 推理、layer 构建）
- 日志级别：grapheme/phoneme → Debug；binding → Trace；错误 → Error

**文件**：`src/apps/shared/data-sources/PhonemeLabelerPage.cpp`

---

### 9.3 Phoneme 多层边界重合时的拖拽激活方案 ✅

**根因**：4 个 widget 的 `hitTestBoundary()` 函数（`WaveformWidget`、`PowerWidget`、`SpectrogramWidget`、`IntervalTierView`）在遍历所有 tier 时使用 `b > bestIdx` 作为同像素位置的 tiebreaker——但 `b` 和 `bestIdx` 是不同 tier 的 boundary 索引，比较无意义，导致选择结果随机。

**需求**：实现基于"层级优先级 + 拖拽空间"的重叠边界选择方案：

1. **Active tier 优先**：如果重叠的边界中有活跃层的边界，优先激活该边界
2. **拖拽空间（drag-room）排第二**：计算每个重叠边界的可移动空间 `min(clamp右邻 - pos, pos - clamp左邻)`，选择空间最大的边界（最外侧的边界，方便向一侧拖动）
3. **拖拽空间相同时**：优先选择左边界（便于向右拖动扩展）

**适配范围**：
- `WaveformWidget::hitTestBoundary()` — 需要返回 `(tierIndex, boundaryIndex)`，增加 active tier 判断
- `PowerWidget::hitTestBoundary()` — 同上
- `SpectrogramWidget::hitTestBoundary()` — 同上（当前 `drawBoundaryOverlay` 也仅绘制 active tier）
- `IntervalTierView::hitTestBoundary()` — 单 tier 内部（同一 tier 的两个相邻边界可能在同一像素，同样需要处理）

**文件**：
- `src/apps/shared/phoneme-editor/ui/WaveformWidget.h`、`WaveformWidget.cpp`
- `src/apps/shared/phoneme-editor/ui/PowerWidget.h`、`PowerWidget.cpp`
- `src/apps/shared/phoneme-editor/ui/SpectrogramWidget.h`、`SpectrogramWidget.cpp`
- `src/apps/shared/phoneme-editor/ui/IntervalTierView.h`、`IntervalTierView.cpp`

---

### 9.4 Phoneme 拖动边界线时实时刷新所有 UI 组件 ✅

**根因**：`BoundaryDragController::dragging` 信号没有连接到任何 UI 刷新逻辑。UI 刷新完全依赖 `TextGridDocument::boundaryMoved` 信号，该信号路径做了全量刷新（包括昂贵的 `entryListPanel.rebuildEntries()` 在每一帧拖拽中都触发）。

**需求**：
1. 连接 `dragging` 信号到轻量级 UI 刷新路径（跳过 `rebuildEntries`），确保拖拽过程中 boundary overlay 和 chart overlay 实时响应
2. 将 `rebuildEntries()` 从 `boundaryMoved` 回调移到 `dragFinished` 回调（仅在拖拽结束后重建 entry 列表）
3. 使用 `AudioVisualizerContainer::invalidateBoundaryModel()` 统一驱动 UI 刷新，替代逐 widget 手动调用 `update()`

**信号连接修改**（`PhonemeEditor::connectSignals()`）：
```cpp
// 新：拖拽过程中轻量刷新
connect(dragCtrl, &BoundaryDragController::dragging, this,
    [this](int, int, TimePos) {
        updateAllBoundaryOverlays();  // overlay + chart overlay
        m_tierLabel->update();        // tier name labels
        m_tierEditWidget->update();   // interval tier views
    });

// 修改：boundaryMoved 不再重建 entries
connect(m_document, &TextGridDocument::boundaryMoved, this,
    [this](int, int, TimePos) {
        updateAllBoundaryOverlays();
        m_tierLabel->update();
    });

// 新：拖拽结束后重建 entries
connect(dragCtrl, &BoundaryDragController::dragFinished, this,
    [this](int, int, TimePos) {
        m_entryListPanel->rebuildEntries();
    });
```

**文件**：`src/apps/shared/phoneme-editor/PhonemeEditor.cpp`

---

### 任务依赖关系

```
9.1 (LogPage) ── 独立，无依赖
9.2 (FA 日志) ── 依赖 LogPage 完成（提供日志展示页面）
9.3 (边界重叠) ── 独立，与 9.4 可并行
9.4 (拖拽刷新) ── 独立，与 9.3 可并行
```

---

## 关联文档

- [human-decisions.md](human-decisions.md) — 人工决策记录（最高优先级）
- [unified-app-design.md](unified-app-design.md) — 应用设计方案
- [framework-architecture.md](framework-architecture.md) — 项目架构
- [conventions-and-standards.md](conventions-and-standards.md) — 编码规范
- [log-and-i18n-design.md](log-and-i18n-design.md) — Log 系统与本地化设计
