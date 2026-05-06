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

### 2.1 视口统一比例尺系统（原任务 1）

> 参考 vLabeler（Kotlin/Compose）的 resolution 模型，适配到本项目的 Qt ViewportController 流式视口架构。

#### 设计理念

**vLabeler 模型**：`resolution` = 每像素代表多少个采样点（整数，默认 40，范围 10-400），整个音频被渲染为一个定宽的可滚动画布（`canvasLength = dataLength / resolution`）。刻度线通过预定义的 major/minor 表查找合适级别（`MIN_MINOR_STEP_PX = 80`）。各子图高度由 `heightWeight` 配比。

**本项目适配**：保留 ViewportController 的 startSec/endSec 流式视口（不预渲染整段音频），但引入 **resolution 概念**作为 PPS 的反面，并以 `defaultResolution` 定义每张图的初始比例尺。

#### 核心概念

```
resolution = samplesPerPixel = sampleRate / pixelsPerSecond
pixelsPerSecond = sampleRate / resolution

默认 resolution = 40（与 vLabeler 一致）
→ 44100Hz 音频的默认 PPS = 44100 / 40 = 1102.5
→ 1 分钟音频的完整画布宽度 = 60 * 1102.5 = 66150px（虚拟，只渲染可见部分）
```

```
AudioVisualizerContainer
├── ViewportController（单一真相源）
│   ├── resolution: int（当前分辨率，可缩放）
│   ├── sampleRate: int（音频采样率）
│   ├── totalSamples: int64（音频总采样数）
│   ├── viewStartSec / viewEndSec（可见区间）
│   ├── pixelsPerSecond() = sampleRate / resolution（派生，只读）
│   ├── totalDuration() = totalSamples / sampleRate（派生）
│   └── canvasLengthPx() = totalSamples / resolution（虚拟总宽度）
│
├── MiniMapScrollBar（显示完整音频缩略图 + 滑窗）
├── TimeRulerWidget（刻度线 — 从 ViewportController 的 resolution 和 sampleRate 计算刻度）
├── TierLabelArea（标签区域）
├── QSplitter
│   ├── WaveformWidget（默认 heightWeight = 1.0）
│   ├── PowerWidget（默认 heightWeight = 0.5）
│   └── SpectrogramWidget（默认 heightWeight = 0.75）
└── BoundaryOverlayWidget
```

#### Step 1：ViewportController 改为 resolution 驱动

```cpp
class ViewportController {
public:
    // === 设置音频参数（加载音频时调用一次） ===
    void setAudioParams(int sampleRate, int64_t totalSamples);
    
    // === Resolution（核心缩放状态） ===
    void setResolution(int resolution);    // 每像素采样数
    int resolution() const;
    
    // === 缩放 ===
    void zoomIn(double centerSec);         // resolution -= step (zoom in)
    void zoomOut(double centerSec);        // resolution += step (zoom out)
    bool canZoomIn() const { return m_resolution > m_minResolution; }
    bool canZoomOut() const { return m_resolution < m_maxResolution; }
    
    // === 派生量（只读） ===
    double pixelsPerSecond() const { return double(m_sampleRate) / m_resolution; }
    double totalDuration() const { return double(m_totalSamples) / m_sampleRate; }
    double canvasLength() const { return double(m_totalSamples) / m_resolution; }
    
    // === 视口（可见区间） ===
    void setViewRange(double startSec, double endSec);
    void scrollBy(double deltaSec);
    double startSec() const;
    double endSec() const;

private:
    int m_sampleRate = 44100;
    int64_t m_totalSamples = 0;
    int m_resolution = 40;
    int m_minResolution = 10;
    int m_maxResolution = 400;
    int m_resolutionStep = 5;
    double m_viewStartSec = 0.0;
    double m_viewEndSec = 10.0;
};
```

- [ ] 重写 ViewportController 为 resolution 驱动模型
- [ ] 保留 `setViewRange()` / `scrollBy()` 不变（向后兼容）
- [ ] 新增 `setAudioParams()` 替代 `setTotalDuration()`
- [ ] Resolution step 按对数步进（10→15→20→30→40→60→80→100→150→200→300→400）

#### Step 2：TimeRulerWidget 刻度计算改为查表法

```cpp
struct TimescaleLevel {
    double majorSec;
    double minorSec;
};

static const TimescaleLevel kLevels[] = {
    {0.001,  0.0005}, {0.005,  0.001},  {0.01,   0.005},
    {0.05,   0.01},   {0.1,    0.05},   {0.5,    0.1},
    {1.0,    0.5},    {5.0,    1.0},    {15.0,   5.0},
    {30.0,   10.0},   {60.0,   15.0},   {300.0,  60.0},
    {900.0,  300.0},  {3600.0, 900.0},
};
constexpr double kMinMinorStepPx = 60.0;

TimescaleLevel findLevel(double pps) {
    for (auto &level : kLevels) {
        if (level.minorSec * pps >= kMinMinorStepPx)
            return level;
    }
    return kLevels[std::size(kLevels) - 1];
}
```

- [ ] 用查表法替代当前 7 层循环 + smoothStep 渐隐逻辑
- [ ] 主刻度：长线 + 时间标签（格式自动选择 ms/s/min:sec/h:min:sec）
- [ ] 次刻度：短线，无标签
- [ ] `kMinMinorStepPx` 可在 Settings 中配置

#### Step 3：每张图的默认高度比例

- [ ] `addChart()` 增加 `double heightWeight` 参数（已实现）
- [ ] 首次打开（无保存状态时）按 heightWeight 比例初始化 splitter
- [ ] 用户手动拖动 splitter 后保存到 AppSettings

#### Step 4：默认 resolution 的 "Fit to Window"

- [ ] 打开新音频时自动 `fitToWindow()`（已实现 `AudioVisualizerContainer::fitToWindow()`）
- [ ] 用户缩放后不再自动 fit（直到下一次打开新音频）

#### Step 5：wheelEvent 统一

- [ ] 所有子图的 Ctrl+滚轮 → 调用 `m_viewport->zoomIn(centerSec)` / `zoomOut(centerSec)`
- [ ] ViewportController 的 `zoomIn/zoomOut` 内部按对数步进表递进，不超过 [10, 400]
- [ ] MiniMapScrollBar wheelEvent：无 modifier = 横向滚动，Ctrl = 缩放

#### Step 6：验证矩阵

- [ ] Ctrl+滚轮 zoom in 到 resolution=10 → 硬停
- [ ] Ctrl+滚轮 zoom out 到 resolution=400 → 硬停
- [ ] 刻度线密度始终与 PPS 严格对应
- [ ] 各子图 x 坐标与刻度线完美对齐
- [ ] MiniMap 滚轮 = 横向滚动，Ctrl+滚轮 = 缩放

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

1. **1.1-1.5** — 显示与 UX 修复（紧急，影响用户直接体验）
2. **3.1、3.2、3.4** — 简单边界/标签修复
3. **5.1** — 刷新框架（关键 bug：FA 结果不刷新）
4. **2.1** — 视口统一比例尺（最大任务，可独立进行）
5. **3.5、3.6、3.7** — 数据健壮性
6. **4.1、4.2** — 编辑器去重
7. **5.2、5.3** — 推理异步化
8. **6.1** — 路径管理
9. **6.2-6.5、3.3** — 低优先级完善

---

## 关联文档

- [human-decisions.md](human-decisions.md) — 人工决策记录（最高优先级）
- [unified-app-design.md](unified-app-design.md) — 应用设计方案
- [framework-architecture.md](framework-architecture.md) — 项目架构
- [conventions-and-standards.md](conventions-and-standards.md) — 编码规范
