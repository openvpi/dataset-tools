# 重构方案

> 2026-05-08 — 合并自 `refactoring-plan-v3.md` + `refactoring-roadmap-v2.md`
>
> 设计准则与决策详见 `human-decisions.md`

---

## 关键架构决策 (ADR)

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
| 85 | applyFaResult 不覆盖 grapheme 层 | 有效 |
| 89 | 激活层贯穿线显示修复（D-40） | **已完成** |
| 90 | 贯穿线拖动修复——chart 绘制边界线（D-41） | **已完成** |
| 91 | FA 原生输出层级从属关系（D-42） | **已完成** |
| 92 | PitchLabeler 工具栏按钮 + 音频播放（D-43） | **已完成** |

---

## 剩余任务

| # | 需求 | 类型 | 优先级 | 设计决策 |
|---|------|------|--------|---------|
| 1 | 页面标题过长，无边框不能显示完整标题 | Bug 修复 | P1 | — |
| 2 | phoneme 激活层贯穿线显示修复——移除 tierline 后贯穿线未覆盖子图区域 | Bug 修复 | P0 | D-40 |
| 3 | phoneme 贯穿线拖动修复——chart 内拖动变成拖动全图而非边界线 | Bug 修复 | P0 | D-41 |
| 4 | FA 原生输出层级从属关系——grapheme 层未正确获取对齐结果，边界未绑定其他层级 | 功能修改 | P0 | D-42 |
| 5 | PitchLabeler 工具栏按钮 + 音频播放修复——缺少 GAME 和抽取 F0 按钮，音频播放异常 | 功能修改 | P1 | D-43 |

---

## 详细分析

### 1. 页面标题过长被截断

**现状**：`FramelessHelper.cpp:114-116` TitleBar::resizeEvent：
```cpp
m_titleLabel->setGeometry(0, 0, width(), height());
```

标题覆盖整个 title bar 宽度，但 menu bar 在左侧、窗口按钮在右侧，导致标题文字与二者重叠，加上 `Qt::AlignCenter` 对齐，开头几个字被 menu bar 遮挡。

**方案**：动态计算可用宽度。标题 label 的宽度应限制为 `width() - menuBarWidth - buttonsWidth`，水平居中在此有效区域内。

**设计约束**：
- `TitleBar` 的 layout 布局：menuBar (pos 0) + stretch + buttons
- 标题 label 是浮动层 (`WA_TransparentForMouseEvents`, `lower()`)
- 应计算 menuBar 的 `sizeHint().width()` + buttons 总宽度，居中偏移

**影响文件**：`src/framework/ui-core/src/FramelessHelper.cpp`（`TitleBar::resizeEvent`）

---

### 2. phoneme 激活层贯穿线显示修复

**设计决策**：详见 `human-decisions.md` D-40

**问题**：`PhonemeEditor::buildLayout()` 调用 `m_container->removeTierLabelArea()` 将 `PhonemeTextGridTierLabel` 从布局中移除。之后 `boundaryOverlay->setTierLabelGeometry(0, 0)` 将 label 高度和行高均设为 0。`BoundaryOverlayWidget::paintEvent()` 中 `tierLabelH == 0`、`tierRowH == 0` 时：
- Active tier 的 `lineTop = t * 0 = 0`，`lineBottom = fullBottom`——理论应显示贯穿线
- 但 `repositionOverSplitter()` 中 `totalTopOffset = m_tierLabelTotalHeight + m_extraTopOffset` 很可能为 0，导致 overlay 未正确覆盖编辑区顶部位置

**修复方案**：
1. 验证 `removeTierLabelArea()` 后 `updateOverlayTopOffset()` 的 offset 值是否正确
2. `AudioVisualizerContainer::removeTierLabelArea()` 中在移除 label area 后，正确设置 `m_extraTopOffset` 为 `m_tierEditWidget->height()` 或编辑区高度
3. 确保 `BoundaryOverlayWidget::repositionOverSplitter()` 在无 tier label 时能正确覆盖编辑区+子图区域

**影响文件**：
- `src/apps/shared/phoneme-editor/ui/BoundaryOverlayWidget.cpp`
- `src/apps/shared/phoneme-editor/PhonemeEditor.cpp`
- `src/apps/shared/audio-visualizer/AudioVisualizerContainer.cpp`

---

### 3. phoneme 贯穿线拖动修复——chart 内拖动不应滚动全图

**设计决策**：详见 `human-decisions.md` D-41

**问题**：`WaveformWidget`、`PowerWidget`、`SpectrogramWidget` 三个 chart widget 的 `mousePressEvent` 中，`hitTestBoundary()` 检查点击位置是否靠近边界线。如果命中，启动 `BoundaryDragController` 拖动；否则启动视口拖动（滚动全图）。用户点击贯穿线（由透明的 `BoundaryOverlayWidget` 绘制）时，鼠标事件穿透到 chart widget。如果 `hitTestBoundary()` 未找到距离够近的边界，则进入视口拖动模式。

**根因**：
1. `WaveformWidget::drawBoundaryOverlay()` 未在 `paintEvent()` 中被调用——chart 自身不绘制边界线，用户看到的贯穿线完全是透明 overlay 绘制的，但 overlay 不接收鼠标事件
2. 即使 chart 不绘制边界，`hitTestBoundary()` 仍可通过 `m_boundaryModel` 找到边界。但如果 `m_boundaryModel` 未正确设置，则始终返回 -1
3. `kBoundaryHitWidth` 的默认值可能在高 DPI 或特定缩放级别下偏小

**修复方案**：
1. 在三个 chart widget 的 `paintEvent()` 末尾调用 `drawBoundaryOverlay()`，使 chart 自身绘制边界线（视觉反馈+鼠标事件目标）
2. 确保 `m_boundaryModel` 在 chart widget 构造后即通过 `setBoundaryModel()` 设置（不要在 `loadAudio()` 中才设置）
3. 增大 `kBoundaryHitWidth` 或在 `mousePressEvent` 中增加辅助判断：当点击位置距最近边界小于某个较大阈值时优先模拟拖动

**影响文件**：
- `src/apps/shared/phoneme-editor/ui/WaveformWidget.{h,cpp}`
- `src/apps/shared/phoneme-editor/ui/PowerWidget.{h,cpp}`
- `src/apps/shared/phoneme-editor/ui/SpectrogramWidget.{h,cpp}`

---

### 4. FA 原生输出层级从属关系——绑定 grapheme↔phoneme 边界

**设计决策**：详见 `human-decisions.md` D-42

**问题**：
1. **Grapheme 层边界不完整**：`buildFaLayers()` 从 `HFA::WordList` 构建两个层（grapheme 和 phoneme），但每个 word 只产生一个 start 边界，没有 end 边界
2. **缺少 word.end 到 phone.end 的绑定**：`applyFaResult()` 显式跳过 grapheme 层（`if (newLayer.name == "grapheme") continue;`），BindingGroup 只在 `word.start == phone[0].start` 时建立
3. **FA 输出未被应用**：grapheme 层在 `applyFaResult` 中被跳过，FA 产生的对齐后更精确的 grapheme 边界被丢弃

**修复方案**：
1. `buildFaLayers()` 为每个 word 同时输出 start 和 end 边界
2. 为 word.end 与 `word.phones.back().end` 建立 BindingGroup
3. 修改 `applyFaResult()`：新增 `fa_grapheme` 层保存 FA 对齐后的 grapheme 边界（保留原始 minlabel grapheme 作为 FA 输入源）
4. `readFaInput()` 始终从原始 minlabel grapheme 层读取

**新增数据结构**：
```cpp
struct LayerDependency {
    QString parentLayer;   // "grapheme"
    QString childLayer;    // "phoneme"
    int parentBoundaryStartId;
    int parentBoundaryEndId;
    int childBoundaryStartId;
    int childBoundaryEndId;
    std::vector<int> childBoundaryIds;
};
```

**影响文件**：
- `src/apps/shared/data-sources/PhonemeLabelerPage.cpp`（`buildFaLayers`、`applyFaResult`、`readFaInput`）
- `src/apps/shared/phoneme-editor/ui/TextGridDocument.{h,cpp}`（绑定逻辑）
- `src/domain/include/dstools/DsTextTypes.h`（新增 `LayerDependency`）

---

### 5. PitchLabeler 工具栏按钮 + 音频播放修复

**设计决策**：详见 `human-decisions.md` D-43

**问题**：
1. **缺乏工具栏**：`PitchLabelerPage::createMenuBar()` 将"提取音高"和"提取 MIDI"放在 Processing 菜单中，无独立 `QToolBar`
2. **音频播放不稳定**：`onSliceSelectedImpl()` 中 `loadAudio()` 只在 `audioPath.isEmpty()` 为 false 时调用。`validatedAudioPath()` 可能因路径验证失败返回空，导致即使音频存在也无法播放

**修复方案**：
1. 在 `PitchLabelerPage` 顶部添加 `QToolBar`：`[Play/Pause] [Stop] | [Extract F0] [Extract MIDI] | [Save] | [Zoom In] [Zoom Out]`
2. 使用与 `PhonemeLabelerPage` 一致的 toolbar 风格（QToolButton + icon）
3. 复用已有的 `m_extractPitchAction` 和 `m_extractMidiAction`
4. 修复音频播放：`onSliceSelectedImpl()` 无条件加载音频；`validatedAudioPath()` 返回空时回退到 `source()->audioPath()`
5. 添加路径验证失败时的明确日志和 toast 提示

**影响文件**：
- `src/apps/shared/data-sources/PitchLabelerPage.{h,cpp}`

---

## 执行顺序

```
组 A (数据正确性): [4]
组 B (核心编辑交互): [2, 3]
组 C (UI 改进): [1, 5]
```

各组内可并行执行。

### 依赖关系

| # | 前置依赖 |
|---|---------|
| 1 | 无 |
| 2 | 任务 28（tierline 移除已完成） |
| 3 | 无 |
| 4 | 无 |
| 5 | 无 |

---

## 受影响文件清单

| # | 文件 | 变更 |
|---|------|------|
| 1 | `src/framework/ui-core/src/FramelessHelper.cpp` | `TitleBar::resizeEvent` 标题区域计算 |
| 2 | `src/apps/shared/phoneme-editor/ui/BoundaryOverlayWidget.cpp` | `paintEvent` / `repositionOverSplitter` 修复 |
| 2 | `src/apps/shared/audio-visualizer/AudioVisualizerContainer.cpp` | `removeTierLabelArea` 更新 `m_extraTopOffset` |
| 2 | `src/apps/shared/phoneme-editor/PhonemeEditor.cpp` | `buildLayout` overlay 定位修复 |
| 3 | `src/apps/shared/phoneme-editor/ui/WaveformWidget.{h,cpp}` | `paintEvent` 调用 `drawBoundaryOverlay` |
| 3 | `src/apps/shared/phoneme-editor/ui/PowerWidget.{h,cpp}` | 同上 |
| 3 | `src/apps/shared/phoneme-editor/ui/SpectrogramWidget.{h,cpp}` | 同上 |
| 4 | `src/apps/shared/data-sources/PhonemeLabelerPage.cpp` | `buildFaLayers` + `applyFaResult` grapheme 完整边界 |
| 4 | `src/domain/include/dstools/DsTextTypes.h` | 新增 `LayerDependency` 结构 |
| 4 | `src/apps/shared/phoneme-editor/ui/TextGridDocument.{h,cpp}` | 加载 dependencies |
| 5 | `src/apps/shared/data-sources/PitchLabelerPage.{h,cpp}` | 添加工具栏 + 音频加载修复 |
