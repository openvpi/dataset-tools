# 重构路线图 — 剩余任务

> 2026-05-05
>
> 前置文档：[human-decisions.md](human-decisions.md)、[refactoring-v2.md](refactoring-v2.md)

---

## 已完成的阶段（仅供参考）

Phase Q（紧急修复）、Phase R（配置架构）、Phase S（可视化架构）、Phase U（文档清理）**全部已完成**。
详见 git 历史（branch: refactor, 从 `545102f` 到 `0b8efc5`）。

---

## Phase T — 功能增强

依赖：Phase S 已完成。

### T.1 DsLabeler CSV 预览页

**状态**：已完成。

- [x] ExportPage 构建 TabWidget："导出设置" | "预览数据"
- [x] "预览数据" tab：点击时调用 CsvAdapter 生成内存 CSV
- [x] QTableView + QStandardItemModel 只读展示
- [x] 缺少前置数据的行标红（grapheme 层缺失时）

### T.2 快捷键系统

**状态**：DsSlicerPage 菜单已添加快捷键（S=自动切片, Ctrl+O=打开文件等）。其他页面已实现。

- [x] DsSlicerPage 菜单快捷键
- [x] PhonemeLabelerPage 快捷键（F=强制对齐, ←/→=切换切片）
- [x] PitchLabelerPage 快捷键（F=提取音高, M=提取MIDI, ←/→=切换切片）
- [x] MinLabelPage 快捷键（R=ASR 识别, ←/→=切换切片）
- [x] 工具按钮 tooltip 标注快捷键
- [x] AppShell 页面切换时激活/停用对应快捷键组

### T.3 Settings 页子图顺序配置 UI

- [x] Settings 页增加"图表顺序"配置项
- [x] 可拖拽排序的列表（waveform / power / spectrogram / viseme 等）
- [x] 变更后实时更新 AudioVisualizerContainer

---

## Phase V — Bug 修复与 UX 改进

依赖：无，可立即执行。优先级高于 Phase T。

### V.1 视口统一比例尺系统重设计（D-24）

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
- 极限是硬编码的 min=10, max=400（对应 PPS 4410 ~ 110 @44.1kHz），永远不会出现"到极限后继续变"的 bug
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
- [ ] ViewportController 的 `zoomIn/zoomOut` 内部按对数步进表递进，永远不会超出 [10, 400] 范围
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

**不需要修改子图**：ViewportController 每次 resolution 变化或 scroll 时照常 emit `viewportChanged(state)`，子图的 `setViewport()` 接口不变。改动仅在 ViewportController 内部。

- [ ] 确认 `ViewportState::pixelsPerSecond` 始终 = `sampleRate / resolution`
- [ ] 子图的 `m_pixelsPerSecond` 副本仍从 state 获取（无需改动）

#### Step 7：验证矩阵

- [ ] Ctrl+滚轮 zoom in 到 resolution=10 → 硬停，视口/刻度不再变化
- [ ] Ctrl+滚轮 zoom out 到 resolution=400 → 硬停
- [ ] 刻度线密度始终与 PPS 严格对应（通过查表法保证）
- [ ] 打开 1 秒音频 → fitToWindow resolution 正常（≈10~50 depending on width）
- [ ] 打开 1 小时音频 → fitToWindow resolution 正常（≈400 or capped）
- [ ] resize 窗口 → 视口的可见时间范围自动调整（PPS 不变，但可见 duration = width / PPS 变化）
- [ ] 刻度线在任何 zoom level 都清晰可读、不消失、不过密
- [ ] 各子图 x 坐标与刻度线完美对齐
- [ ] MiniMap 滚轮 = 横向滚动，Ctrl+滚轮 = 缩放
- [ ] splitter 默认比例 = heightWeight，用户调整后持久化

### V.2 最右侧边界线无法拖动（D-22）

**问题**：TextGridDocument::clampBoundaryTime 中最后一条边界的 nextBoundary 取了自身 interval 的 max_time，等于边界本身，clamp 区间为 0。

**修复**：

- [ ] `TextGridDocument::clampBoundaryTime()`：`boundaryIndex == count - 1` 时 nextBoundary 使用 `tier->GetMaxTime()`（文档总时长）
- [ ] 排查 `SliceBoundaryModel::clampBoundaryTime()` 是否有类似逻辑问题
- [ ] 验证：最后一条边界可自由拖动到音频结尾
- [ ] 全面测试各 widget（WaveformWidget、SpectrogramWidget、PowerWidget）的 boundary drag 是否正常

### V.3 Phoneme 标签区域无数据时显示提示（D-21）

**问题**：`PhonemeTextGridTierLabel` 在 tierCount == 0 时高度为 0，标签区域消失。

**修复**：

- [ ] `PhonemeTextGridTierLabel::rebuildRadioButtons()`：tierCount == 0 时保持最小高度 `kTierRowHeight`
- [ ] `PhonemeTextGridTierLabel::paintEvent()`：tierCount == 0 时绘制居中灰色文字 "No label data"
- [ ] 验证：切换到未标注切片时标签区域仍可见

### V.4 最近工程列表标灰无效路径（D-23）

**修复**：

- [ ] `WelcomePage::refreshRecentList()`：对每个路径检查 `QFileInfo::exists()`
- [ ] 不存在的路径：item 文字灰色 + 删除线（`QFont::setStrikeOut(true)` + `setForeground(Qt::gray)`）
- [ ] 双击不存在的路径：弹出 QMessageBox "工程文件不存在，是否从列表中移除？"，确认后从 QSettings 中删除并刷新
- [ ] 验证：删除 .dsproj 文件后打开 DsLabeler，对应条目显示为灰色删除线

### V.5 删除废弃的 SliceNumberLayer（D-25）

- [ ] 删除 `src/apps/shared/data-sources/SliceNumberLayer.h`
- [ ] 删除 `src/apps/shared/data-sources/SliceNumberLayer.cpp`
- [ ] 从 CMakeLists.txt 移除（如有引用）
- [ ] 确认无其他文件 include 或引用该类

### V.6 切片列表面板统一（D-19）

**目标**：将 `SlicerListPanel` 的功能合并到 `SliceListPanel`，使其成为所有页面的统一切片列表。

**方案**：

- [x] `SliceListPanel` 增加右键菜单（丢弃/恢复），复用 SlicerListPanel 的逻辑
- [x] `SliceListPanel` 增加 `setDiscarded()` / `discardedIndices()` API
- [x] `SliceListPanel` 增加进度条功能（已有 FileProgressTracker，确保所有页面启用）
- [x] `SliceListPanel` 增加时长信息显示（当前仅显示 sliceId，应加上时长）
- [x] Slicer 页面额外的切点编辑菜单项（添加切点/删除边界）通过子类或可选信号扩展
- [x] 迁移 DsSlicerPage 使用新的统一 SliceListPanel 替代 SlicerListPanel
- [x] 删除 `SlicerListPanel.h/.cpp`
- [ ] 验证：所有页面的切片列表功能一致
- [x] `SliceListPanel` 增加 `ensureSelection()` / `saveSelection()` / `validateAudioPath()` 公共 API
- [x] 三页面（MinLabel / Phoneme / Pitch）使用新 API 消除重复逻辑

### V.7 切片不一致弹窗（D-20）

**依赖**：V.6 完成后。

- [x] 页面切换守卫（`main.cpp`）增加切点数量 vs 项目 item 数量的一致性检查
- [x] 不一致时弹窗：提示文案 + 详情列出问题文件
- [x] 未导出 / 未切片 / 切点数不一致 / 切点位置已变化 四种状态检测
- [x] 用户确认后跳回 Slicer 页面
- [x] 修复跳转按钮不生效：使用 `clickedButton()` + `QTimer::singleShot` 延迟跳转
- [x] 验证：修改切点后切换到 MinLabel 页面触发提醒
- [ ] 复选框选择需要重新切片的音频（当前简化为全局提醒 + 详情展示）

### V.8 批量导出不更新项目 items + 导出模式合并（D-26）

**问题**：
1. `onBatchExportAll()` 导出所有音频的切片 WAV，但不注册 items 到 project，导致其他页面看不到新导出的切片
2. 当前 Slicer 页面有两个独立按钮（"导出…" 和 "批量导出全部"），体验割裂

**方案**：

- [x] `onBatchExportAll()` 导出后注册所有 items 到 project（同 `onExportAudio()` 逻辑），并 emit `sliceListChanged()`
- [x] 合并"导出…"按钮为统一入口，弹出选择对话框：
  - 导出当前（当前选中的音频文件的切片）
  - ~~导出选定（复选框选择要导出的音频文件）~~（简化为当前/全部二选一）
  - 导出全部（所有已切片的音频文件）
- [x] 导出后 project items 正确包含所有已导出切片（合并而非覆盖）
- [ ] 验证：导出全部后切换到歌词/音素页面，item 列表完整显示

---

## 待验证项（需人工测试）

以下为已实现功能的验证清单，需要人工运行程序确认：

| 来源 | 验证项 | 优先级 |
|------|--------|--------|
| Q.1 | CI `cmake --build build --target install` 后 deploy/ 目录完整可用 | P1 |
| Q.2 | 关闭重开工程后 slicer 状态完整恢复 | P0 |
| Q.3.1 | CLion 重新加载 CMake 验证 target 树结构 | P2 |
| Q.4 | 从 slicer 切换到歌词页再返回，文件列表不重复 | P0 |
| Q.5 | 标记丢弃后再次点击 Discard 可还原 | P1 |
| Q.6 | DsLabeler 和 LabelSuite 的 HubertFA 推理正常工作 | P0 |
| Q.7 | 新建工程 → 切片导出后 .dsproj、dstemp/ 均在工程文件夹内 | P0 |
| Q.8 | 两条边界完全重合时可选中后方边界并向右拖开 | P1 |
| Q.9 | FA 输出的边界位置正确分布；切换到未标注切片时不显示旧数据 | P0 |
| R.2 | 启动时不加载模型，首次 FA/F0/MIDI 时自动加载 | P1 |
| R.3 | 跨工程保持 splitter 比例和子图排列 | P2 |
| S.2 | radio button 切换时边界线贯穿范围正确变化 | P0 |
| S.5 | Slicer 单层全贯穿；PhonemeLabeler radio 切换正确 | P0 |
| S.6 | 拖动到极限位置时被正确约束 | P1 |
| S.9 | 两个页面功能无回归 | P0 |

---

## 执行顺序

```
Phase V（优先）:
  V.1 ──┐
  V.2 ──┤ 可并行，无依赖
  V.3 ──┤
  V.4 ──┤
  V.5 ──┤
  V.6 ──┤──→ V.7（依赖 V.6）
  V.8 ──┘    （可独立）

Phase T（V 完成后或并行）:
  T.1 ──┐
  T.2 ──┤ 可并行
  T.3 ──┘
```

**预估工作量**：

| 任务 | 复杂度 | 预估 |
|------|--------|------|
| V.1 视口比例尺重设计（7 steps） | 高 | 2-3 天 |
| V.2 边界拖动修复 | 中 | 2h |
| V.3 无数据提示 | 低 | 0.5h |
| V.4 最近工程标灰 | 低 | 1h |
| V.5 删除废弃组件 | 低 | 0.5h |
| V.6 列表面板统一 | 高 | 1-2 天（部分已完成：ensureSelection/validateAudioPath API）|
| V.7 切片不一致弹窗 | 中 | ~~0.5 天~~（已完成，待验证复选框细化）|
| V.8 导出合并 + items 更新 | 中 | 0.5 天 |
| T.1 CSV 预览 | 中 | 1 天 |
| T.2 快捷键 | 低 | 0.5 天 |
| T.3 子图顺序 UI | 中 | 1 天 |
| **合计** | | **5-7 天** |
