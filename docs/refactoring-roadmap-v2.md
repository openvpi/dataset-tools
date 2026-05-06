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

### 1.1 PhonemeLabeler 标签层位置调整

**问题**：PhonemeEditor 中 TierEditWidget（interval 文本编辑控件）作为 chart 添加到 chartSplitter 中（stretchFactor=0，heightWeight=0），实际不可见。标签层（TierEditWidget）应该在 TimeRuler 刻度线下方、Waveform 波形图上方。

**设计**：当前容器布局为 MiniMap → TimeRuler → TierLabelArea → chartSplitter(charts)。PhonemeTextGridTierLabel（tier 选择 radio buttons）已正确位于 TierLabelArea 位置。TierEditWidget 需要从 chartSplitter 中移出并插入到 TimeRuler 下方、chartSplitter 上方。

**修复**：

- [ ] 研究 TierEditWidget 作为独立行插入到 TimeRuler 和 chartSplitter 之间的布局方案（不在 chartSplitter 内）
- [ ] 或：将 TierEditWidget 的 stretchFactor/heightWeight 设为非零值使其在 chartSplitter 内正常显示
- [ ] 确保 boundary overlay 的 tierLabelGeometry 偏移正确反映新布局
- [ ] 验证：DsLabeler 的 PhonemeLabeler 页面标签编辑区在刻度线下方、波形图上方可见

### 1.2 Slicer 波形图与刻度线同步缩放

**问题**：SlicerPage 的 WaveformWidget/SpectrogramWidget 在缩放时没有与刻度线同步。所有应用应使用统一的 AudioVisualizerContainer 显示框架，由 ViewportController 驱动所有子图缩放。

**修复**：

- [ ] 审查 SlicerPage 中 viewport → widget 的连接：目前 AudioVisualizerContainer::addChart() 通过 QMetaMethod::invoke 连接 setViewport，需确认 SlicerPage 的 waveform/spectrogram 收到 viewportChanged 信号
- [ ] 检查 SlicerPage 是否有多余的 viewport 连接/覆盖导致缩放失效
- [ ] 验证：Ctrl+滚轮缩放时 WaveformWidget、SpectrogramWidget、TimeRuler 同步缩放
- [ ] 验证：DsLabeler 的 DsSlicerPage 同样有效

### 1.3 PhonemeEditor 波形图显示异常

**问题**：PhonemeLabeler 页面中波形图没有正常显示，而 SlicerPage 使用同一套 AudioVisualizerContainer 框架却正常显示。

**修复**：

- [ ] 对比 PhonemeEditor::loadAudio() 与 SlicerPage::loadAudioFile() 的波形数据设置逻辑，找出差异
- [ ] 检查 PhonemeEditor 中 WaveformWidget 是否通过 AudioVisualizerContainer::addChart() 正确注册且收到 setViewport 信号
- [ ] 检查 WaveformWidget 的 setAudioData() 调用时机与 viewport 初始化顺序
- [ ] 验证：PhonemeLabeler 页面加载切片后波形图正常渲染

### 1.4 PhonemeEditor 视图菜单状态与实际图状态联动

**问题**：PhonemeLabeler 页面"视图"菜单中 "Show Power" / "Show Spectrogram" 等选项的 checked 状态没有和实际图表的可见性联动。

**设计**：目前 PhonemeEditor 的 `setPowerVisible()` / `setSpectrogramVisible()` 方法只调用 `widget->setVisible()` 和 `action->setChecked()`，但没有反向同步机制。当外部通过其他途径（如 splitter 拖拽隐藏、chartOrder 变更）改变图表可见性时，菜单项 checked 状态不会更新。

**修复**：

- [ ] PhonemeEditor 新增信号 `powerVisibilityChanged(bool)` / `spectrogramVisibilityChanged(bool)`
- [ ] 在 `setPowerVisible()` / `setSpectrogramVisible()` 中 emit 对应信号
- [ ] PhonemeLabelerPage::createMenuBar() 连接信号以更新 action checked 状态
- [ ] 确保 chartOrder 变更时（图表被移除/重新排序）也触发可见性信号更新
- [ ] 验证：切换图表显示/隐藏时菜单项 checked 状态实时同步

### 1.5 HFA 对齐意外导出自动添加的 SP

**问题**：HFA::recognize() 在 `words.add_SP(wav_length, "SP")` 中自动在头尾添加 SP（静音）音素。PhonemeLabelerPage::runFaForSlice() / onBatchFA() 将这些 SP 导出到音素层，导致数据集中出现意外的头尾 SP 标注。

**设计**：需要区分"算法内部需要 SP 用于对齐精度"和"用户期望的输出数据"。HFA 内部 add_SP 是算法行为，但输出到数据集时应该过滤掉这些自动添加的 SP。

**修复**：

- [ ] 审查 HFA 返回的 WordList 中哪些 phoneme 是 `add_SP` 自动添加的（头尾 SP）
- [ ] 在 `PhonemeLabelerPage::runFaForSlice()` 和 `onBatchFA()` 的结果处理中，过滤掉头尾的 auto-SP 音素
- [ ] 或：HFA 层提供选项 `autoAddSP` 参数，由调用方决定是否添加
- [ ] 验证：FA 完成后导出的 phoneme 层不含意外头尾 SP

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

### 2.2 TimeRulerWidget 刻度计算查表法

```cpp
struct TimescaleLevel { double majorSec; double minorSec; };
static const TimescaleLevel kLevels[] = {
    {0.001, 0.0005}, {0.005, 0.001}, {0.01, 0.005},
    {0.05, 0.01}, {0.1, 0.05}, {0.5, 0.1},
    {1.0, 0.5}, {5.0, 1.0}, {15.0, 5.0},
    {30.0, 10.0}, {60.0, 15.0}, {300.0, 60.0},
    {900.0, 300.0}, {3600.0, 900.0},
};
constexpr double kMinMinorStepPx = 60.0;
```

- [ ] 用查表法替代当前 smoothStep 渐隐逻辑
- [ ] 主刻度：长线 + 时间标签（自动选择 ms/s/min:sec/h:min:sec）
- [ ] 次刻度：短线，无标签

### 2.3 wheelEvent 统一

- [ ] 所有子图的 Ctrl+滚轮 → `m_viewport->zoomIn/zoomOut(centerSec)`
- [ ] MiniMapScrollBar wheelEvent：无 modifier = 横向滚动，Ctrl = 缩放

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

### 3.1 最右侧边界线无法拖动

**问题**：TextGridDocument::clampBoundaryTime 中最后一条边界的 nextBoundary 取了自身 interval 的 max_time，等于边界本身，clamp 区间为 0。

**修复**：

- [ ] `TextGridDocument::clampBoundaryTime()`：`boundaryIndex == count - 1` 时 nextBoundary 使用 `tier->GetMaxTime()`（文档总时长）
- [ ] 排查 `SliceBoundaryModel::clampBoundaryTime()` 是否有类似问题
- [ ] 验证：最后一条边界可自由拖动到音频结尾

### 3.2 Phoneme 标签区域无数据时显示提示

**问题**：`PhonemeTextGridTierLabel` 在 tierCount == 0 时高度为 0，标签区域消失。

**修复**：

- [ ] `PhonemeTextGridTierLabel::rebuildRadioButtons()`：tierCount == 0 时保持最小高度 `kTierRowHeight`
- [ ] `PhonemeTextGridTierLabel::paintEvent()`：tierCount == 0 时绘制居中灰色文字 "No label data"
- [ ] 验证：切换到未标注切片时标签区域仍可见

### 3.3 最近工程列表标灰无效路径

**修复**：

- [ ] `WelcomePage::refreshRecentList()`：对每个路径检查 `QFileInfo::exists()`
- [ ] 不存在的路径：item 文字灰色 + 删除线
- [ ] 验证：删除 .dsproj 文件后打开 DsLabeler，对应条目显示为灰色删除线

### 3.4 删除废弃的 SliceNumberLayer

- [ ] 删除 `src/apps/shared/data-sources/SliceNumberLayer.h/cpp`
- [ ] 从 CMakeLists.txt 移除引用
- [ ] 确认无其他文件 include 或引用该类

### 3.5 IEditorDataSource 层统一音频文件有效性管理

**问题**：音频文件存在性检查散落在各页面代码中，批量操作全部绕过检查。

**修复**：

- [ ] `IEditorDataSource` 新增 `audioExists()`、`validatedAudioPath()`、`missingAudioSlices()`、`audioAvailabilityChanged` 信号
- [ ] `SliceListPanel::refresh()` 中标记缺失音频条目（图标 + 颜色 + tooltip）
- [ ] 所有推理调用点 `audioPath()` → `validatedAudioPath()`
- [ ] 批量操作结束时报告跳过数

### 3.6 dsproj audioSource 路径健壮性

- [ ] `ProjectDataSource::sliceAudioPath()`：路径不存在时在 `wavs/` 目录下查找启发式 fallback
- [ ] fallback 时增加 `qWarning()` 日志
- [ ] 考虑将 audioSource 转为相对路径存储

### 3.7 dsproj 数据模型其他问题

- [ ] ExportConfig 格式重复 → 加载后去重
- [ ] defaults round-trip 残留 → 保存时从 m_extraFields 移除
- [ ] slice.in/out 语义文档化
- [ ] 版本校验

---

## 4. 编辑器页面去重

### 4.1 激活 EditorPageBase，合并确定性重复代码

仅提取**字面相同**的部分，不做过度抽象。

```cpp
class EditorPageBase : public QWidget,
                       public labeler::IPageActions,
                       public labeler::IPageLifecycle {
    Q_OBJECT
protected:
    // 公共成员
    SliceListPanel *m_sliceList = nullptr;
    QSplitter *m_splitter = nullptr;
    IEditorDataSource *m_source = nullptr;
    ISettingsBackend *m_settingsBackend = nullptr;
    QString m_currentSliceId;
    dstools::AppSettings m_settings;
    dstools::widgets::ShortcutManager *m_shortcutManager = nullptr;
    QAction *m_prevAction = nullptr;
    QAction *m_nextAction = nullptr;

    // 构造辅助
    void setupBaseLayout(QWidget *editorWidget);
    void setupNavigationActions();

    // 生命周期公共部分
    void onActivated() override;
    bool onDeactivating() override;
    void onDeactivated() override;
    void onShutdown() override;

    // 公共实现
    void setDataSource(IEditorDataSource *source, ISettingsBackend *settingsBackend);
    QString windowTitle() const override;

    // 子类必须实现
    virtual QString windowTitlePrefix() const = 0;
    virtual bool isDirty() const = 0;
    virtual bool saveCurrentSlice() = 0;
    virtual void onSliceSelectedImpl(const QString &sliceId) = 0;

    // 可选钩子
    virtual void restoreExtraSplitters() {}
    virtual void saveExtraSplitters() {}
    virtual void onAutoInfer() {}
    virtual void onDeactivatedImpl() {}

    // 公共辅助
    bool maybeSaveDialog();

signals:
    void sliceChanged(const QString &sliceId);
};
```

- [ ] `EditorPageBase` 实现上述公共成员和方法
- [ ] MinLabelPage、PhonemeLabelerPage、PitchLabelerPage 改为继承 `EditorPageBase`
- [ ] 删除各页面中字面重复的代码
- [ ] 验证：三页面行为不变

### 4.2 删除废弃的 EditorPageBase 旧实现

- [ ] 清除旧的 `createMenuBar()` 英文实现
- [ ] 清除旧的 `maybeSave()` 简陋版本
- [ ] 确认无其他代码依赖旧行为

---

## 5. 推理引擎优化

### 5.1 AudioVisualizerContainer 文档/模型刷新框架

**问题**：`IBoundaryModel` 是纯数据接口，无变更通知能力。PhonemeEditor FA 结果不刷新、切换切片丢标签。

**修复**：

- [ ] `AudioVisualizerContainer` 新增 `invalidateBoundaryModel()` 方法（已实现）
- [ ] `invalidateBoundaryModel()` 内部：`m_boundaryOverlay->update()` + `m_tierLabelArea->onModelDataChanged()` + 遍历 chart widgets 调用 `update()`
- [ ] 新增 `boundaryModelInvalidated()` 信号供外部组件连接
- [ ] `TierLabelArea` 新增虚方法 `onModelDataChanged()`
- [ ] `PhonemeTextGridTierLabel` override `onModelDataChanged()` → `rebuildRadioButtons()` + 调整高度
- [ ] PhonemeEditor 中 `documentChanged` 连接 → container `invalidateBoundaryModel()`
- [ ] Slicer 页面切片点变化时调用 `m_container->invalidateBoundaryModel()`

### 5.2 FA 自动对齐非阻塞化 + 状态指示

- [ ] `ensureHfaEngine()` 拆分为同步检查 + 异步加载
- [ ] 模型加载期间 UI 不阻塞
- [ ] `PhonemeTextGridTierLabel` 新增 `setAlignmentRunning(bool)` — FA 运行中显示 "对齐中..."
- [ ] 切换切片/页面时取消当前 FA 或丢弃结果
- [ ] 验证：模型加载不阻塞 UI，FA 运行状态可见

### 5.3 所有自动推理/补全操作非阻塞化

- [ ] `EditorPageBase` 新增 `loadEngineAsync()` 通用方法
- [ ] PhonemeLabelerPage `ensureHfaEngineAsync()` ✓（已实现，需验证）
- [ ] PitchLabelerPage `ensureRmvpeEngine()` / `ensureGameEngine()` → 异步版本
- [ ] MinLabelPage `ensureAsrEngine()` → 异步版本
- [ ] ExportPage `onExport()` 中的 autoComplete 循环移入 `QtConcurrent::run`
- [ ] 删除 `QApplication::processEvents()` 调用

---

## 6. 路径与 UI 完善

### 6.1 统一路径管理模块

- [ ] 创建 `audio-util/include/audio-util/PathCompat.h`
- [ ] `resample_to_vio`、`FlacDecoder`、`write_vio_to_wav` 中的 `SndfileHandle(path.string())` → `openSndfile(path)`
- [ ] `GameModel.cpp` 中的 `ifstream((path).string())` → `openIfstream(path)`
- [ ] `dstools::toFsPath` 改为调用 `dsfw::PathUtils::toStdPath` 的 wrapper
- [ ] 验证：中文/日文/空格路径推理成功

### 6.2 CSV 预览暗/明主题适配

- [ ] `dark.qss` 添加 QTableView 完整样式
- [ ] `light.qss` 添加 QTableView 完整样式
- [ ] 验证：ExportPage CSV 预览在暗色/亮色主题下均清晰可读

### 6.3 快捷键系统

| 页面 | 快捷键 |
|------|--------|
| Slicer | S=自动切片, E=导出, I=导入切点, P=指针, K=切刀 |
| PhonemeLabeler | F=FA, ←/→=切换切片 |
| PitchLabeler | F=提取音高, M=MIDI, ←/→=切换切片 |
| MinLabel | R=ASR, ←/→=切换切片 |

### 6.4 子图顺序配置 UI

Settings 页面增加"图表顺序"可拖拽排序列表。

### 6.5 DsLabeler CSV 预览

ExportPage 内部 TabWidget："导出设置" | "预览数据" → QTableView + QStandardItemModel（只读），缺少前置数据的行标红。

---

## 执行优先级

1. **2.1** — 比例尺连续范围改造 ✅ 已完成
2. **2.4** — Phoneme 标签 binding/snap 重设计 ✅ 已完成
3. **7.x** — 后续 Bug 修复批次（见下方）
4. **1.1-1.5** — 显示与 UX 修复
5. **3.1、3.2、3.4** — 简单边界/标签修复
6. **5.1** — 刷新框架
7. **2.2、2.3** — 刻度查表法 + wheelEvent 统一
8. **3.5、3.6、3.7** — 数据健壮性
9. **4.1、4.2** — 编辑器去重
10. **5.2、5.3** — 推理异步化
11. **6.1** — 路径管理
12. **6.2-6.5、3.3** — 低优先级完善

---

## 7. 后续 Bug 修复批次

### 7.1 Phoneme 刻度显示异常

**问题**：PhonemeEditor 加载音频后，TimeRulerWidget 的刻度间距/标签不正确。

**根因**：`TimeRulerWidget` 初始化时 `m_resolution=40, m_sampleRate=44100`（ViewportState 默认值）。音频加载后 `setAudioParams()` + `fitToWindow()` 更新 resolution=800，但 `syncStateFields()` 必须在 `setAudioParams()` 中被调用以确保 ViewportState.sampleRate 正确广播。检查 `ViewportController::setAudioParams()` 是否在更新 `m_sampleRate` 后调用了 `syncStateFields()`。

**修复**：
- [ ] 确认 `ViewportController::setAudioParams()` 中 `syncStateFields()` 在 `clampAndEmit()` 之前被调用（已有，需验证）
- [ ] 确认 `fitToWindow()` 在 widget 尺寸 > 0 时被调用（可能延迟到 resizeEvent）
- [ ] 验证：PhonemeEditor 加载音频后 TimeRuler 正确显示秒级刻度

### 7.2 Slicer 右键播放无红色游标

**问题**：SlicerPage 右键播放音频片段时没有显示红色竖线播放游标。

**根因**：SlicerPage 独立组装，缺少 playhead 信号连接到 waveform 和自动清除超时。PhonemeEditor 有此逻辑但未复用。

**修复**：✅ `AudioVisualizerContainer::setPlayWidget()` 统一处理：自动连接 playhead 到所有有 `setPlayhead` 方法的 chart widget（通过 QMetaMethod），包含 200ms 自动清除超时。SlicerPage 和 PhonemeEditor 的 per-page 连接已删除。

### 7.3 Phoneme 层级边界约束失效（跨层 clamp）

**问题**：低层级（如 phoneme）的边界拖动时可以越过高层级（如 grapheme）的边界，违反 D-17 层级包含规则。

**根因**：`TextGridDocument::clampBoundaryTime()` 只检查**同层**相邻边界。`BoundaryDragController::updateDrag()` 也只调用同层 clamp。没有任何地方实施跨层约束（高层边界作为低层边界的外包围）。

**设计（D-17 天然支持）**：

对于 tier hierarchy（tier 0 = 最高层如 grapheme，tier N = 最低层如 phoneme）：
- 低层级的边界**不得超出**其所在的高层级区间的左右边界
- 具体：phoneme tier 的某个边界 B，找到 grapheme tier 中包含 B 当前位置的区间 [Gstart, Gend]，B 的 clamp 范围进一步收紧为 max(sameLayerLeft, Gstart) ~ min(sameLayerRight, Gend)

**修复**：
- [ ] `IBoundaryModel` 新增 `clampToParentTier(tierIndex, boundaryIndex, proposedTime)` 虚方法（默认返回 proposedTime）
- [ ] `TextGridDocument` 实现：对 tierIndex > 0 的层，找上层区间包围，收紧 clamp
- [ ] `BoundaryDragController::updateDrag()` 在 `clampBoundaryTime()` 后再调用 `clampToParentTier()`
- [ ] 验证：phoneme 层边界拖动到达 grapheme 边界时被阻挡

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
