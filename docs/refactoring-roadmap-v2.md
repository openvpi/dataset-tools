# 重构路线图

> 2026-05-06
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

---

## 1. 显示与 UX 修复

### 1.1 PhonemeLabeler 标签层位置调整 ✅

**问题**：PhonemeEditor 中 TierEditWidget（interval 文本编辑控件）作为 chart 添加到 chartSplitter 中（stretchFactor=0，heightWeight=0），实际不可见。标签层（TierEditWidget）应该在 TimeRuler 刻度线下方、Waveform 波形图上方。

**设计**：当前容器布局为 MiniMap → TimeRuler → TierLabelArea → chartSplitter(charts)。PhonemeTextGridTierLabel（tier 选择 radio buttons）已正确位于 TierLabelArea 位置。TierEditWidget 需要从 chartSplitter 中移出并插入到 TimeRuler 下方、chartSplitter 上方。

**修复**：✅ `AudioVisualizerContainer::setEditorWidget()` 将 TierEditWidget 插入到 chartSplitter 之前。`PhonemeEditor::buildLayout()` 调用 `m_container->setEditorWidget(m_tierEditWidget)`。BoundaryOverlay 的 `setExtraTopOffset()` 正确追踪 editor widget 高度变化。

### 1.2 Slicer 波形图与刻度线同步缩放 ✅

**问题**：SlicerPage 的 WaveformWidget/SpectrogramWidget 在缩放时没有与刻度线同步。所有应用应使用统一的 AudioVisualizerContainer 显示框架，由 ViewportController 驱动所有子图缩放。

**修复**：✅ `AudioVisualizerContainer::connectViewportToWidget()` 通过 QMetaMethod::invoke 自动连接所有 `addChart()` 注册的 widget 到 `viewportChanged` 信号。TimeRuler 在构造时直接连接 ViewportController。SlicerPage 使用同一框架。

### 1.3 PhonemeEditor 波形图显示异常 ✅

**问题**：PhonemeLabeler 页面中波形图没有正常显示，而 SlicerPage 使用同一套 AudioVisualizerContainer 框架却正常显示。

**修复**：✅ 同 7.5/7.6 — 根因是 splitter 状态在 widget 高度=0 时恢复。已通过 `AudioVisualizerContainer::restoreSplitterState()` 延迟到 `resizeEvent` 中 widget 有有效尺寸时执行解决。

### 1.4 PhonemeEditor 视图菜单状态与实际图状态联动 ✅

**问题**：PhonemeLabeler 页面"视图"菜单中 "Show Power" / "Show Spectrogram" 等选项的 checked 状态没有和实际图表的可见性联动。

**修复**：✅ `PowerWidget`/`SpectrogramWidget` 在 `showEvent`/`hideEvent` 中 emit `visibleStateChanged(bool)` 信号。`PhonemeEditor::connectSignals()` 将该信号连接到 action `setChecked()` 和 `powerVisibilityChanged`/`spectrogramVisibilityChanged` 信号。外部可见性变化（splitter 拖拽等）自动同步到菜单项。

### 1.5 HFA 对齐意外导出自动添加的 SP ✅

**问题**：HFA::recognize() 自动在头尾添加 SP 音素，导出到数据集时出现意外的头尾 SP 标注。

**修复**：✅ `buildFaLayers()` 使用 `isSilenceWord()` 过滤 word.text == "SP" || "AP" 的词。phoneme 层也跳过 phone.text == "SP" || "AP" 的音素。只有非静音词和音素被导出到结果层。

---

## 2. 视口与刻度系统重构

### 2.1 比例尺从固定列表改为整十对数表 + 废弃 PPS（D-26）

> 参考 human-decisions.md D-26。

#### 问题

1. 当前 `kResolutionTable = {10, 20, 30, 40, 60, 80, 100, 150, 200, 300, 400}` 最大 400，44100Hz 音频在 1200px 视口只能显示 ~110s，**无法显示几分钟的音频**。
2. `pixelsPerSecond` 作为概念散落在 ViewportState、ViewportController 和十几个 widget 中，与 resolution 重复表达同一状态，增加认知负担。

#### 设计

**废弃 PPS，统一使用 resolution（比例尺）**：

```cpp
// ViewportState 改为：
struct ViewportState {
    double startSec = 0.0;
    double endSec = 10.0;
    int resolution = 40;    // samples per pixel（唯一缩放量）
    int sampleRate = 44100; // 需要 sampleRate 才能做 time↔pixel 转换
};

// 所有 widget 中的 m_pixelsPerSecond 成员变量 → 删除
// time↔pixel 转换统一用：
//   pixelX = (timeSec * sampleRate / resolution) - scrollOffset
//   timeSec = (pixelX + scrollOffset) * resolution / sampleRate
```

**整十对数步进表**（替代旧的 11 档固定表）：

```cpp
static const std::vector<int> kResolutionTable = {
    10, 20, 30, 50, 80, 100, 150, 200, 300, 500, 800, 1000,
    1500, 2000, 3000, 5000, 8000, 10000, 15000, 20000, 30000, 44100
};
// 22 档，覆盖精细编辑（10 spx ≈ 4410 px/s）到全局概览（44100 spx = 1 px/s）
```

**每页默认 resolution**（fitToWindow 时使用）：

| 页面 | 目标视口时长 | 计算值 @44100Hz/1200px | 取表中最近值 |
|------|------------|----------------------|------------|
| Slicer | ~2 min | 4410 | **5000** |
| Phoneme | ~20 sec | 735 | **800** |

**每页 resolution 持久化**：
- `AudioVisualizerContainer` 在 `m_settings` 中保存/恢复 `"Viewport/resolution"` 键
- 各实例通过 `settingsAppName`（"Slicer" / "PhonemeEditor"）隔离

#### 实施步骤

**Phase A：ViewportController + ViewportState 改造**

- [ ] `ViewportState`：删除 `pixelsPerSecond` 字段，新增 `resolution` 和 `sampleRate` 字段
- [ ] `ViewportController`：删除 `setPixelsPerSecond()`、`pixelsPerSecond()` 方法
- [ ] `ViewportController`：删除 `updatePPS()` 内部方法
- [ ] `ViewportController`：`kResolutionTable` 替换为 22 档整十对数表
- [ ] `ViewportController`：`zoomIn()`/`zoomOut()` 在新表中前后移动一格
- [ ] `ViewportController`：`setResolution()` snap 到表中最近值
- [ ] `ViewportController`：`clampAndEmit()` 中 emit 的 `ViewportState` 包含 `resolution` 和 `sampleRate`

**Phase B：所有 Widget 适配**

- [ ] `WaveformWidget`：`m_pixelsPerSecond` → 删除，`timeToX()`/`xToTime()` 改用 `m_resolution`+`m_sampleRate`
- [ ] `SpectrogramWidget`：同上
- [ ] `PowerWidget`：同上
- [ ] `IntervalTierView`：同上
- [ ] `TierEditWidget`：`m_pixelsPerSecond` → 删除
- [ ] `TierLabelArea`：`timeToX()` 改用 resolution
- [ ] `TimeRulerWidget`：`m_pixelsPerSecond` → 删除，刻度计算改用 resolution
- [ ] `MiniMapScrollBar`：删除 `setPixelsPerSecond()` 调用

**Phase C：PhonemeEditor / PitchEditor 适配**

- [ ] `PhonemeEditor`：删除 `setPixelsPerSecond()`，删除 `zoomChanged(double pixelsPerSecond)` 信号（或改为 `zoomChanged(int resolution)`）
- [ ] `PianoRollView`：`m_hScale`（原 PPS）→ 改用 resolution
- [ ] `PitchEditor`：`setPixelsPerSecond(100.0)` → `setResolution(对应值)`

**Phase D：默认值 + 持久化**

- [ ] `SlicerPage`：`setDefaultResolution(5000)` 替代原 `setDefaultResolution(60)`
- [ ] `PhonemeEditor`：`setDefaultResolution(800)` 替代原 `setDefaultResolution(40)`
- [ ] `AudioVisualizerContainer`：`onActivated()` 恢复保存的 resolution；页面离开时保存
- [ ] 比例尺指示器文本改为 `"{N} spx"` 或 `"{time}/div"`

**Phase E：测试修复**

- [ ] `TestDsfwWidgets.cpp`：删除 `pixelsPerSecond()` 相关断言，改为 resolution 断言

**验证矩阵**

- [ ] 编译通过，无 `pixelsPerSecond` 残留引用
- [ ] Slicer 打开 5 分钟音频 → 视口显示约 2 分钟
- [ ] Phoneme 打开切片 → 视口显示约 20 秒
- [ ] Ctrl+滚轮 zoom out 到极限（resolution=44100）→ 可显示 20 分钟，硬停
- [ ] Ctrl+滚轮 zoom in 到极限（resolution=10）→ 硬停
- [ ] 切换页面后回来，resolution 恢复为上次离开时的值
- [ ] 两个页面的 resolution 互不影响

### 2.2 TimeRulerWidget 刻度计算查表法 ✅

✅ `TimeRulerWidget.cpp` 使用 `kLevels[]` 14 档查表 + `findLevel(pps)` 选择合适级别。主刻度长线+时间标签（自动选择 ms/s/min:sec/h:min:sec），次刻度短线无标签。

### 2.3 wheelEvent 统一 ✅

✅ 所有子图（WaveformWidget、SpectrogramWidget、PowerWidget）Ctrl+滚轮 → `zoomAt(centerSec, factor)`，无 modifier → 横向滚动。MiniMapScrollBar wheelEvent：无 modifier = 横向滚动，Ctrl = 缩放。

### 2.4 Phoneme 标签区域层级边界绑定与吸附重设计（D-27）

> 参考 human-decisions.md D-27。

#### 问题总结

1. **合成 ID 方案脆弱**：`tier*10000+index+1` 在边界增删后失效
2. **拖动逻辑四处重复**：WaveformWidget/SpectrogramWidget/PowerWidget/IntervalTierView 各有 ~90 行完全相同的 startDrag/updateDrag/endDrag 代码（违反 P-01）
3. **snap 方向限制**：`snapToLowerTier()` 只向 tierIndex 更小的层吸附
4. **BoundaryBindingManager 职责不清**：只是 findAlignedBoundaries + createLinkedMoveCommand 两个方法的容器

#### 新架构：BoundaryDragController

将四个 widget 中重复的拖动逻辑**提取**到 `BoundaryDragController`，由 `AudioVisualizerContainer` 持有。各 widget 只做 hit-test + 鼠标事件转发。

```
Widget.mousePressEvent → container->dragController()->startDrag(tier, idx)
Widget.mouseMoveEvent  → container->dragController()->updateDrag(time)
Widget.mouseReleaseEvent → container->dragController()->endDrag(time, undoStack)
```

#### 实施步骤

**Step 1：创建 BoundaryDragController**

- [ ] 新建 `src/apps/shared/phoneme-editor/ui/BoundaryDragController.h/cpp`
- [ ] 实现 `startDrag(tier, boundaryIndex, IBoundaryModel*)`：实时搜索时间容差内的 partners
- [ ] 实现 `updateDrag(proposedTime)`：clamp source + clamp each partner + preview move
- [ ] 实现 `endDrag(finalTime, QUndoStack*)`：snap → restore → push LinkedMoveBoundaryCommand
- [ ] 实现 `cancelDrag()`：restore all to original positions
- [ ] 实现 `setBindingEnabled(bool)`、`setSnapEnabled(bool)`、`setToleranceMs(double)`

**Step 2：IBoundaryModel 接口调整**

- [ ] `IBoundaryModel::snapToLowerTier()` 重命名为 `snapToNearestBoundary(tierIndex, proposedTime, thresholdUs)` 并搜索**所有其他层**
- [ ] `TextGridDocument` 实现新的 `snapToNearestBoundary()`
- [ ] `SliceBoundaryModel`：单层无需 snap，返回 proposedTime

**Step 3：AudioVisualizerContainer 集成**

- [ ] `AudioVisualizerContainer` 新增 `BoundaryDragController *m_dragController` 成员
- [ ] 构造函数中创建 dragController
- [ ] 暴露 `dragController()` 访问器

**Step 4：Widget 拖动代码清理**

- [ ] `WaveformWidget`：删除 startBoundaryDrag/updateBoundaryDrag/endBoundaryDrag + m_bindingMgr + m_dragAligned + m_dragAlignedStartTimes 等成员；mousePressEvent 中调用 container->dragController()
- [ ] `SpectrogramWidget`：同上
- [ ] `PowerWidget`：同上
- [ ] `IntervalTierView`：删除拖动代码，改为调用 controller（注意：IntervalTierView 的 container 是通过 TierEditWidget 间接访问）

**Step 5：PhonemeEditor 适配**

- [ ] `PhonemeEditor`：删除 `m_bindingManager` 成员
- [ ] toolbar 中 binding/snap toggle 改为调用 `m_container->dragController()->setBindingEnabled()`
- [ ] 删除 `#include "ui/BoundaryBindingManager.h"`

**Step 6：删除 BoundaryBindingManager**

- [ ] 删除 `BoundaryBindingManager.h/cpp`
- [ ] 更新 CMakeLists.txt 移除这两个文件
- [ ] `BoundaryCommands.cpp` 删除 `#include "../BoundaryBindingManager.h"`（如果 LinkedMoveBoundaryCommand 不需要）

**Step 7：验证**

- [ ] 编译通过，无 BoundaryBindingManager 残留引用
- [ ] 拖动 word 层边界 → phoneme 层对应位置的边界跟随移动
- [ ] 释放边界 → 自动吸附到最近的其他层边界（阈值 5px）
- [ ] 插入边界后立即拖动 → binding 仍正常（不依赖预存 group）
- [ ] 删除边界后拖动其他边界 → 不崩溃，binding 正确
- [ ] 禁用 binding → 拖动不联动，释放时仍 snap（如 snap 开启）
- [ ] 禁用 snap → 释放时不吸附
- [ ] Undo/Redo linked move → 所有关联边界正确恢复
- [ ] FA 完成后 → 边界自动对齐，拖动联动正常

---

## 3. 数据模型与边界修复

### 3.1 最右侧边界线无法拖动 ✅

**问题**：TextGridDocument::clampBoundaryTime 中最后一条边界的 nextBoundary 取了自身 interval 的 max_time，等于边界本身，clamp 区间为 0。

**修复**：✅ `TextGridDocument::clampBoundaryTime()` 中 `boundaryIndex < count` 时取 interval max_time，否则取 `tier->GetMaxTime()`。`SliceBoundaryModel::clampBoundaryTime()` 同理使用 `m_durationSec`。

### 3.2 Phoneme 标签区域无数据时显示提示 ✅

**问题**：`PhonemeTextGridTierLabel` 在 tierCount == 0 时高度为 0，标签区域消失。

**修复**：✅ 构造时 `setFixedHeight(kTierRowHeight)` 始终保持最小高度。`paintEvent()` 在 tierCount==0 时绘制居中灰色文字 "No label data"。FA 运行时显示 "Aligning..."。

### 3.3 最近工程列表标灰无效路径 ✅

**修复**：✅ `WelcomePage::refreshRecentList()` 对每个路径检查 `QFileInfo::exists()`，不存在的路径设置灰色前景 + 删除线字体。双击不存在的路径弹出确认删除对话框。

### 3.4 删除废弃的 SliceNumberLayer ✅

✅ 文件已删除，CMakeLists.txt 无残留引用，代码库中无任何 include 或引用。

### 3.5 IEditorDataSource 层统一音频文件有效性管理 ✅

**修复**：✅ `IEditorDataSource` 已有 `audioExists()`（虚方法，默认 QFile::exists）、`validatedAudioPath()`、`missingAudioSlices()`、`audioAvailabilityChanged` 信号。`SliceListPanel::refresh()` 对缺失音频条目设置橙色前景 + warning 图标 + tooltip。PhonemeLabelerPage/PitchLabelerPage/MinLabelPage 的推理调用点使用 `validatedAudioPath()`。批量操作报告跳过数。

### 3.6 dsproj audioSource 路径健壮性 ✅

✅ `ProjectDataSource::sliceAudioPath()` 在主路径不存在时尝试 `wavs/<sliceId>.wav` 启发式 fallback + `qWarning()` 日志。audioSource 通过 `toPosixPath`/`fromPosixPath` 存储为 POSIX 格式相对路径。

### 3.7 dsproj 数据模型其他问题 ✅

✅ ExportConfig 格式：`load()` 末尾 `proj.m_exportConfig.formats.removeDuplicates()`。defaults round-trip：`save()` 中 `json.erase("defaults")`。slice.in/out：Slice 结构体有 `///` 注释说明微秒语义。版本校验：`load()` 中检查版本前缀并 `qWarning`。

---

## 4. 编辑器页面去重

### 4.1 激活 EditorPageBase，合并确定性重复代码 ✅

✅ `EditorPageBase` 实现完整：公共成员（m_sliceList、m_splitter、m_source 等）、`setupBaseLayout()`、`setupNavigationActions()`、生命周期方法（onActivated/onDeactivating/onDeactivated/onShutdown）、`setDataSource()`、`windowTitle()`、`maybeSave()`、`loadEngineAsync()`。MinLabelPage、PhonemeLabelerPage、PitchLabelerPage 均继承 EditorPageBase，通过虚方法钩子实现页面特定逻辑。

### 4.2 删除废弃的 EditorPageBase 旧实现 ✅

✅ 无旧的英文 `createMenuBar()` 或简陋 `maybeSave()` 残留。当前实现为唯一版本。

---

## 5. 推理引擎优化

### 5.1 AudioVisualizerContainer 文档/模型刷新框架 ✅

**修复**：✅ `invalidateBoundaryModel()` 刷新 overlay + tierLabelArea + 所有 chart widgets。`boundaryModelInvalidated()` 信号已声明。`TierLabelArea` 有虚方法 `onModelDataChanged()`，`PhonemeTextGridTierLabel` override 后调用 `update()`。PhonemeEditor 中 `documentChanged` → `invalidateBoundaryModel()`。

### 5.2 FA 自动对齐非阻塞化 + 状态指示 ✅

✅ `ensureHfaEngineAsync()` 拆分为同步检查 + QTimer::singleShot 延迟加载。FA 通过 `QtConcurrent::run` 在后台线程运行，QPointer 保护页面生命周期。`PhonemeTextGridTierLabel::setAlignmentRunning(bool)` 显示 "Aligning..." 指示。FA 结果通过 `QMetaObject::invokeMethod` 回到主线程。

### 5.3 所有自动推理/补全操作非阻塞化 ✅

✅ `EditorPageBase::loadEngineAsync()` 通用方法已实现。PhonemeLabelerPage `ensureHfaEngineAsync()`、PitchLabelerPage `ensureRmvpeEngineAsync()` / `ensureGameEngineAsync()`、MinLabelPage `ensureAsrEngineAsync()` 均为异步版本。ExportPage `onExport()` 中的 autoComplete 循环已移入 `QtConcurrent::run`，通过 `continueExport()` 在主线程继续导出流程。无 `QApplication::processEvents()` 调用。

---

## 6. 路径与 UI 完善

### 6.1 统一路径管理模块 ✅

✅ `audio-util/include/audio-util/PathCompat.h` 已创建，提供 `openSndfile(path)`、`openIfstream(path)`、`openOfstream(path)`。Windows 使用 wstring API 确保 CJK/空格路径正确。`dstools::toFsPath` 使用 wstring（Win）/ string（Unix）。

### 6.2 CSV 预览暗/明主题适配 ✅

✅ `dark.qss` 和 `light.qss` 均包含完整 QTableView 样式（background、alternate-background、color、gridline-color、selection、hover、item padding）和 QHeaderView::section 样式。

### 6.3 快捷键系统 ⚠️ 部分完成

| 页面 | 规划快捷键 | 实际状态 |
|------|-----------|---------|
| Slicer | S=自动切片, E=导出, I=导入切点, P=指针, K=切刀 | ⚠️ 只有 V=指针, C=切刀，缺少 S/E/I |
| PhonemeLabeler | F=FA, ←/→=切换切片 | ✅ F=FA, ←/→ 通过 EditorPageBase |
| PitchLabeler | F=提取音高, M=MIDI, ←/→=切换切片 | ✅ F=extractPitch, M=extractMidi, ←/→ |
| MinLabel | R=ASR, ←/→=切换切片 | ✅ R=ASR, ←/→ |

**剩余**：SlicerPage 需要通过 ShortcutManager 绑定 S=自动切片、E=导出、I=导入切点，并将 V→P、C→K 调整为规划键位。

### 6.4 子图顺序配置 UI ✅

✅ SettingsPage 中包含"图表顺序" QListWidget，带上移/下移按钮，保存到 `AudioVisualizer/chartOrder` QSettings 键。

### 6.5 DsLabeler CSV 预览 ✅

✅ ExportPage 内部 QTabWidget 包含 "预览数据" 标签页，QTableView + QStandardItemModel（只读）。`refreshPreview()` 填充所有切片的标注数据。

---

## 执行优先级

1. **2.1** — 比例尺连续范围改造 ✅ 已完成
2. **2.4** — Phoneme 标签 binding/snap 重设计 ✅ 已完成
3. **7.x** — 后续 Bug 修复批次 ✅ 已完成
4. **1.1-1.5** — 显示与 UX 修复 ✅ 已完成
5. **3.1、3.2、3.4** — 简单边界/标签修复 ✅ 已完成
6. **5.1** — 刷新框架 ✅ 已完成
7. **2.2、2.3** — 刻度查表法 + wheelEvent 统一 ✅ 已完成
8. **3.5、3.6、3.7** — 数据健壮性 ✅ 已完成
9. **4.1、4.2** — 编辑器去重 ✅ 已完成
10. **5.2、5.3** — 推理异步化 ✅ 已完成
11. **6.1** — 路径管理 ✅ 已完成
12. **6.2、6.4、6.5、3.3** — 低优先级完善 ✅ 已完成
13. **6.3** — 快捷键系统 ⚠️ Slicer 页面键位未完成（缺 S/E/I，V/C 与规划键位 P/K 不一致）

---

## 7. 后续 Bug 修复批次

### 7.1 Phoneme 刻度显示异常 ✅

**问题**：PhonemeEditor 加载音频后，TimeRulerWidget 的刻度间距/标签不正确。

**修复**：✅ `ViewportController::setAudioParams()` 在 `clampAndEmit()` 之前调用 `syncStateFields()` 确保 sampleRate 正确广播。`AudioVisualizerContainer::fitToWindow()` 在 widget 宽度=0 时设置 `m_needsFitOnResize=true`，在 `resizeEvent` 中延迟执行。

### 7.2 Slicer 右键播放无红色游标

**问题**：SlicerPage 右键播放音频片段时没有显示红色竖线播放游标。

**根因**：SlicerPage 独立组装，缺少 playhead 信号连接到 waveform 和自动清除超时。PhonemeEditor 有此逻辑但未复用。

**修复**：✅ `AudioVisualizerContainer::setPlayWidget()` 统一处理：自动连接 playhead 到所有有 `setPlayhead` 方法的 chart widget（通过 QMetaMethod），包含 200ms 自动清除超时。SlicerPage 和 PhonemeEditor 的 per-page 连接已删除。

### 7.3 Phoneme 层级边界约束失效（跨层 clamp）✅

**问题**：低层级（如 phoneme）的边界拖动时可以越过高层级（如 grapheme）的边界，违反 D-17 层级包含规则。

**修复**：✅ 跨层约束直接内联在 `TextGridDocument::clampBoundaryTime()` 中（D-29 注释）：对 tierIndex > 0 的层，找到 parentTier（tierIndex-1）中包含当前边界位置的区间，将 clamp 范围收紧到父区间边界内。未采用独立 `clampToParentTier()` 虚方法方案，效果等价。

### 7.4 选择词级时音素级边界不能拖动 ✅ D-28

**问题**：已修复。`hitTestBoundary()` 改为搜索所有层级。详见 commit f63efb3。

### 7.7 Slicer/Phoneme 界面行为不统一

**问题**：两个页面虽然共享 AudioVisualizerContainer，但：
- Slicer 的 playhead、dragController 等连接由页面自行组装，与 PhonemeEditor 行为不一致
- splitter 状态在 `onActivated()` 中恢复时产生双写冲突

**修复方向**：将更多公共逻辑下沉到 AudioVisualizerContainer：
- ✅ playhead 连接已统一到 `setPlayWidget()`
- ✅ splitter 状态延迟恢复已实现
- ⬜ dragController 连接可进一步统一，减少 SlicerPage 组装代码

### 7.5 Slicer 默认比例尺仍过小

**问题**：用户期望视口默认显示 ~2 分钟音频，但实际可能恢复了旧的保存值。

**根因**：`onActivated()` 中 `restoreSplitterState()` 在 widget 高度=0 时调用，QSplitter 将所有子图尺寸设为 0，导致波形图消失、视口显示异常。同时 `rebuildChartLayout()` 中的全局 QSettings 与 per-page AppSettings 分裂器状态冲突。

**修复**：✅ `restoreSplitterState()` 延迟到 widget 首次 layout 后执行；`rebuildChartLayout()` 删除重复的 QSettings 检查。

### 7.6 PhonemeEditor 波形图缺失

**问题**：PhonemeEditor 中波形图没有正常显示。

**根因**：同 7.5 — 分裂器状态恢复时高度=0 导致。同时 `onActivated` → `setChartOrder()` → `rebuildChartLayout()` → `applyDefaultHeightRatios()`（QSettings 路径）覆盖了 AppSettings 恢复的状态。

**修复**：✅ 同上。

---

## 关联文档

- [human-decisions.md](human-decisions.md) — 人工决策记录（最高优先级）
- [unified-app-design.md](unified-app-design.md) — 应用设计方案
- [framework-architecture.md](framework-architecture.md) — 项目架构
- [conventions-and-standards.md](conventions-and-standards.md) — 编码规范
