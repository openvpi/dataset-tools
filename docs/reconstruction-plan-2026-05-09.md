# 重构方案：Phoneme/PitchLabel 控件刷新与交互修复

> 版本：1.1 | 日期：2026-05-09 | 基于用户8+5项需求整理

---

## 一、需求总览与完成状态

| # | 需求 | 状态 | 对应提交 |
|---|------|------|----------|
| 1 | phoneme 缩放时图无法缩放、刻度线减少 | ✅ 已修复 | T-18 (f8ce260) |
| 2 | phoneme 贯穿线在波形图上永久显示异常线 | ✅ 已修复 | 前期提交（drawBoundaryOverlay 仅绘制活跃层） |
| 3 | phoneme 贯穿线拖动崩溃闪退 | ✅ 已修复 | T-23 (4febf6b) + BoundaryDragController 空指针防护 |
| 4 | pitchlabel 音高线显示不正常 | ✅ 已修复 | T-20 (b2dc36d) + PianoRollView 整体重构 |
| 5 | pitchlabel 最大缩放限制不超音频长度 | ✅ 已修复 | T-19 (04f307a) |
| 6 | 控件未实时刷新的问题 | ✅ 已修复 | T-23 + T-24 (50a655d) |
| 7 | **本文档：整理需求，生成重构方案** | 📋 当前 | — |
| 8 | 逐个提交 | ✅ 已完成 | 每任务单独 commit，未推送 |

---

## 二、架构审核：按设计原则逐项核对

### 2.1 P-01（职责单一）：刷新通知由容器统一分发

**现状**：`AudioVisualizerContainer::invalidateBoundaryModel()` 统一刷新 overlay + tierLabel + 所有 chart。符合 P-01。

**审核结论**：✅ 通过。容器级刷新模式正确。

### 2.2 P-02（被动接口 + 容器通知）：IBoundaryModel 不引入 QObject

**现状**：`IBoundaryModel` 是纯虚接口，不含 QObject。容器通过 `invalidateBoundaryModel()` 统一通知。符合 P-02。

**审核结论**：✅ 通过。

### 2.3 P-12（相似模块统一设计）：Waveform/Spectrogram/Power 拖拽逻辑

**现状**（TD-05）：WaveformWidget、SpectrogramWidget、PowerWidget 各自实现 ~150 行拖拽逻辑，代码高度相似但未抽取公共基类。

**问题**：
- WaveformWidget 继承 AudioChartWidget（有公共拖拽逻辑）
- SpectrogramWidget 和 PowerWidget 直接继承 QWidget，各自重新实现拖拽

**影响**：修复拖拽 bug 需同步修改 3 处，与 P-12 冲突。

**建议**：将 SpectrogramWidget 和 PowerWidget 也改为继承 AudioChartWidget，或抽取独立的 DragHandler 组合类（遵循 P-14 组合优于继承）。

**审核结论**：⚠ 需重构，但优先级低（当前无活跃 bug）。

### 2.4 P-14（组合优于继承）：BoundaryDragController 组合模式

**现状**：`BoundaryDragController` 作为独立类被各 widget 持有，通过 QMetaMethod 注入。符合 P-14。

**审核结论**：✅ 通过。

### 2.5 D-05（分割线贯穿规则）：活跃层贯穿，非活跃层仅在标签区

**现状**：
- `BoundaryOverlayWidget::paintEvent`：非活跃层 `lineBottom` 限制在标签区高度内（line 134）
- `AudioChartWidget::drawBoundaryOverlay`：仅绘制活跃层（line 240-244）
- `SpectrogramWidget::drawBoundaryOverlay`：仅绘制活跃层（line 330-333）
- `PowerWidget::drawBoundaryOverlay`：仅绘制活跃层（line 197-199）

**审核结论**：✅ 通过，符合 D-05。

### 2.6 D-24（分辨率驱动视口）：ViewportController resolution 系统

**现状**：ViewportController 基于 resolution 查表驱动缩放（分辨率表：10→15→...→400）。刻度线采用查表法。

**问题**（历史）：PianoRollView 不适用此系统——resolution 步进太大导致音高线过短挤在一起。PianoRollView 已独立管理 hScale（像素/秒平滑缩放）。

**审核结论**：✅ 通过。PianoRollView 通过 `syncToViewport()` 与 ViewportController 双向同步，保持刻度线一致。

### 2.7 D-03（Ctrl+滚轮缩放、Shift+滚轮振幅）

**现状**：
- AudioVisualizerContainer::wheelEvent：Ctrl = zoomIn/zoomOut
- AudioChartWidget::wheelEvent：Ctrl = zoomAt, Shift = verticalZoom
- PianoRollView：通过 InputHandler 独立管理缩放

**审核结论**：✅ 通过。

### 2.8 新增审计：控件实时刷新（任务6）

**审计范围**：全部 UI 组件 31 个 setter 方法。

**审计结果**：

| 组件 | 方法 | update() | 状态 |
|------|------|----------|------|
| AudioChartWidget | setViewport | ✅ | — |
| AudioChartWidget | setBoundaryModel | ❌→✅ | T-24 修复 |
| AudioChartWidget | updateBoundaryOverlay | ✅ | — |
| BoundaryOverlayWidget | setDocument | ✅ | — |
| BoundaryOverlayWidget | setBoundaryModel | ✅ | — |
| BoundaryOverlayWidget | setTierLabelGeometry | ✅ | — |
| BoundaryOverlayWidget | setViewport | ✅ | — |
| BoundaryOverlayWidget | setHoveredBoundary | ✅ | — |
| BoundaryOverlayWidget | setDraggedBoundary | ✅ | — |
| BoundaryOverlayWidget | setPlayhead | ✅ | — |
| WaveformWidget | setAudioData | ✅ | — |
| WaveformWidget | setPlayhead | ✅ | — |
| WaveformWidget | setBoundaryOverlayEnabled | ✅ | — |
| WaveformWidget | onVerticalZoom | ✅ | — |
| SpectrogramWidget | setAudioData | ✅ | — |
| SpectrogramWidget | setViewport | ✅ | — |
| SpectrogramWidget | setBoundaryModel | ❌→✅ | T-24 修复 |
| SpectrogramWidget | setColorPalette | ✅ | — |
| SpectrogramWidget | updateBoundaryOverlay | ✅ | — |
| PowerWidget | setAudioData | ✅ | — |
| PowerWidget | setViewport | ✅ | — |
| PowerWidget | setDocument | ❌→✅ | T-24 修复 |
| PowerWidget | updateBoundaryOverlay | ✅ | — |
| IntervalTierView | setViewport | ✅ | — |
| IntervalTierView | setActive | ✅ | — |
| TierLabelArea | setBoundaryModel | ✅ | — |
| TierLabelArea | setViewportController | ✅ | — |
| TierLabelArea | setActiveTierIndex | ✅ | — |
| TierLabelArea | onModelDataChanged | ✅ | — |
| PianoRollView | setDSFile | ✅ | — |
| PianoRollView | setAudioDuration | ✅ | — |
| PianoRollView | setPlayheadTime | ✅ | — |
| PianoRollView | setPlayheadState | ✅ | — |
| PianoRollView | setABComparisonActive | ✅ | — |
| PianoRollView | setToolMode | ✅ | — |
| PianoRollView | selectNotes | ✅ | — |
| PianoRollView | doPitchMove | ✅ | — |
| PianoRollView | loadConfig | ✅ | — |
| PianoRollView | zoomIn/zoomOut/resetZoom | ✅ | — |

**发现的3个缺口**（已在 T-24 修复）：
1. `AudioChartWidget::setBoundaryModel` — 影响 WaveformWidget 设置边界模型后不刷新
2. `SpectrogramWidget::setBoundaryModel`（内联方法）— 频谱图边界不刷新
3. `PowerWidget::setDocument`（内联方法）— 功率图文档切换不刷新

---

## 三、技术要点总结

### 3.1 phoneme 编辑器缩放修复（任务1）

**根因**：`updateViewRangeFromResolution()` 使用 startSec 作为锚点，当 visibleDuration > totalDuration 时强制重置为 [0, totalDuration]，覆盖了 zoomIn/zoomOut 的中心保持计算。

**修复**：新增 `adjustViewRangeToResolution()` — 保持视口时间中心不变，仅重新计算 viewRange 以匹配新 resolution。

### 3.2 贯穿线修复（任务2）

**根因**：`AudioChartWidget::drawBoundaryOverlay()` 对所有层级绘制全高边界线，违背 D-05。

**修复**：仅绘制 activeTier 边界线。非活跃层边界线由 `BoundaryOverlayWidget` 限高绘制。

### 3.3 拖动崩溃修复（任务3）

**根因**：eventFilter 向 `startDrag` 传递 `nullptr` 模型，`BoundaryDragController::startDrag` 第45行解引用空指针。

**修复**：
- 传递正确的 `m_boundaryModel`
- `BoundaryDragController::startDrag` 增加 `if (!model) return;` 空指针防护

### 3.4 音高线修复（任务4）

**根因**：ViewportController 基于分辨率的步进式缩放对 pitch 编辑不适用（步进太大），且 hScale 从 sampleRate/resolution 反推导致显示异常。

**修复**：PianoRollView 独立管理 hScale，使用平滑因子缩放，从 view duration 推导 hScale。添加 `m_syncingViewport` 递归防护和 `syncToViewport()` 双向同步。

### 3.5 缩放限制（任务5）

**实现**：`minScale = qMax(20.0, drawW / m_audioDuration)` — 确保视口不超过音频总时长。

---

## 四、待改进项（低优先级）

### 4.1 SpectrogramWidget/PowerWidget 与 AudioChartWidget 统一（P-12）

两个 widget 各自重新实现拖拽、hitTest、边界绘制逻辑，与 WaveformWidget（继承 AudioChartWidget）代码重复约 60%。

**建议方向**：
- 方案A：SpectrogramWidget/PowerWidget 改为继承 AudioChartWidget
- 方案B：抽取独立 DragHandler + BoundaryRenderer 组合类（遵循 P-14）

### 4.2 PianoRollView 与 ViewportController 的双向同步复杂度

当前 PianoRollView 通过 `m_syncingViewport` 标志防止递归，依赖 `syncToViewport()` 推送状态给 ViewportController。若未来增加更多 pitch 相关视图，此模式需进一步抽象。

### 4.3 MiniMapScrollBar 缩略图伸缩

原始需求提到"缩略图也无法伸长（覆盖更长时间段）"。当前通过 resolution 驱动，缩略图范围与 resolution/viewRange 联动。在大缩放时范围受限是 resolution 步进导致，非独立 bug。

---

## 五、变更文件清单

| 提交 | 文件 | 变更说明 |
|------|------|----------|
| T-18 | AudioVisualizerContainer.h/cpp | adjustViewRangeToResolution 中心保持缩放 |
| T-19 | PianoRollView.cpp | minScale = drawW/audioDuration 缩放上限 |
| T-20 | PianoRollView.cpp/h | hScale 平滑缩放 + syncToViewport 双向同步 |
| T-23 | AudioVisualizerContainer.cpp | 吸附点击到最近边界启动拖拽 |
| T-23 | BoundaryOverlayWidget.cpp | z-order 修复（活跃层最后绘制） |
| T-23 | PitchLabelerPage.cpp | 推理运行时禁用提取按钮 |
| T-24 | AudioChartWidget.cpp | setBoundaryModel 补 update() |
| T-24 | SpectrogramWidget.h/cpp | setBoundaryModel 移出内联，补 update() |
| T-24 | PowerWidget.h/cpp | setDocument 移出内联，补 update() |

---

## 六、新增任务（2026-05-09 第二轮）

| # | 需求 | 状态 | 对应提交 |
|---|------|------|----------|
| 9 | 一切失败的任务都要在log留下详细报错 | ✅ 已完成 | Task-1 |
| 10 | phoneme有时长的条目在pitch页面提取midi时显示转录失败（align模式输入时长） | ✅ 已完成 | Task-2 |
| 11 | 窗口放大缩小、工作区调整比例持久化到用户目录（带版本，向后兼容） | ✅ 已完成 | Task-3 |
| 12 | minlabel的batchasr加个选项，自动g2p到结果 | ✅ 已完成 | Task-4 |
| 13 | dstext音高曲线无法正常显示（timestep float/int兼容） | ✅ 已完成 | Task-5 |
| 14 | 整理以上问题，更新到文档 | 📋 当前 | Task-6 |

### 6.1 失败任务详细日志（任务9）

**问题**：PipelineRunner、PitchLabelerPage 批量提取、MinLabelPage 批量 ASR 失败时仅设置状态/弹窗，不记录日志。

**修复**：
- [PipelineRunner.cpp](file:///d:/projects/dataset-tools/src/framework/core/src/PipelineRunner.cpp): import/buildInput/process 失败点均增加 DSFW_LOG_ERROR
- [PitchLabelerPage.cpp](file:///d:/projects/dataset-tools/src/apps/shared/data-sources/PitchLabelerPage.cpp): 批量 pitch/MIDI 提取失败增加 DSFW_LOG_ERROR + 对话框显示实际错误
- [MinLabelPage.cpp](file:///d:/projects/dataset-tools/src/apps/shared/data-sources/MinLabelPage.cpp): 批量 ASR 失败将 msg 写入日志和对话框

### 6.2 MIDI提取align模式修复（任务10）

**问题**：phoenme有时长的条目在pitch页面提取MIDI时显示"转录失败"。

**根因**：批量 MIDI 提取仅使用 `game->getNotes()`（非对齐模式），未利用已有的 phoneme 时长数据。

**修复**：
- [PitchLabelerPage.cpp](file:///d:/projects/dataset-tools/src/apps/shared/data-sources/PitchLabelerPage.cpp): 批量提取循环中优先检测 phoneme 层数据，存在时构建 AlignInput 并使用 `game->align()` 对齐模式
- 对齐失败时回退到 `game->getNotes()` 非对齐模式
- 错误消息显示实际错误原因

### 6.3 窗口/工作区持久化 + 版本化存储（任务11）

**问题**：窗口大小、分割器状态等未统一持久化，不同版本设置文件可能不兼容。

**修复**：
- [CommonKeys.h](file:///d:/projects/dataset-tools/src/framework/core/include/dsfw/CommonKeys.h): 新增 SettingsVersion（版本号）、AudioSplitter/PitchSplitter/MinSplitter/HomeSplitter 分割器状态 key
- [AppSettings.h/cpp](file:///d:/projects/dataset-tools/src/framework/core): 新增 `migrateIfNeeded()` 方法，加载后自动执行版本迁移（v0→v1 标记当前版本号）
- [AppShell.cpp](file:///d:/projects/dataset-tools/src/framework/ui-core/src/AppShell.cpp): WindowGeometry/WindowState 已在 closeEvent/showEvent 持久化（已有代码）

**迁移兼容**：version 0（无版本号的旧设置）→ version 1（标记版本）。未来可通过 `migrateIfNeeded()` 处理跨版本迁移。

### 6.4 MinLabel batch ASR 自动 G2P（任务12）

**问题**：批量 ASR 后需手动逐条执行 G2P 转换，效率低。

**修复**：
- [MinLabelPage.h/cpp](file:///d:/projects/dataset-tools/src/apps/shared/data-sources): 新增 `batchG2P()` 方法，使用 cpp-pinyin 库进行汉字→拼音转换
- 批量 ASR 对话框新增「自动 G2P (拼音) 到结果」复选框
- 后台线程中 ASR 完成后自动调用 `batchG2P()` 生成 grapheme 层并保存

### 6.5 dstext音高曲线无法显示（任务13）

**问题**：特定 dstext 文件（如 `01_huanwulinglong_001.dstext`）音高曲线不显示。

**根因**：`DsTextDocument::load()` 中 `timestep` 字段使用 `nlohmann::json.value<int64_t>()` 读取，但旧版本文件可能以 float（秒）格式存储 timestep（如 `"timestep": 0.005`）。JSON 解析 float→int64_t 失败返回默认值 0，PianoRollRenderer 检查 `timestep <= 0` 时跳过绘制。

**修复**：
- [DsTextDocument.cpp](file:///d:/projects/dataset-tools/src/domain/src/DsTextDocument.cpp): 增加双重格式兼容：
  - float 格式且值 < 1.0：视为旧版秒格式，自动转换为微秒（`ts * 1000000`）
  - float 格式且值 ≥ 1.0 或 integer 格式：直接作为微秒读取

### 6.6 变更文件清单（第二轮）

| 提交 | 文件 | 变更说明 |
|------|------|----------|
| Task-1 | PipelineRunner.cpp | import/buildInput/process 失败增加 DSFW_LOG_ERROR |
| Task-1 | PitchLabelerPage.cpp | 批量提取失败日志 + 对话框显示实际错误 |
| Task-1 | MinLabelPage.cpp | 批量 ASR 失败日志含错误消息 |
| Task-2 | PitchLabelerPage.cpp | 批量 MIDI 提取使用 align 模式（有 phoneme 时） |
| Task-3 | CommonKeys.h | 新增版本号、分割器状态 SettingsKey |
| Task-3 | AppSettings.h/cpp | migrateIfNeeded() 版本迁移机制 |
| Task-4 | MinLabelPage.h/cpp | batchG2P() + 批量 ASR 自动 G2P 复选框 |
| Task-5 | DsTextDocument.cpp | timestep 读取兼容 float(秒)/int64(微秒) 双格式 |