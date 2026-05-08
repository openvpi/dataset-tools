# 架构债审计报告

**审计日期**：2026-05-08
**验证日期**：2026-05-08（已对照源码逐项验证）
**审计范围**：src/ 全目录
**参考文档**：framework-architecture.md、human-decisions.md (P-01 ~ P-09)

---

## 汇总

| 严重度 | 数量 |
|--------|------|
| 高 | 5 |
| 中 | 6 |
| 低 | 4 |
| **合计** | **15** |

---

## ARCH-01: Apps 层直接依赖 Infer 层 — 违反 apps → libs → infer 分层

| 属性 | 值 |
|------|-----|
| **严重度** | **高** |
| **违反准则** | 分层架构（framework-architecture.md §6） |
| **验证状态** | ✅ 已确认 |
| **影响文件** | PhonemeLabelerPage.cpp:L20, PitchLabelerPage.cpp:L22-23, ExportService.cpp:L9-11, ExportPage.cpp:L14-16, SettingsPage.cpp:L25, ModelProviderInit.cpp:L9-11, PitchExtractionService.cpp:L3-4 |

**描述**：项目设计要求 `apps → libs → infer` 三层依赖，apps 层不应直接 include infer 层头文件。但实际代码中，7 个 apps/shared 文件直接 `#include <hubert-infer/Hfa.h>`、`<rmvpe-infer/Rmvpe.h>`、`<game-infer/Game.h>`。

`LayerAudit.cmake` 已定义了检查逻辑（L7-12），将 `hubert-infer/`、`rmvpe-infer/`、`game-infer/`、`audio-util/` 列为禁止 include，但当前仅输出 WARNING 而不阻断编译。

**补充发现**：
- `ModelProviderInit.cpp` 是"注册工厂"，职责是将具体 infer 实现绑定到 `IModelManager`，这种依赖在某种程度上是合理的（工厂模式需要知道具体类型），但按分层原则应位于 libs 层而非 apps 层
- `PitchExtractionService.cpp` 直接调用 `Rmvpe::get_f0()` 和 `Game` 的方法，是典型的业务逻辑绕过抽象层直接使用底层实现
- `ExportService.cpp` 和 `ExportPage.cpp` 直接使用 infer 层类型做验证/推理，而非通过 `IModelProvider` 抽象接口

**修复方案**：
1. **ModelProviderInit.cpp**：移到 libs 层（如新建 `model-registry` 库），apps 层通过 `IModelManager` 间接使用
2. **PitchExtractionService.cpp**：改为通过 `IModelProvider` 接口调用推理，而非直接持有 `Rmvpe*` / `Game*`
3. **ExportService.cpp / ExportPage.cpp**：同上，通过 `IModelProvider` 获取推理结果
4. **SettingsPage.cpp**：`DictionaryG2P` 的配置项应通过 libs 层封装暴露，而非直接 include infer 层头文件
5. **LayerAudit.cmake**：将 WARNING 升级为 FATAL_ERROR，强制阻断违规编译

**风险项**：
- 修复工作量较大，涉及多个 Page 类的重构
- libs 层 Facade 可能需要新增接口类型
- 修改期间可能引入编译错误，需全量编译验证
- ModelProviderInit 移到 libs 层后，apps 层的初始化流程需要调整

---

## ARCH-02: hitTestBoundary() 在4个 Widget 中重复实现 — P-01 违规

| 属性 | 值 |
|------|-----|
| **严重度** | **高** |
| **违反准则** | P-01（模块职责单一，行为不得分散重复） |
| **验证状态** | ✅ 已确认 |
| **影响文件** | WaveformWidget.cpp:L321, SpectrogramWidget.cpp:L356, PowerWidget.cpp:L222, IntervalTierView.cpp:L137 |

**描述**：D-27 已提取 `BoundaryDragController` 统一了拖动逻辑，但 `hitTestBoundary()` 仍在四个 Widget 中各自实现。核心算法完全相同：遍历所有 tier 的 boundary，计算像素距离，筛选在 `kBoundaryHitWidth/2` 范围内的候选，按优先级排序（activeTier 优先 > 距离最近 > dragRoom 最大 > boundary 索引最小）。每个实现约 60-80 行，总计约 260 行重复代码。

**关键差异**（验证补充）：
- WaveformWidget/SpectrogramWidget 使用 `IBoundaryModel*`（已抽象化）
- PowerWidget/IntervalTierView 使用 `TextGridDocument*`（未抽象化，与 ARCH-06 联动）
- IntervalTierView 只处理单 tier（无 `outTier` 参数），签名不同
- `BoundaryDragController` 已集中管理拖拽状态机（startDrag/updateDrag/endDrag/cancelDrag），但**不包含 hitTest 逻辑**，其 `startDrag` 接收已 hit-test 完成后的 `(tierIndex, boundaryIndex)` 参数

**修复方案**：在 `BoundaryDragController` 中新增 `hitTestBoundary()` 方法，将算法集中化：

```cpp
struct HitTestResult {
    int tierIndex = -1;
    int boundaryIndex = -1;
};
HitTestResult hitTestBoundary(IBoundaryModel *model, int x,
                               const std::function<double(int)>& timeToX,
                               int widgetWidth, int hitWidth) const;
```

4 个 Widget 改为调用集中化的 hitTest，仅传入各自的 `timeToX` 转换函数。同时修正 PowerWidget 和 IntervalTierView 使其也使用 `IBoundaryModel*`（与 ARCH-06 联动）。

**风险项**：
- IntervalTierView 的签名不同（无 outTier），需要适配
- timeToX 转换函数的接口设计需考虑不同 Widget 的坐标系统差异
- 需要同时解决 PowerWidget/IntervalTierView 对 TextGridDocument 的直接依赖（ARCH-06）

---

## ARCH-03: drawBoundaryOverlay() 在3个 Widget 中重复实现 — P-01 违规

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **违反准则** | P-01 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | WaveformWidget.cpp:L192-214, SpectrogramWidget.cpp:L329-350, PowerWidget.cpp:L195-216 |

**描述**：三个子图 Widget 的 `drawBoundaryOverlay()` 方法逻辑完全相同：遍历 active tier 的 boundary，根据拖动/悬停/普通状态选择不同颜色的画笔绘制竖线。每处约 20 行，总计约 60 行重复代码。

**修复方案**：提取到 `BoundaryOverlayWidget` 或 `BoundaryDragController` 中提供 `drawBoundaryOverlay(QPainter&, IBoundaryModel*, BoundaryDragController*, int hoveredBoundary, std::function<int(double)> timeToX)` 工具方法。

**风险项**：低风险，纯提取重构。需注意 PowerWidget 使用 `TextGridDocument*` 而非 `IBoundaryModel*`（与 ARCH-06 联动）。

---

## ARCH-04: 鼠标事件处理模式在3个 Widget 中重复 — P-01 违规

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **违反准则** | P-01 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | WaveformWidget.cpp:L271-319, SpectrogramWidget.cpp:L421-468, PowerWidget.cpp:L287-334 |

**描述**：WaveformWidget、SpectrogramWidget、PowerWidget 的 `mousePressEvent`/`mouseMoveEvent`/`mouseReleaseEvent` 实现了完全相同的边界拖动交互模式：mousePressEvent 做 hitTest → startDrag / 开始视口拖动；mouseMoveEvent 做 updateDrag / 视口滚动 / hover 检测；mouseReleaseEvent 做 endDrag / 结束视口拖动。每个 Widget 约 50 行鼠标事件代码，总计约 150 行重复。

**修复方案**：创建 `ChartMouseHandler` 基类或 mixin，封装边界拖动 + 视口滚动 + hover 检测的统一交互逻辑。Widget 只需将鼠标事件委托给 handler。

**风险项**：
- WaveformWidget 额外支持 Shift+滚轮振幅调整（D-03），需要设计 hook 点
- 需要设计灵活的 hook 点让子类覆盖特定行为

---

## ARCH-05: xToTime()/timeToX()/wheelEvent()/contextMenuEvent() 在多个 Widget 中重复 — P-01 违规

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **违反准则** | P-01 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | WaveformWidget, SpectrogramWidget, PowerWidget, IntervalTierView |

**描述**：以下方法在多个 Widget 中几乎逐字重复：
- `xToTime()`/`timeToX()` — 4 个 Widget 完全相同
- `findSurroundingBoundaries()` — WaveformWidget 和 SpectrogramWidget 完全相同
- `wheelEvent()` — 3 个 Widget 几乎相同（Ctrl=缩放, 无修饰=滚动；WaveformWidget 额外支持 Shift=振幅）
- `contextMenuEvent()` — 3 个 Widget 几乎相同（右键播放当前区间）

**修复方案**：提取公共基类 `AudioChartWidget`（继承 QWidget），统一实现坐标转换、滚轮事件、右键播放、边界查找等通用行为。各子类只需实现 `paintEvent()` 和特定渲染逻辑。

**风险项**：
- 需要同时修改多个 Widget 的继承关系
- IntervalTierView 的坐标系统可能与其他三个不同
- WaveformWidget 的 Shift+滚轮振幅调整需要通过虚方法 hook

---

## ARCH-06: PowerWidget 和 IntervalTierView 直接依赖 TextGridDocument 而非 IBoundaryModel — 接口不一致

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **违反准则** | P-02（被动接口 + 容器通知）、接口一致性 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | PowerWidget.h:L24/L48/L94, PowerWidget.cpp:L2/L222-285, IntervalTierView.h:L12, IntervalTierView.cpp |

**描述**：WaveformWidget 和 SpectrogramWidget 使用 `IBoundaryModel*` 接口访问边界数据，而 PowerWidget 直接持有 `TextGridDocument* m_document`，IntervalTierView 直接持有 `TextGridDocument* m_doc`。

**验证补充**：
- PowerWidget 的 `hitTestBoundary()` 和 `drawBoundaryOverlay()` 调用的 `tierCount()`、`activeTierIndex()`、`boundaryCount()`、`boundaryTime()` 全部是 `IBoundaryModel` 接口已有的方法，可以纯机械替换
- PowerWidget 的 `m_dragController->startDrag(hitTier, boundaryIdx, m_document)` 已经将 `TextGridDocument*` 作为 `IBoundaryModel*` 传递给 BoundaryDragController，证明接口兼容
- IntervalTierView 额外调用了 `m_doc->intervalText()`、`m_doc->intervalStart()`、`m_doc->intervalEnd()`、`m_doc->isTierReadOnly()` 等方法，这些不在 `IBoundaryModel` 接口中

**修复方案**：
1. **PowerWidget**（简单）：将 `TextGridDocument *m_document` 替换为 `IBoundaryModel *m_boundaryModel`，新增 `setBoundaryModel(IBoundaryModel*)` 方法（与 WaveformWidget/SpectrogramWidget 对齐），移除 `setDocument()` 和 `#include "TextGridDocument.h"`。纯机械替换，无行为变化
2. **IntervalTierView**（需扩展接口）：需要扩展 `IBoundaryModel` 接口，新增 `intervalText()`、`intervalStart()`、`intervalEnd()`、`isTierReadOnly()` 等虚方法（或创建 `IIntervalModel` 子接口），然后才能替换。优先级低于 PowerWidget

**风险项**：
- PowerWidget 修复为纯机械替换，风险极低
- IntervalTierView 需要扩展 IBoundaryModel 接口，需评估对 SliceBoundaryModel 的影响

---

## ARCH-07: SlicerPage 与 DsSlicerPage 大量代码重复 — P-01 违规

| 属性 | 值 |
|------|-----|
| **严重度** | **高** |
| **违反准则** | P-01 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | SlicerPage.h/cpp (~865行), DsSlicerPage.h/cpp (~1241行) |

**描述**：SlicerPage（LabelSuite 用）和 DsSlicerPage（DsLabeler 用）共享几乎相同的成员变量布局、切片参数 UI、自动切片逻辑、音频加载逻辑、切片点管理、导出逻辑、工具模式。

**验证补充**（实际对比结果）：

相同的成员变量（30+ 个完全重复）：`m_undoStack`、`m_boundaryModel`、`m_playWidget`、`m_audioFileList`、`m_waveformWidget`、`m_spectrogramWidget`、`m_sliceListPanel`、`m_samples`、`m_sampleRate`、5 个 SpinBox（threshold/minLength/minInterval/hopSize/maxSilence）、6 个按钮（autoSlice/reSlice/importMarkers/saveMarkers/exportAudio）、`m_toolbar`、`m_btnPointer`/`m_btnKnife`、`ToolMode` 枚举、`m_slicePoints`、`m_selectedBoundary`、`m_fileSlicePoints`、`m_currentAudioPath`

相同的方法（18+ 个完全重复）：`buildLayout()`、`connectSignals()`、`onAutoSlice()`、`onImportMarkers()`、`onSaveMarkers()`、`onExportAudio()`、`onBatchExportAll()`、`onOpenAudioFiles()`、`onOpenAudioDirectory()`、`refreshBoundaries()`、`updateSlicerListPanel()`、`saveCurrentSlicePoints()`、`loadSlicePointsForFile()`、`updateFileProgress()`、`autoSliceFiles()`、`loadAudioFile()`、`keyPressEvent()`

DsSlicerPage 独有：`m_dataSource` (ProjectDataSource*)、`setDataSource()`、`onExportMenu()`、`promptSliceUpdateIfNeeded()`、`saveSlicerParamsToProject()`、`saveSlicerStateToProject()`

SlicerPage 独有：`m_viewport` (ViewportController*)、`m_tierLabel` (SliceTierLabel*)、`m_hSplitter` (QSplitter*)、`m_settings` (AppSettings)

**修复方案**：提取公共基类 `SlicerPageBase`，包含所有共享的 UI 构建、信号连接、切片逻辑、文件管理逻辑。SlicerPage 继承 `SlicerPageBase`，仅保留独立 App 特有的 `AppSettings` 持久化和布局恢复；DsSlicerPage 继承 `SlicerPageBase`，仅保留 `ProjectDataSource` 集成和项目持久化逻辑。预估可消除约 **600-800 行** 重复代码。

**风险项**：
- 两个 Page 的数据源接口不同，需要设计统一的 DataSource 抽象
- DsSlicerPage 的项目持久化逻辑需要保持独立
- 修改期间可能影响两个应用的切片功能

---

## ARCH-08: data-sources 是"依赖磁铁"模块 — 模块耦合

| 属性 | 值 |
|------|-----|
| **严重度** | **高** |
| **违反准则** | 模块低耦合原则 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | data-sources/CMakeLists.txt |

**描述**：`data-sources` 库的 CMake 依赖列表极其庞大（13 个内部库 + 3 个 Qt 模块 + 1 个第三方库），其中包含 4 个 infer 层库、3 个完整编辑器模块、可视化组件、音频处理工具。data-sources 本应是一个数据服务层，却承担了 UI 构建、推理调用、音频处理、项目持久化等多重职责。

**修复方案**：
1. 将 `data-sources` 拆分为更聚焦的子模块：
   - `slicer-service` — 切片逻辑（依赖 `audio-util`, `dsfw-core`）
   - `labeler-services` — 标注服务（依赖 `phoneme-editor`, `pitch-editor`）
   - `export-service` — 导出服务（依赖 `dstools-domain`）
   - `model-registry` — 模型注册（依赖 infer 层）
2. 将 infer 层依赖从 `PUBLIC` 降为 `PRIVATE`，或完全移除，通过 `IModelProvider` 接口间接使用
3. 编辑器模块依赖应改为 `PRIVATE`，避免传递污染

**风险项**：
- 拆分后 CMake 结构变复杂
- 需要重新设计 Page 之间的共享逻辑
- 可能影响增量编译时间

---

## ARCH-09: God Class — DsSlicerPage (~1241行)

| 属性 | 值 |
|------|-----|
| **严重度** | **高** |
| **违反准则** | 单一职责 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | DsSlicerPage.cpp |

**描述**：DsSlicerPage 承担了至少 **7 种不同职责**：UI 构建、切片逻辑、WAV 文件导出、项目持久化、音频解码、进度管理、用户交互提示。framework-architecture.md §15 已承认此问题但行数已从 945 增长到 1241。

**验证补充**（实际行数分布）：

| 职责 | 方法 | 行范围 | 行数 |
|------|------|--------|------|
| UI 构建 | `buildLayout()` | L70-212 | 142 |
| 信号连接 | `connectSignals()` | L214-316 | 102 |
| 自动切片 | `onAutoSlice()` | L318-367 | 49 |
| 导入/保存标记 | `onImportMarkers()`, `onSaveMarkers()` | L369-407 | 38 |
| 导出菜单 | `onExportMenu()` | L409-443 | 34 |
| 单文件导出 | `onExportAudio()` | L445-608 | 163 |
| 菜单栏 | `createMenuBar()` | L615-646 | 31 |
| 页面生命周期 | `onActivated()`, `onDeactivating()` | L652-725 | 78 |
| 批量自动切片 | `autoSliceFiles()` | L812-886 | 74 |
| 批量导出 | `onBatchExportAll()` | L888-1061 | 173 |
| 切片更新提示 | `promptSliceUpdateIfNeeded()` | L1063-1153 | 90 |
| 项目持久化 | `saveSlicerParamsToProject()`, `saveSlicerStateToProject()` | L1155-1186 | 31 |
| 音频加载 | `loadAudioFile()` | L1188-1233 | 45 |

**修复方案**：
1. **提取 `SliceExportService`**：将 `onExportAudio()` (163行) + `onBatchExportAll()` (173行) 中的 WAV 文件写入逻辑抽取为独立服务类
2. **提取 `AudioFileLoader`**：将 `loadAudioFile()` (45行) + `autoSliceFiles()` (74行) 中的音频解码+混音逻辑抽取
3. **提取 `SlicerProjectBridge`**：将 `saveSlicerParamsToProject()`, `saveSlicerStateToProject()`, `promptSliceUpdateIfNeeded()` 中的项目持久化逻辑抽取
4. 重构后 DsSlicerPage 应降至约 **300-400 行**，仅负责组合各服务并处理页面生命周期

**风险项**：与 ARCH-07 联动，建议先完成 SlicerPage/DsSlicerPage 去重再拆分。

---

## ARCH-10: God Class — 三个 Labeler Page (850-950行)

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **违反准则** | 单一职责 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | PitchLabelerPage.cpp (~947行), PhonemeLabelerPage.cpp (~884行), MinLabelPage.cpp (~859行) |

**描述**：三个 Labeler Page 均超过 850 行，各自混合了 UI 布局、推理引擎加载、批量处理逻辑、数据加载/保存、ServiceLocator 查询、异步任务管理。虽然 EditorPageBase 已提取了部分公共逻辑，但各 Page 中的推理调用和批量处理逻辑仍然庞大。

**修复方案**：
1. 推理调用逻辑通过 libs 层 Facade 封装（与 ARCH-01 联动）
2. 批量处理逻辑提取到独立的 BatchProcessor 服务类
3. ServiceLocator::get<IModelManager>() 改为构造函数注入

**风险项**：中等风险，需要仔细设计 Facade 接口。

---

## ARCH-11: ServiceLocator 在 Page 中直接查询 IModelManager — 隐式依赖

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **违反准则** | 依赖注入应显式化 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | PhonemeLabelerPage.cpp:418, PitchLabelerPage.cpp:294/328, MinLabelPage.cpp:341 |

**描述**：三个 Labeler Page 在运行时通过 `ServiceLocator::get<IModelManager>()` 获取模型管理器，而非通过构造函数或 setter 注入。IModelManager 是真正的全局服务（P-08 允许通过 ServiceLocator），但获取方式应显式化。

**修复方案**：通过 EditorPageBase 构造函数或 setter 注入 IModelManager，而非运行时查询 ServiceLocator。这样 Page 与 ServiceLocator 不再隐式耦合，也可在不注册 ServiceLocator 的情况下测试。

**风险项**：低风险，但需要修改所有 Page 的构造函数签名。

---

## ARCH-12: IExportFormat/IQualityMetrics 通过 ServiceLocator 注册 — P-08 边界案例

| 属性 | 值 |
|------|-----|
| **严重度** | 低 |
| **违反准则** | P-08（ServiceLocator 仅用于全局服务） |
| **验证状态** | ✅ 已确认 |
| **影响文件** | LabelSuite main.cpp:59-60 |

**描述**：LabelSuite 的 main.cpp 中将 `IExportFormat` 和 `IQualityMetrics` 注册到 ServiceLocator，但这两个服务是 LabelSuite 特有的（DsLabeler 不使用），属于"应用级"而非"全局级"服务。

**修复方案**：将 IExportFormat/IQualityMetrics 改为通过 AppShell 或 EditorPageBase 注入，而非 ServiceLocator 全局注册。或者如果确认所有应用都需要，则在 AppInit 中统一注册。

**风险项**：低风险，仅影响 LabelSuite 应用。

---

## ARCH-13: DsLabeler main.cpp 包含大量业务逻辑 — God Function

| 属性 | 值 |
|------|-----|
| **严重度** | 中 |
| **违反准则** | 单一职责 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | DsLabeler main.cpp:154-270 |

**描述**：DsLabeler 的 main.cpp 包含约 120 行的"页面切换守卫"业务逻辑（检查切片一致性、弹窗提示用户回到 Slicer 页面）。这段逻辑涉及遍历项目 items 和 slicerState、比较切点数量和位置、构建问题文件列表、弹出 QMessageBox，不属于 main() 函数的职责。

**修复方案**：提取 `SliceConsistencyChecker` 类到 data-sources 或 domain 层，封装切片一致性检查逻辑。main.cpp 中仅连接信号。

**风险项**：低风险，纯提取重构。

---

## ARCH-14: IBoundaryModel 接口过胖 — 接口隔离问题

| 属性 | 值 |
|------|-----|
| **严重度** | 低 |
| **违反准则** | 接口隔离原则 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | IBoundaryModel.h |

**描述**：IBoundaryModel 包含 11 个虚方法，混合了只读查询（tierCount, boundaryTime, totalDuration）和写操作（moveBoundary），以及可选功能（supportsBinding, supportsInsert, snapToNearestBoundary）。对于仅需要绘制边界线的 Widget，它被迫依赖包含写方法的接口。

**修复方案**：考虑拆分为 `IBoundaryReader`（只读）和 `IBoundaryEditor`（继承 IBoundaryReader，增加 moveBoundary）。但考虑到当前消费者数量有限，且 ARCH-06 修复后 PowerWidget/IntervalTierView 也将统一使用 IBoundaryModel，此为低优先级改进。

**风险项**：当前消费者数量有限，低优先级。

---

## ARCH-15: src/ui-core/ 和 src/widgets/ 与 src/framework/ 双层结构混乱

| 属性 | 值 |
|------|-----|
| **严重度** | 低 |
| **违反准则** | 目录结构清晰性 |
| **验证状态** | ✅ 已确认 |
| **影响文件** | src/ui-core/CMakeLists.txt, src/widgets/CMakeLists.txt |

**描述**：项目同时存在 `src/framework/ui-core/` (dsfw-ui-core) 和 `src/ui-core/` (dstools-ui-core)，以及 `src/framework/widgets/` (dsfw-widgets) 和 `src/widgets/` (dstools-widgets)。`src/ui-core/` 仅包含一个文件 AppInit.h/cpp，`src/widgets/` 包含领域特定 Widget。但 src/CMakeLists.txt 中将它们与 domain 并列添加，造成目录结构与逻辑层级不一致。

**修复方案**：将 `src/ui-core/` 的内容（AppInit）移入 `src/apps/shared/` 或 `src/domain/`；将 `src/widgets/` 的内容移入 `src/apps/shared/`。保持 `src/framework/` 下的 dsfw-* 模块为唯一的框架层。

**风险项**：需要修改多个 CMakeLists.txt 的路径引用。

---

## 修复优先级建议

### 第一批（高优先级，解决层级违规和核心 P-01 问题）

| ID | 工作量 | ROI | 前置依赖 |
|----|--------|-----|---------|
| ARCH-06 | 小 | PowerWidget 接口一致性，为 ARCH-02 铺路 | 无 |
| ARCH-02 | 中 | 消除 260 行重复代码 | ARCH-06 |
| ARCH-01 | 大 | 恢复分层架构，减少编译依赖 | 无 |
| ARCH-07 | 大 | 消除 SlicerPage/DsSlicerPage 重复（600-800行） | 无 |

### 第二批（中优先级，消除 Widget 层重复代码）

| ID | 工作量 | ROI | 说明 |
|----|--------|-----|------|
| ARCH-03/04/05 | 中 | 一次性解决 Widget 层所有重复 | 建议合并为 AudioChartWidget 基类提取 |
| ARCH-08 | 大 | data-sources 拆分 | 与 ARCH-01 联动 |

### 第三批（低优先级，改善代码质量）

| ID | 工作量 | ROI | 说明 |
|----|--------|-----|------|
| ARCH-09/10 | 中 | God Class 拆分 | ARCH-07 完成后进行 |
| ARCH-11/12/13 | 小 | 依赖注入改善 | |
| ARCH-14/15 | 小 | 接口和目录结构优化 | |
