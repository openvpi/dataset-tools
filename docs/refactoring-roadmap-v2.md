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

## 待实现功能（非 Bug 修复）

### T.1 DsLabeler CSV 预览

ExportPage 内部 TabWidget："导出设置" | "预览数据"
- QTableView + QStandardItemModel（只读）
- 点击"预览数据" tab 时调用 CsvAdapter 实时生成
- 缺少前置数据的行标红

### T.2 快捷键系统

详见 [human-decisions.md](human-decisions.md) D-13。

| 页面 | 快捷键 |
|------|--------|
| Slicer | S=自动切片, E=导出, I=导入切点, P=指针, K=切刀 |
| PhonemeLabeler | F=FA, ←/→=切换切片 |
| PitchLabeler | F=提取音高, M=MIDI, ←/→=切换切片 |
| MinLabel | R=ASR, ←/→=切换切片 |

### T.3 子图顺序配置 UI

Settings 页面增加"图表顺序"可拖拽排序列表。

***

## 任务清单

> **分类**：任务 1-8 = Bug 修复与 UX 改进；任务 9-10 = 编辑器页面去重；任务 11-15 = 路径管理与推理 UX

### 1. 视口统一比例尺系统重设计（D-24）

> 参考 vLabeler（Kotlin/Compose）的 resolution 模型，适配到本项目的 Qt ViewportController 流式视口架构。

#### 设计理念

**vLabeler 模型**：`resolution` = 每像素代表多少个采样点（整数，默认 40，范围 10-400），整个音频被渲染为一个定宽的可滚动画布（
`canvasLength = dataLength / resolution`）。刻度线通过预定义的 major/minor 表查找合适级别（`MIN_MINOR_STEP_PX = 80`）。各子图高度由
`heightWeight` 配比。

**本项目适配**：保留 ViewportController 的 startSec/endSec 流式视口（不预渲染整段音频），但引入 **resolution 概念**作为 PPS
的反面，并以 `defaultResolution` 定义每张图的初始比例尺。

#### 核心概念

```
resolution = samplesPerPixel = sampleRate / pixelsPerSecond
pixelsPerSecond = sampleRate / resolution

默认 resolution = 40（与 vLabeler 一致）
→ 44100Hz 音频的默认 PPS = 44100 / 40 = 1102.5
→ 1 分钟音频的完整画布宽度 = 60 * 1102.5 = 66150px（虚拟，只渲染可见部分）
```

#### 架构重设计

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
    int m_resolution = 40;               // 默认值（与 vLabeler 一致）
    int m_minResolution = 10;            // zoom in 极限
    int m_maxResolution = 400;           // zoom out 极限
    int m_resolutionStep = 5;            // 每次缩放的步长
    double m_viewStartSec = 0.0;
    double m_viewEndSec = 10.0;
};
```

**关键改进**：

- `resolution` 是**整数**，步进式变化（不会出现浮点精度问题）
- 极限是硬编码的 min=10, max=400（对应 PPS 4410 \~ 110 @44.1kHz），永远不会出现"到极限后继续变"的 bug
- `zoomIn/zoomOut` 替代原来的 `zoomAt(center, factor)`，语义更清晰
- 不再需要 `clampAndEmit` 中的特殊保护——resolution 本身是离散有界的
- [ ] 重写 ViewportController 为 resolution 驱动模型
- [ ] 保留 `setViewRange()` / `scrollBy()` 不变（向后兼容）
- [ ] 新增 `setAudioParams()` 替代 `setTotalDuration()`
- [ ] Resolution step 按对数步进（10→15→20→30→40→60→80→100→150→200→300→400）而非线性

#### Step 2：TimeRulerWidget 刻度计算改为查表法

参考 vLabeler 的 `Timescale.find()`：

```cpp
struct TimescaleLevel {
    double majorSec;    // 主刻度间隔（秒）
    double minorSec;    // 次刻度间隔（秒）
};

static const TimescaleLevel kLevels[] = {
    {0.001,  0.0005},   // 1ms / 0.5ms
    {0.005,  0.001},
    {0.01,   0.005},
    {0.05,   0.01},
    {0.1,    0.05},
    {0.5,    0.1},
    {1.0,    0.5},
    {5.0,    1.0},
    {15.0,   5.0},
    {30.0,   10.0},
    {60.0,   15.0},
    {300.0,  60.0},
    {900.0,  300.0},
    {3600.0, 900.0},
};
constexpr double kMinMinorStepPx = 60.0; // vLabeler 用 80，我们用 60（刻度更密）

TimescaleLevel findLevel(double pps) {
    for (auto &level : kLevels) {
        if (level.minorSec * pps >= kMinMinorStepPx)
            return level;
    }
    return kLevels[std::size(kLevels) - 1];
}
```

**优势**：

- 不再用渐显渐隐（smoothStep/spacingVisibility）——直接选中一个刻度级别
- 主刻度有文字标签，次刻度只画短线
- 刻度疏密**完全由 resolution（即 PPS）决定**，不会出现"图表缩放到极限但刻度继续变化"
- [ ] 用查表法替代当前 7 层循环 + smoothStep 渐隐逻辑
- [ ] 主刻度：长线 + 时间标签（格式自动选择 ms/s/min:sec/h:min:sec）
- [ ] 次刻度：短线，无标签
- [ ] `kMinMinorStepPx` 可在 Settings 中配置（影响刻度密度）

#### Step 3：每张图的默认高度比例

参考 vLabeler 的 `heightWeight` 模型：

```cpp
struct ChartConfig {
    QString id;
    double heightWeight;    // 默认高度权重
    bool enabled;           // 是否显示
};

// 默认配置
static const ChartConfig kDefaultCharts[] = {
    {"waveform",    1.0,  true},
    {"power",       0.5,  true},
    {"spectrogram", 0.75, true},
};
```

AudioVisualizerContainer 在首次加载时按 heightWeight 设置 QSplitter 比例：

```cpp
void AudioVisualizerContainer::applyDefaultHeightRatios() {
    QList<int> sizes;
    int totalHeight = m_chartSplitter->height();
    double totalWeight = 0;
    for (const auto &id : m_chartOrder) {
        auto it = m_charts.find(id);
        if (it != m_charts.end())
            totalWeight += it->heightWeight;
    }
    for (const auto &id : m_chartOrder) {
        auto it = m_charts.find(id);
        if (it != m_charts.end())
            sizes.append(int(totalHeight * it->heightWeight / totalWeight));
    }
    m_chartSplitter->setSizes(sizes);
}
```

- [ ] `addChart()` 增加 `double heightWeight` 参数
- [ ] 首次打开（无保存状态时）按 heightWeight 比例初始化 splitter
- [ ] 用户手动拖动 splitter 后保存到 AppSettings（已有 `saveSplitterState`）
- [ ] Settings 页可重置为默认比例

#### Step 4：默认 resolution 的"Fit to Window"

打开音频时自动计算一个让音频填满窗口宽度的 resolution：

```cpp
void AudioVisualizerContainer::fitToWindow() {
    int fitResolution = m_viewport->totalSamples() / width();
    fitResolution = std::clamp(fitResolution, 10, 400);
    m_viewport->setResolution(fitResolution);
    m_viewport->setViewRange(0, m_viewport->totalDuration());
}
```

- [ ] 打开新音频时自动 `fitToWindow()`
- [ ] 用户可通过快捷键（如 Home 键或双击 MiniMap）重置为 fitToWindow
- [ ] 用户缩放后不再自动 fit（直到下一次打开新音频）

#### Step 5：wheelEvent 统一与 MiniMap 修正

- [ ] 所有子图的 Ctrl+滚轮 → 调用 `m_viewport->zoomIn(centerSec)` / `zoomOut(centerSec)`
- [ ] ViewportController 的 `zoomIn/zoomOut` 内部按对数步进表递进，永远不会超出 \[10, 400] 范围
- [ ] MiniMapScrollBar wheelEvent：无 modifier = 横向滚动，Ctrl = 缩放
- [ ] WaveformWidget Shift+滚轮 → 仅调整振幅（不影响 resolution/视口）

#### Step 6：与 ViewportState 信号的兼容

现有子图通过 `setViewport(ViewportState)` 接收视口状态：

```cpp
struct ViewportState {
    double startSec;
    double endSec;
    double pixelsPerSecond;  // = sampleRate / resolution
};
```

**不需要修改子图**：ViewportController 每次 resolution 变化或 scroll 时照常 emit `viewportChanged(state)`，子图的
`setViewport()` 接口不变。改动仅在 ViewportController 内部。

- [ ] 确认 `ViewportState::pixelsPerSecond` 始终 = `sampleRate / resolution`
- [ ] 子图的 `m_pixelsPerSecond` 副本仍从 state 获取（无需改动）

#### Step 7：验证矩阵

- [ ] Ctrl+滚轮 zoom in 到 resolution=10 → 硬停，视口/刻度不再变化
- [ ] Ctrl+滚轮 zoom out 到 resolution=400 → 硬停
- [ ] 刻度线密度始终与 PPS 严格对应（通过查表法保证）
- [ ] 打开 1 秒音频 → fitToWindow resolution 正常（≈10\~50 depending on width）
- [ ] 打开 1 小时音频 → fitToWindow resolution 正常（≈400 or capped）
- [ ] resize 窗口 → 视口的可见时间范围自动调整（PPS 不变，但可见 duration = width / PPS 变化）
- [ ] 刻度线在任何 zoom level 都清晰可读、不消失、不过密
- [ ] 各子图 x 坐标与刻度线完美对齐
- [ ] MiniMap 滚轮 = 横向滚动，Ctrl+滚轮 = 缩放
- [ ] splitter 默认比例 = heightWeight，用户调整后持久化

### 2. 最右侧边界线无法拖动（D-22）

**问题**：TextGridDocument::clampBoundaryTime 中最后一条边界的 nextBoundary 取了自身 interval 的 max\_time，等于边界本身，clamp
区间为 0。

**修复**：

- [ ] `TextGridDocument::clampBoundaryTime()`：`boundaryIndex == count - 1` 时 nextBoundary 使用 `tier->GetMaxTime()`
  （文档总时长）
- [ ] 排查 `SliceBoundaryModel::clampBoundaryTime()` 是否有类似逻辑问题
- [ ] 验证：最后一条边界可自由拖动到音频结尾
- [ ] 全面测试各 widget（WaveformWidget、SpectrogramWidget、PowerWidget）的 boundary drag 是否正常

### 3. Phoneme 标签区域无数据时显示提示（D-21）

**问题**：`PhonemeTextGridTierLabel` 在 tierCount == 0 时高度为 0，标签区域消失。

**修复**：

- [ ] `PhonemeTextGridTierLabel::rebuildRadioButtons()`：tierCount == 0 时保持最小高度 `kTierRowHeight`
- [ ] `PhonemeTextGridTierLabel::paintEvent()`：tierCount == 0 时绘制居中灰色文字 "No label data"
- [ ] 验证：切换到未标注切片时标签区域仍可见

### 4. 最近工程列表标灰无效路径（D-23）

**修复**：

- [ ] `WelcomePage::refreshRecentList()`：对每个路径检查 `QFileInfo::exists()`
- [ ] 不存在的路径：item 文字灰色 + 删除线（`QFont::setStrikeOut(true)` + `setForeground(Qt::gray)`）
- [ ] 双击不存在的路径：弹出 QMessageBox "工程文件不存在，是否从列表中移除？"，确认后从 QSettings 中删除并刷新
- [ ] 验证：删除 .dsproj 文件后打开 DsLabeler，对应条目显示为灰色删除线

### 5. 删除废弃的 SliceNumberLayer（D-25）

- [ ] 删除 `src/apps/shared/data-sources/SliceNumberLayer.h`
- [ ] 删除 `src/apps/shared/data-sources/SliceNumberLayer.cpp`
- [ ] 从 CMakeLists.txt 移除（如有引用）
- [ ] 确认无其他文件 include 或引用该类

### 6. IEditorDataSource 层统一音频文件有效性管理（D-26）

**问题**：音频文件存在性检查散落在各页面代码中，且只有单条操作（`runFaForSlice` 等）经过 `SliceListPanel::validateAudioPath()` 保护，所有批量操作（批量 FA/ASR/Pitch/MIDI、ExportService auto-completion）全部绕过检查，直接把不存在的路径传给推理引擎。

**受影响代码**：PhonemeLabelerPage `onBatchFA()`、MinLabelPage 批量 ASR、PitchLabelerPage 批量 Pitch/MIDI、ExportService auto-completion。

**设计方案**：在 `IEditorDataSource` 基类层统一解决，而非在每个消费者各自检查。

#### Step 1：IEditorDataSource 新增音频有效性接口

```cpp
class IEditorDataSource : public QObject {
    Q_OBJECT
public:
    // --- 现有接口不变 ---
    virtual QStringList sliceIds() const = 0;
    virtual QString audioPath(const QString &sliceId) const = 0;
    // ...

    // --- 新增 ---

    /// 音频文件是否存在于文件系统。
    /// 默认实现：QFile::exists(audioPath(sliceId))。
    /// 子类可缓存结果以避免重复 stat。
    [[nodiscard]] virtual bool audioExists(const QString &sliceId) const;

    /// 返回一个已验证的音频路径：文件存在则返回路径，不存在返回空字符串。
    /// 这是所有推理操作统一使用的入口，替代直接调用 audioPath()。
    [[nodiscard]] QString validatedAudioPath(const QString &sliceId) const;

    /// 返回所有音频文件缺失的 sliceId 列表。
    [[nodiscard]] QStringList missingAudioSlices() const;

signals:
    // --- 现有信号不变 ---
    void sliceListChanged();
    void sliceSaved(const QString &sliceId);

    // --- 新增 ---
    /// 音频可用性发生变化（文件出现/消失）。
    void audioAvailabilityChanged();
};
```

默认实现（`IEditorDataSource.cpp`）：

```cpp
bool IEditorDataSource::audioExists(const QString &sliceId) const {
    const QString path = audioPath(sliceId);
    return !path.isEmpty() && QFile::exists(path);
}

QString IEditorDataSource::validatedAudioPath(const QString &sliceId) const {
    const QString path = audioPath(sliceId);
    if (path.isEmpty() || !QFile::exists(path))
        return {};
    return path;
}

QStringList IEditorDataSource::missingAudioSlices() const {
    QStringList missing;
    for (const auto &id : sliceIds()) {
        if (!audioExists(id))
            missing.append(id);
    }
    return missing;
}
```

- [ ] `IEditorDataSource` 新增 `audioExists()`、`validatedAudioPath()`、`missingAudioSlices()`、`audioAvailabilityChanged` 信号
- [ ] 默认实现基于 `QFile::exists(audioPath())`

#### Step 2：SliceListPanel 自动标记缺失音频

`SliceListPanel::refresh()` 在构建列表时调用 `source->audioExists(sliceId)`，对缺失音频的条目：
- 文字前加 `⚠` 图标
- 前景色设为灰色 / 橙色
- tooltip 显示 "音频文件不存在: {path}"

```cpp
void SliceListPanel::refresh() {
    // ... 现有逻辑 ...
    for (const auto &id : ids) {
        // ... 现有文字构建 ...
        if (!m_source->audioExists(id)) {
            item->setForeground(QColor(200, 120, 50));  // 橙色
            item->setIcon(QIcon(":/icons/warning.svg"));
            item->setToolTip(
                QStringLiteral("音频文件不存在: %1")
                    .arg(m_source->audioPath(id)));
        }
    }
}
```

连接 `audioAvailabilityChanged` → `refresh()` 使文件状态变化时自动刷新。

- [ ] `SliceListPanel::refresh()` 中标记缺失音频条目（图标 + 颜色 + tooltip）
- [ ] 连接 `audioAvailabilityChanged` 信号到 `refresh()`

#### Step 3：所有推理消费者统一使用 validatedAudioPath

全局替换：所有批量/单条推理操作中的 `source->audioPath(sliceId)` → `source->validatedAudioPath(sliceId)`。

这样 `isEmpty()` 检查同时覆盖了"无路径"和"文件不存在"两种情况，无需额外的 `QFile::exists()` 调用。

| 文件 | 改动 |
|---|---|
| `PhonemeLabelerPage.cpp` | `onBatchFA()` 第584行、`runFaForSlice()` 第416行 |
| `MinLabelPage.cpp` | 批量 ASR 第504行、`runAsrForSlice()` |
| `PitchLabelerPage.cpp` | 批量 Pitch 第892行、`runPitchForSlice()`、`runMidiForSlice()` |
| `ExportService.cpp` | `processSlice()` 第122行 |
| `PhonemeLabelerPage.cpp` `onSliceSelected()` 第107行 | 替换 `validateAudioPath` 静态方法 |
| `PitchLabelerPage.cpp` `onSliceSelected()` | 同上 |
| `MinLabelPage.cpp` `onSliceSelected()` | 同上 |

- [ ] 所有推理调用点 `audioPath()` → `validatedAudioPath()`
- [ ] 删除 `SliceListPanel::validateAudioPath()` 静态方法（职责已移入基类）
- [ ] 批量操作结束时报告跳过数（如 "批量 FA 完成: 10/12 个切片，2 个音频文件缺失已跳过"）

#### Step 4：ProjectDataSource 缓存优化（可选）

`ProjectDataSource` 可在 `setProject()` 时预扫描所有 audioSource 文件存在性，缓存到 `QSet<QString> m_missingAudio`，避免批量操作中对每个 slice 重复 `stat()`。在 `audioAvailabilityChanged` 时或用户手动刷新时重新扫描。

```cpp
class ProjectDataSource : public IEditorDataSource {
    // ...
    bool audioExists(const QString &sliceId) const override;
    void rescanAudioAvailability();  // 重新扫描，emit audioAvailabilityChanged
private:
    mutable QSet<QString> m_missingAudioIds;
    mutable bool m_audioScanned = false;
};
```

- [ ] `ProjectDataSource` override `audioExists()` 使用缓存
- [ ] `setProject()` 时触发异步预扫描
- [ ] 提供 `rescanAudioAvailability()` 供手动刷新

#### 验证矩阵

- [ ] dsproj 中 audioSource 指向不存在文件 → SliceListPanel 条目显示橙色警告图标
- [ ] 选中缺失音频的切片 → 编辑器加载空音频，不触发自动推理，无报错
- [ ] 批量 FA/ASR/Pitch/MIDI → 自动跳过缺失音频，完成后报告跳过数
- [ ] ExportService auto-completion → 跳过缺失音频的推理补全，不报 0 采样错误
- [ ] 切片导出后文件出现 → rescan 后 SliceListPanel 更新为正常状态
- [ ] LabelSuite (DirectoryDataSource) 和 DsLabeler (ProjectDataSource) 均正常工作

### 7. dsproj audioSource 路径健壮性（D-27）

**问题**：dsproj 中 audioSource 存储绝对路径，项目目录移动后路径全部失效；路径解析 fallback 静默无日志。

**修复**：

- [ ] `ProjectDataSource::sliceAudioPath()`：当 `item.audioSource` 不存在时，尝试以 `sliceId + ".wav"` 在 `wavs/` 目录下查找（启发式 fallback）
- [ ] `ProjectDataSource::sliceAudioPath()` fallback 到 `ProjectPaths::sliceAudioPath()` 时增加 `qWarning()` 日志
- [ ] 考虑在 dsproj 保存时将 audioSource 转为相对于 workingDirectory 的路径（如果可能）

### 8. dsproj 数据模型其他问题（D-28）

**问题**：多处设计不完善。

- [ ] **ExportConfig 格式重复**：`DsProject::load()` 中 `defaults.export.formats` 和顶层 `export.formats` 都追加到 `m_exportConfig.formats`，可能产生重复。加载后去重或优先使用顶层。
- [ ] **defaults round-trip 残留**：`save()` 使用 `m_extraFields` 保留未知字段，已废弃的 `defaults` 会残留。保存时应从 `m_extraFields` 中移除 `defaults` key。
- [ ] **slice.in/out 语义文档化**：in/out 在切片后表示原始音频中的位置（microseconds），但 audioSource 指向独立 wav。在 DsProject.h 或 ds-format.md 中明确文档说明此语义。
- [ ] **版本校验**：`load()` 时检查 `version` 字段，不兼容版本给出警告。

***

依赖：任务 6（IEditorDataSource 新接口）完成后执行。

### 现状与复核结论

三个编辑器页面（MinLabelPage、PhonemeLabelerPage、PitchLabelerPage）直接继承 `QWidget`，未使用已存在的 `EditorPageBase`。逐项复核后，**部分重复值得合并，部分不值得**：

#### ✅ 值得合并（真正相同、无差异、合并后降低维护成本）

| 重复模式 | 理由 |
|---|---|
| **公共成员变量声明**（m_sliceList, m_splitter, m_source, m_settingsBackend, m_currentSliceId, m_settings, m_shortcutManager, m_prevAction, m_nextAction） | 三页完全一致，无变体 |
| **构造函数骨架**（SliceListPanel + QSplitter + layout + stretch factor） | 三页字面相同 |
| **setDataSource()** | 三页字面相同 |
| **onDeactivating()**（save splitter + maybeSave） | 三页字面相同（Phoneme/Pitch 多一个 editorSplitter，但可用 virtual hook） |
| **onDeactivated()**（disable shortcuts） | 公共部分相同 |
| **onShutdown()**（saveAll） | 三页字面相同 |
| **windowTitle()** | 结构相同，仅前缀不同 |
| **sliceSelected 连接 + saveSelection** | 三页字面相同 |

#### ⚠️ 结构相似但细节不同（不宜强行统一，提取 helper 即可）

| 模式 | 差异点 | 结论 |
|---|---|---|
| **maybeSave()** | "脏"判定方式不同：MinLabel 用 `m_dirty`，Phoneme 用 `document()->isModified()`，Pitch 用 `undoStack()->isClean()` | 对话框部分可提 helper，脏检查留子类 |
| **onActivated()** | 公共的 8 行相同（refresh+restore+ensureSelection），但自动推理逻辑完全不同（MinLabel 只查 dirty+toast，Phoneme 有 FA 自动触发，Pitch 有双引擎自动触发） | 公共前 8 行提基类，自动推理留 virtual hook |
| **createMenuBar()** | 菜单项名称和组合不同（MinLabel 有播放菜单、无 undo/redo；Phoneme 有 undo/redo；Pitch 有 zoom/AB compare） | **不合并**。菜单是 UI 身份的一部分，强行用 populateXxxMenu() 虚方法只会增加理解成本 |
| **createStatusBarContent()** | sliceLabel 相同，但 Phoneme 加 posLabel，Pitch 加 pos+zoom+noteCount | **不合并**。状态栏各页差异大 |

#### ❌ 不值得合并（看似相同但本质不同，强行抽象会增加复杂度）

| 模式 | 理由 |
|---|---|
| **ensureXxxEngine() 模板化** | MinLabel 用 `FunAsrModelProvider`（自定义 provider，不走 `InferenceModelProvider<T>`），与其他三个不同源。isOpen/initialized/is_open 检查方式不统一。模板化后需要额外的 traits/concept 适配层，增加了复杂度但只消除约 20 行/页 |
| **InferenceRunner 通用执行器** | 三种推理的输入准备完全不同（Phoneme 需先加载 grapheme 构造 WordList，Pitch 是两个引擎串行，MinLabel 最简单）。结果回调的签名也不同。用 `std::function<bool(audioPath)>` 抹平差异会丢失类型安全，且 lambda 捕获列表变复杂 |
| **批量推理统一** | 同上，各批量操作的循环体内逻辑差异大，统一接口会变成一个过度设计的"万能"执行器 |

### 9. 激活 EditorPageBase，合并确定性重复代码（D-29）

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
    void setupBaseLayout(QWidget *editorWidget);  // SliceListPanel + QSplitter + layout
    void setupNavigationActions();                  // 创建 prev/next + connect

    // 生命周期公共部分（子类调用 base:: 后追加自己的逻辑）
    void onActivated() override;       // setEnabled + refresh + restoreSplitter + ensureSelection
    bool onDeactivating() override;    // saveSplitter + maybeSave
    void onDeactivated() override;     // setEnabled(false)
    void onShutdown() override;        // saveAll

    // 公共实现
    void setDataSource(IEditorDataSource *source, ISettingsBackend *settingsBackend);
    QString windowTitle() const override;  // windowTitlePrefix() + " — " + sliceId

    // 子类必须实现
    virtual QString windowTitlePrefix() const = 0;
    virtual bool isDirty() const = 0;       // 各页面脏检查方式不同
    virtual bool saveCurrentSlice() = 0;
    virtual void onSliceSelectedImpl(const QString &sliceId) = 0;

    // 可选钩子
    virtual void restoreExtraSplitters() {}  // Phoneme/Pitch 有 editorSplitter
    virtual void saveExtraSplitters() {}
    virtual void onAutoInfer() {}            // onActivated 中自动推理

    // 公共辅助
    bool maybeSaveDialog();  // Save/Discard/Cancel 对话框（不含脏检查）

signals:
    void sliceChanged(const QString &sliceId);
};
```

- [ ] `EditorPageBase` 实现上述公共成员和方法
- [ ] 三个页面改为继承 `EditorPageBase`
- [ ] 删除各页面中字面重复的代码（构造骨架、setDataSource、生命周期公共部分、windowTitle）
- [ ] `maybeSave()` 各页面自行实现（调用 `isDirty()` + `maybeSaveDialog()`）
- [ ] `createMenuBar()` / `createStatusBarContent()` 不合并，各页面保留独立实现
- [ ] `ensureXxxEngine()` 不合并，各页面保留独立实现
- [ ] 验证：三页面行为不变

### 10. 删除废弃的 EditorPageBase 旧实现（D-33）

当前 `EditorPageBase.cpp` 有一些与新设计冲突的旧实现（英文菜单、不同的 maybeSave 逻辑），需清理后重写。

- [ ] 清除旧的 `createMenuBar()` 英文实现
- [ ] 清除旧的 `maybeSave()`（不含 QMessageBox 的简陋版本）
- [ ] 确认无其他代码依赖旧行为

### 不做的事项（复核后排除）

| 原提案 | 排除理由 |
|---|---|
| ~~任务 10 ensureEngine\<T\>() 模板~~ | MinLabel 的 ASR provider 不走通用模板，强行统一需额外 traits 层，投入产出比低 |
| ~~VI.3 InferenceRunner~~ | 三种推理的输入准备和结果处理差异太大，统一接口会过度抽象。任务 6 的 `validatedAudioPath()` 已统一解决音频检查问题，各页面只需改一行 |
| ~~VI.4 菜单骨架统一~~ | 菜单是各页面的 UI 身份，populateXxxMenu() 虚方法增加间接层但不减少实质复杂度 |

### 预期效果（保守估计）

| 变化 | 行数 |
|---|---|
| 三页面各减少 ~60 行公共骨架 | -180 |
| EditorPageBase 新增实现 | +100 |
| **净减少** | **~80行** |

收益不在行数，而在：
1. **新增页面成本降低**：未来新建编辑器页面只需继承 EditorPageBase + 实现 4 个虚方法
2. **生命周期 bug 统一修复**：splitter 保存/恢复、ensureSelection 时机等只需改一处
3. **任务 6 音频有效性检查自动传播**：基类的 `onSliceSelected` 统一调用 `validatedAudioPath()`

***

### 11. 统一路径管理模块（D-30）

**问题**：路径编码处理散落在各模块，各自用不同方式处理 Unicode 路径，导致中文/日文路径在 Windows 上频繁出错。

**现状分析**：

| 模块 | 路径来源 | 传递方式 | 已知问题 |
|---|---|---|---|
| `dsfw::PathUtils` | 框架层 | `QString → std::filesystem::path` | ✅ 已正确处理 wstring |
| `dstools::toFsPath` | domain 层 | `QString → std::filesystem::path` | ✅ 已正确处理 wstring |
| `DsProject` | 项目文件 | `toPosixPath / fromPosixPath` | 仅做 `/` ↔ `\` 转换 |
| `AudioUtil::resample_to_vio` | 推理层 | `std::filesystem::path → SndfileHandle` | ⚠ 已修 wstring，但 FlacDecoder 也需要 |
| `SndfileHandle` 直接使用 | 各推理引擎 | `path.string()` | ❌ ANSI codepage 破坏 CJK |
| `std::ifstream` | 模型加载 | `path.string()` | ⚠ MSVC 的 ifstream 接受 path，但 string() 版本不安全 |

**设计方案**：在 `audio-util` 层新增轻量路径工具（不依赖 Qt），供所有推理引擎使用。

#### Step 1：audio-util 新增 PathCompat.h

```cpp
// audio-util/include/audio-util/PathCompat.h
#pragma once
#include <filesystem>
#include <sndfile.hh>

namespace AudioUtil {

/// Open a SndfileHandle with correct encoding on all platforms.
/// On Windows: uses sf_wchar_open via wstring().c_str().
/// On Unix: uses string() (UTF-8).
inline SndfileHandle openSndfile(const std::filesystem::path &path,
                                  int mode = SFM_READ,
                                  int format = 0,
                                  int channels = 0,
                                  int samplerate = 0) {
#ifdef _WIN32
    return SndfileHandle(path.wstring().c_str(), mode, format, channels, samplerate);
#else
    return SndfileHandle(path.string(), mode, format, channels, samplerate);
#endif
}

/// Open a std::ifstream with correct encoding on all platforms.
/// On Windows: passes std::filesystem::path directly (MSVC supports it).
/// On Unix: uses string().
inline std::ifstream openIfstream(const std::filesystem::path &path,
                                   std::ios_base::openmode mode = std::ios_base::in) {
    return std::ifstream(path, mode);
}

} // namespace AudioUtil
```

- [ ] 创建 `audio-util/include/audio-util/PathCompat.h`
- [ ] `resample_to_vio` 中的 `SndfileHandle(filepath.string())` 替换为 `openSndfile(filepath)`
- [ ] `FlacDecoder.cpp` 中的 `SndfileHandle(filepath.string())` 替换为 `openSndfile(filepath)`
- [ ] `write_vio_to_wav` 中的 `SndfileHandle(filepath.string(), SFM_WRITE, ...)` 替换为 `openSndfile(filepath, SFM_WRITE, ...)`
- [ ] `GameModel.cpp` 中的 `std::ifstream((path).string())` 替换为 `openIfstream(path)`
- [ ] Hfa.cpp `JsonUtils::loadFile` 内部的 ifstream 也需检查

#### Step 2：统一推理引擎的路径入口

所有推理引擎（HFA、RMVPE、GAME）的公共接口统一使用 `std::filesystem::path`，内部通过 `PathCompat.h` 处理平台差异。

**约定**：
- 上层（Qt 代码）通过 `dstools::toFsPath(qStringPath)` 或 `dsfw::PathUtils::toStdPath(qStringPath)` 转换
- 推理层通过 `AudioUtil::openSndfile(path)` 打开音频文件
- 永远不要在推理层代码中出现 `path.string()` 用于文件 I/O

- [ ] 全局搜索 `path.string()` 用于文件 I/O 的场景，逐一替换
- [ ] 在 CONTRIBUTING.md 或代码规范中记录此约定

#### Step 3：dsfw::PathUtils 与 dstools::toFsPath 合并

两个重复的路径转换工具：
- `dstools::toFsPath()`（domain 层，inline）
- `dsfw::PathUtils::toStdPath()`（框架层，非 inline）

- [ ] `dstools::toFsPath` 改为调用 `dsfw::PathUtils::toStdPath` 的 inline wrapper，消除重复
- [ ] 或将 `dstools::toFsPath` 标记为 deprecated，迁移使用者

#### Step 4：macOS NFD 兼容

macOS HFS+ 文件系统使用 NFD（分解形式）Unicode。Qt 内部处理了这个问题，但直接使用 `std::filesystem::path` 与 C 库交互时可能出现文件名不匹配。

- [ ] `dsfw::PathUtils::normalize()` 已实现 NFC 标准化（macOS），确认所有路径比较都经过 normalize
- [ ] 音频文件存在性检查（`QFile::exists`）在 macOS 上验证 NFD/NFC 兼容性

#### 验证矩阵

- [ ] Windows：中文路径音频 → FA/Pitch/MIDI 推理成功
- [ ] Windows：日文路径音频 → FA/Pitch/MIDI 推理成功
- [ ] Windows：路径含空格 → 推理成功
- [ ] macOS：含中文的路径 → 推理成功
- [ ] Linux：UTF-8 路径 → 推理成功
- [ ] dsproj 项目目录移动后 → audioSource fallback 正常
- [ ] 跨平台 VM 共享目录（Windows host ↔ macOS guest）→ 路径解析正常

### 12. FA 自动对齐非阻塞化 + 状态指示（D-31）

**问题**：`onAutoInfer()` 在 `onActivated()` 中同步调用 `ensureHfaEngine()`，模型加载可能阻塞 UI 数秒。FA 运行期间虽然在后台线程，但 tier 标签区域没有状态指示，用户不知道正在处理。

**设计方案**：

#### Step 1：ensureEngine 异步化

```cpp
// EditorPageBase 新增
signals:
    void engineReady(const QString &engineKey);

// PhonemeLabelerPage
void PhonemeLabelerPage::ensureHfaEngineAsync() {
    if (m_hfa && m_hfa->isOpen()) {
        emit engineReady("phoneme_alignment");
        return;
    }
    
    // 在后台线程加载模型
    auto config = readModelConfig(settingsBackend(), "phoneme_alignment");
    if (config.modelPath.isEmpty()) return;
    
    QtConcurrent::run([this, config]() {
        // ... 加载模型 ...
        QMetaObject::invokeMethod(this, [this]() {
            emit engineReady("phoneme_alignment");
        }, Qt::QueuedConnection);
    });
}
```

- [ ] `ensureHfaEngine()` 拆分为同步检查 + 异步加载
- [ ] 模型加载期间 UI 不阻塞
- [ ] 模型加载完成后通过信号触发自动 FA

#### Step 2：TierLabel 状态指示

FA 运行期间在 `PhonemeTextGridTierLabel` 显示 "正在对齐..." 动画：

```cpp
class PhonemeTextGridTierLabel : public TierLabelArea {
public:
    void setAlignmentRunning(bool running);
    
protected:
    void paintEvent(QPaintEvent *) override {
        // ... 现有绘制 ...
        if (m_alignmentRunning) {
            // 在标签区域右上角显示旋转图标或 "⏳ 对齐中..."
            painter.setPen(QColor(100, 180, 255));
            painter.drawText(rect().adjusted(0, 2, -4, 0),
                           Qt::AlignTop | Qt::AlignRight,
                           tr("对齐中..."));
        }
    }
};
```

- [ ] `PhonemeTextGridTierLabel` 新增 `setAlignmentRunning(bool)` 
- [ ] FA 开始时设置 `true`，完成/失败时设置 `false`
- [ ] 标签区域绘制对齐状态文字/动画

#### Step 3：页面切换时的 FA 中断

- [ ] 切换到其他切片时，如果 FA 正在运行，取消当前 FA（或让其完成但丢弃结果）
- [ ] 切换到其他页面时（`onDeactivating`），等待 FA 完成或取消
- [ ] FA 结果回调检查 sliceId 是否仍为当前切片

#### 验证矩阵

- [ ] 切换到 Phoneme 页面 → 模型加载不阻塞 UI（可以操作列表）
- [ ] 模型加载完成 → 自动触发 FA
- [ ] FA 运行中 → 标签区域显示 "对齐中..."
- [ ] FA 运行中切换切片 → 旧 FA 结果不应用到新切片
- [ ] FA 完成 → 标签区域恢复正常，音素层自动刷新

### 13. 所有自动推理/补全操作非阻塞化（D-32）

**问题**：不仅 FA，所有自动推理和批量操作都存在界面阻塞风险。

**受影响模块**：

| 模块 | 阻塞操作 | 当前状态 |
|---|---|---|
| PhonemeLabelerPage | `ensureHfaEngine()` + auto FA | ⚠ 模型加载阻塞，FA 本身后台 |
| PitchLabelerPage | `ensureRmvpeEngine()` + `ensureGameEngine()` + auto Pitch/MIDI | ⚠ 模型加载阻塞，推理本身后台 |
| MinLabelPage | `ensureAsrEngine()` + auto ASR 检查 | ⚠ 模型加载阻塞 |
| ExportPage `onExport()` | `ensureEngines()` + 逐切片 `autoCompleteSlice()` | ❌ **完全阻塞主线程**（processEvents 不充分） |
| ExportPage `runValidation()` | 遍历所有切片检查完整性 | ⚠ 大项目可能卡顿 |

**设计原则**：

1. **模型加载一律异步**：所有 `ensureXxxEngine()` 改为 `ensureXxxEngineAsync()`，通过信号通知就绪
2. **批量操作一律 QtConcurrent**：ExportPage 的 autoComplete 循环移入后台线程
3. **进度反馈统一**：所有后台操作通过 `QMetaObject::invokeMethod` 回主线程更新 UI
4. **结果丢弃**：所有回调检查 sliceId/页面状态是否仍有效

#### Step 1：统一异步模型加载基础设施

> **归属说明**：`loadEngineAsync` 将作为 Phase 任务 9 `EditorPageBase` 的一部分实现，而非独立新建。任务 12/任务 13 的异步引擎设施合并到 任务 9 的 EditorPageBase 定义中。

在 `EditorPageBase` 中提供通用的异步模型加载框架：

```cpp
// EditorPageBase 新增 protected 方法
// 注意：loadFunc 返回 Result<void> 而非 bool，确保失败时根因可追溯（P-04）
void EditorPageBase::loadEngineAsync(const QString &taskKey,
                                      std::function<Result<void>()> loadFunc,
                                      std::function<void()> onReady,
                                      std::function<void(const QString &error)> onFailed = {});
```

**错误处理（P-04 合规）**：
- `loadFunc` 返回 `Result<void>`，失败时包含根因错误消息
- 失败时回主线程调用 `onFailed(error)`，默认行为：`DSFW_LOG_ERROR("infer", error)` + Toast 通知用户
- UI 层通过 Toast 或状态栏显示具体失败原因（如 "模型加载失败: 文件不存在 /path/to/model.onnx"）
- 禁止静默失败或仅记日志不通知用户

- [ ] `EditorPageBase` 新增 `loadEngineAsync()` 通用方法（含错误回调）
- [ ] PhonemeLabelerPage `ensureHfaEngine()` → `ensureHfaEngineAsync()` + 回调触发自动 FA
- [ ] PitchLabelerPage `ensureRmvpeEngine()` / `ensureGameEngine()` → 异步版本
- [ ] MinLabelPage `ensureAsrEngine()` → 异步版本
- [ ] ExportPage `ensureEngines()` → 异步版本，所有引擎并行加载

#### Step 2：ExportPage 自动补全后台化

```cpp
void ExportPage::onExport() {
    // ...
    if (m_chkAutoComplete->isChecked()) {
        ensureEnginesAsync([this, sliceIds, outputDir]() {
            // 引擎就绪后在后台线程执行补全
            QtConcurrent::run([this, sliceIds]() {
                for (const auto &sliceId : sliceIds) {
                    autoCompleteSlice(sliceId);
                    QMetaObject::invokeMethod(this, [this, sliceId]() {
                        m_progressBar->setValue(m_progressBar->value() + 1);
                        m_statusLabel->setText("补全: " + sliceId);
                    }, Qt::QueuedConnection);
                }
                QMetaObject::invokeMethod(this, [this]() {
                    doActualExport(); // 补全完成后执行实际导出
                }, Qt::QueuedConnection);
            });
        });
        return; // 异步返回，不阻塞
    }
    doActualExport();
}
```

- [ ] ExportPage `onExport()` 中的 autoComplete 循环移入 `QtConcurrent::run`
- [ ] 删除 `QApplication::processEvents()` 调用（反模式）
- [ ] 导出操作本身也后台化（大项目可能有大量文件操作）
- [ ] 导出期间禁用导出按钮 + 显示取消按钮

#### Step 3：PitchLabelerPage 恢复丢失的 onAutoInfer

Phase VI 重构时 PitchLabelerPage 的自动推理逻辑丢失（原 `onActivated` 中的 needAutoInfer 逻辑未迁移到 `onAutoInfer()`）。

- [ ] PitchLabelerPage override `onAutoInfer()`，恢复原有的 dirty layers 检查 + auto pitch/midi 逻辑
- [ ] 使用异步 ensureEngine（同 Step 1）

#### 验证矩阵

- [ ] ExportPage 自动补全 100+ 切片 → UI 不卡顿，进度条实时更新
- [ ] 切换到 Pitch 页面 → 模型加载不阻塞，自动推理在后台执行
- [ ] 所有模型加载期间 → 切片列表可正常操作
- [ ] 自动推理期间切换切片 → 旧结果丢弃

### 14. CSV 预览表明暗主题适配（D-33）

**问题**：`ExportPage` 的 CSV 预览 `QTableView` 在暗色主题下对比度不足，看不清内容。当前 QSS 只有 `QHeaderView::section` 样式，缺少 `QTableView` 本体样式。

**修复方案**：在 `dark.qss` 和 `light.qss` 中补充 `QTableView` 完整样式。

```css
/* dark.qss */
QTableView {
    background-color: #1E1F22;
    alternate-background-color: #26282E;
    color: #BCBEC4;
    gridline-color: #393B40;
    selection-background-color: #2E436E;
    selection-color: #FFFFFF;
    border: 1px solid #393B40;
}

QTableView::item {
    padding: 4px 8px;
    border: none;
}

QTableView::item:hover {
    background-color: #2E3135;
}

QTableView::item:selected {
    background-color: #2E436E;
}

/* light.qss */
QTableView {
    background-color: #FFFFFF;
    alternate-background-color: #F7F8FA;
    color: #1E1F22;
    gridline-color: #EBECF0;
    selection-background-color: #DAEBF9;
    selection-color: #1E1F22;
    border: 1px solid #EBECF0;
}

QTableView::item {
    padding: 4px 8px;
    border: none;
}

QTableView::item:hover {
    background-color: #EFF1F5;
}

QTableView::item:selected {
    background-color: #DAEBF9;
}
```

- [ ] `dark.qss` 添加 QTableView 完整样式
- [ ] `light.qss` 添加 QTableView 完整样式
- [ ] 确保 `setAlternatingRowColors(true)` 的 alternate 色可见
- [ ] 验证：ExportPage CSV 预览在暗色/亮色主题下均清晰可读

### 15. AudioVisualizerContainer 文档/模型刷新框架（D-34）— 紧急

**问题**：`IBoundaryModel` 是纯数据接口，无变更通知能力。当模型底层数据变化（TextGrid 重载、SliceBoundaryModel 切片点更新）时，持有 model 指针的可视化组件无从得知，需要外部手动重新 set 或逐一 update。这导致：
1. PhonemeEditor：FA 结果不刷新、切换切片丢标签
2. SlicerPage：切片点变化后也需要手动调 `setBoundaryModel` + `update`
3. 未来任何新的可视化页面都会遇到同样的问题

**受影响组件**：

| 使用者 | 模型实现 | 当前刷新方式 | 问题 |
|---|---|---|---|
| PhonemeEditor | TextGridDocument | `loadAudio` 时重新 set，`loadFromDsText` 时**不刷新** | ❌ 关键 bug |
| SlicerPage | SliceBoundaryModel | 手动 `setBoundaryModel` + `update` | ⚠ 分散，易遗漏 |
| DsSlicerPage | SliceBoundaryModel | 同上 | ⚠ 同上 |

**设计方案**：在 `AudioVisualizerContainer` 层提供统一的 "model invalidated" 机制。

#### 方案 A（推荐）：AudioVisualizerContainer 增加 `invalidateBoundaryModel()`

不修改 `IBoundaryModel` 接口（它是纯数据，不应有 QObject 依赖），而是让容器提供刷新入口：

```cpp
class AudioVisualizerContainer : public QWidget {
public:
    // ... 现有接口 ...
    
    /// 通知容器：boundary model 的底层数据已变化，需要全面刷新。
    /// 由外部在修改 model 数据后调用。
    /// 内部行为：
    ///   1. BoundaryOverlayWidget::update()
    ///   2. TierLabelArea::onModelDataChanged()（虚方法，子类重写）
    ///   3. 所有注册的 chart widgets::update()
    void invalidateBoundaryModel();
    
signals:
    /// 发射时机：invalidateBoundaryModel() 被调用后。
    /// 外部组件（如 TierEditWidget、EntryListPanel）可连接此信号。
    void boundaryModelInvalidated();
};
```

调用点（外部）：
```cpp
// PhonemeEditor — loadFromDsText 之后
connect(m_document, &TextGridDocument::documentChanged, this, [this]() {
    m_container->invalidateBoundaryModel();
    // 额外的 PhonemeEditor 特有刷新（TierEditWidget 等）
    m_tierEditWidget->rebuildTierViews();
    m_entryListPanel->rebuildEntries();
});

// SlicerPage — 切片点变化后
m_boundaryModel->setSlicePoints(newPoints);
m_container->invalidateBoundaryModel();  // 一行替代原来散落的 update 调用
```

- [ ] `AudioVisualizerContainer` 新增 `invalidateBoundaryModel()` 方法
- [ ] `invalidateBoundaryModel()` 内部：`m_boundaryOverlay->update()` + `m_tierLabelArea->update()` + 遍历 chart widgets 调用 `update()`
- [ ] 新增 `boundaryModelInvalidated()` 信号供外部组件连接
- [ ] `TierLabelArea` 新增虚方法 `onModelDataChanged()` — 默认 `update()`，`PhonemeTextGridTierLabel` 重写为 `rebuildRadioButtons() + update()`

#### 方案 B（备选，不推荐）：IBoundaryModel 添加信号

需要让 IBoundaryModel 继承 QObject 或使用观察者模式。侵入性太大，且 SliceBoundaryModel 不是 QObject，改动代价高。**不采用。**

#### Step 1：AudioVisualizerContainer 实现

```cpp
void AudioVisualizerContainer::invalidateBoundaryModel() {
    // 刷新 overlay
    if (m_boundaryOverlay)
        m_boundaryOverlay->update();
    
    // 刷新 tier label
    if (m_tierLabelArea)
        m_tierLabelArea->onModelDataChanged();
    
    // 刷新所有 chart widgets
    for (const auto &id : m_chartOrder) {
        auto it = m_charts.find(id);
        if (it != m_charts.end() && it->widget)
            it->widget->update();
    }
    
    emit boundaryModelInvalidated();
}
```

- [ ] 实现 `invalidateBoundaryModel()`
- [ ] `TierLabelArea` 新增 `virtual void onModelDataChanged()` { `update()`; }
  > **命名说明**：`onModelDataChanged` 是容器内部的模板方法 hook（由 `invalidateBoundaryModel()` 调用），不是信号槽回调。这不违反 P-02（P-02 禁止的是在纯数据接口上加 QObject/信号，而非容器内部的虚方法 hook）。
- [ ] `PhonemeTextGridTierLabel` override `onModelDataChanged()` → `rebuildRadioButtons()` + 调整高度

#### Step 2：PhonemeEditor 连接

```cpp
// PhonemeEditor::connectSignals() 新增：
connect(m_document, &TextGridDocument::documentChanged, this, [this]() {
    m_container->invalidateBoundaryModel();
    m_tierEditWidget->rebuildTierViews();
    m_entryListPanel->rebuildEntries();
    emit modificationChanged(m_document->isModified());
});
```

- [ ] `documentChanged` 连接到 container `invalidateBoundaryModel()` + 页面特有组件刷新
- [ ] 删除 `loadAudio` 中每次都调用的 `setBoundaryModel/setDocument`（改为只在首次构建时调用）
- [ ] `loadAudio` 中只处理音频加载，不再重复设置 model（model 指针不变，数据变化通过 invalidate 通知）

#### Step 3：Slicer 页面适配

```cpp
// SlicerPage 切片点更新后：
m_boundaryModel->setSlicePoints(newPoints);
m_container->invalidateBoundaryModel();  // 替代原来的 3-4 行分散 update
```

- [ ] `SlicerPage` 切片点变化时调用 `m_container->invalidateBoundaryModel()`
- [ ] `DsSlicerPage` 同上
- [ ] 删除两个 Slicer 页面中手动逐个 widget `update()` 的分散代码

#### Step 4：PhonemeTextGridTierLabel 空状态

- [ ] `onModelDataChanged()` override：如果 `boundaryModel()->tierCount() == 0` → 固定高度 `kTierRowHeight`，`paintEvent` 绘制 "No label data"
- [ ] 如果 `tierCount() > 0` → `rebuildRadioButtons()`，高度 = `max(tierCount, 1) * kTierRowHeight`
- [ ] 确保 FA 完成后（`documentChanged` → `invalidateBoundaryModel` → `onModelDataChanged`）自动从 "No label data" 切换到 tier buttons

#### 验证矩阵

- [ ] PhonemeEditor：`loadFromDsText` 后边界线、标签、tier edit、entry list 全部正确刷新
- [ ] PhonemeEditor：FA 完成后所有 UI 立即更新
- [ ] PhonemeEditor：切换切片后旧数据清除，新数据正确显示
- [ ] PhonemeEditor：空切片显示 "No label data"，tier label 保持最小高度
- [ ] SlicerPage：移动/添加/删除切片点后波形、频谱、overlay 立即刷新
- [ ] DsSlicerPage：同上
- [ ] 新增可视化页面只需调用 `m_container->invalidateBoundaryModel()` 即可刷新

***

## 执行顺序

```
任务 1-8（设计完成，待实现）:
  任务 1 视口统一比例尺 ── 待实现（最大任务）
  任务 2 最右侧边界线   ── 待实现
  任务 3 Phoneme 空状态  ── 待实现
  任务 4 最近工程列表    ── 待实现
  任务 5 删除 SliceNumberLayer ── 待实现
  任务 6 音频有效性管理  ── 待实现
  任务 7 audioSource 路径 ── 待实现
  任务 8 dsproj 数据模型  ── 待实现

任务 9-10（设计完成，待实现）:
  任务 9 EditorPageBase 合并 ── 待实现（依赖 任务 6）
  任务 10 旧实现清理          ── 待实现（依赖 任务 9）

任务 11-15（可并行）:
  任务 15 ──→ 紧急，优先执行（P-01/P-02 核心）
  任务 11 ──┐
  任务 12 ──┤ 可并行
  任务 13 ──┤ 任务 13 依赖 任务 12 的设计模式；loadEngineAsync 归入 任务 9 的 EditorPageBase
  任务 14 ──┘ 独立，随时可做
```

### 执行优先级建议

1. **任务 15** — 解决关键刷新 bug（PhonemeEditor FA 结果不刷新），且为 P-01/P-02 原则的核心实现
2. **任务 2** — 简单修复，影响用户体验
3. **任务 5** — 简单清理，减少代码噪音
4. **任务 3 + 任务 4** — 小型 UX 修复
5. **任务 6** — 中型重构，为 任务 9 铺路
6. **任务 11** — 路径健壮性，修复中文路径 bug
7. **任务 9 + 任务 10** — 去重，依赖 任务 6
8. **任务 12 + 任务 13** — 异步化，较大改动
9. **任务 1** — 最大任务，可独立进行
10. **任务 7 + 任务 8 + 任务 14** — 低优先级修复

---

## 关联文档

- [human-decisions.md](human-decisions.md) — 人工决策记录（最高优先级）
- [unified-app-design.md](unified-app-design.md) — 应用设计方案
- [framework-architecture.md](framework-architecture.md) — 项目架构
- [conventions-and-standards.md](conventions-and-standards.md) — 编码规范

