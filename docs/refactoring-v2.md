# 重构设计方案 v2

> 版本 2.2.0 — 2026-05-05
>
> 本文档记录已完成的架构设计和待实现的功能设计。
> 人工决策见 [human-decisions.md](human-decisions.md)，与本文档冲突时以人工决策为准。

---

## 1 AudioVisualizerContainer — 统一可视化基类（✅ 已完成）

### 1.1 设计目标

- Slicer、PhonemeLabeler 共享同一基类
- 支持动态添加/移除子图类型（波形图、Power 图、Mel 频谱图、未来的口型曲线图等）
- 子图排列顺序可在 Settings 页面自定义（持久化到用户配置）
- PitchLabeler 完全独立，保留钢琴卷帘窗架构
- **遵循 P-01/P-02 准则**：容器统一负责子组件刷新，消费者只需调用 `invalidateBoundaryModel()` 一个方法（见 roadmap VII.5）

### 1.2 垂直布局

```
┌───────────────────────────────────────────────────────────────┐
│ MiniMapScrollBar（迷你波形缩略图 + 滑窗）                       │  固定 ~30px
├───────────────────────────────────────────────────────────────┤
│ TimeRulerWidget（时间刻度尺）                                   │  固定 ~24px
├───────────────────────────────────────────────────────────────┤
│ ┌─ TierLabelArea ──────────────────────────────────────────┐ │
│ │ [●] phones  │ sh │ i │ g │ an │ n │ ian │ ...           │ │  标签区域
│ │ [○] words   │  shi │  guang │  nian │ ...               │ │  (带 radio button)
│ └──────────────────────────────────────────────────────────┘ │
├───────────────────────────────────────────────────────────────┤
│ ┌─ QSplitter (可拖拽调整比例) ─────────────────────────────┐ │
│ │ WaveformWidget    (dB 刻度左侧)                          │ │
│ │─────────────────────────────────────────────────────────│ │
│ │ PowerWidget       (可选)                                 │ │
│ │─────────────────────────────────────────────────────────│ │
│ │ SpectrogramWidget (Mel 频谱图)                           │ │
│ └─────────────────────────────────────────────────────────┘ │
├───────────────────────────────────────────────────────────────┤
│ BoundaryOverlayWidget（透明，覆盖标签区域 + QSplitter 区域）    │  叠加层
└───────────────────────────────────────────────────────────────┘
```

### 1.3 组件 API

```cpp
class AudioVisualizerContainer : public QWidget {
public:
    void addChart(const QString &id, QWidget *chart, int defaultOrder);
    void removeChart(const QString &id);
    void setTierLabelWidget(TierLabelArea *tierLabel);
    void setChartOrder(const QStringList &order);
    ViewportController *viewport() const;
    IBoundaryModel *boundaryModel() const;
    void setBoundaryModel(IBoundaryModel *model);
};
```

### 1.4 标签区域

- **TierLabelArea 基类**: `activeTierIndex()` / `activeBoundaries()` / `activeTierChanged` 信号
- **SliceTierLabel**（Slicer）: 单层自动编号，源于 `src/apps/shared/audio-visualizer/SliceTierLabel.h`
- **PhonemeTextGridTierLabel**（PhonemeLabeler）: 多层 + radio button，源于 `src/apps/shared/phoneme-editor/ui/`

### 1.5 键鼠交互

| 操作 | 行为 |
|------|------|
| 滚轮 | 横向滚动 |
| Ctrl+滚轮 | 横向缩放 |
| Shift+滚轮 | 波形图垂直振幅调整 |
| 右键单击任意子图 | 播放当前分割区域（红色游标，播放结束 200ms 后消失） |

---

## 2 配置架构（✅ 已完成）

- 所有配置存 `dsfw::AppSettings`（QSettings），不存 `.dsproj`
- `.dsproj` 仅保留工程数据（items、slicer、export）
- `ModelManager::ensureLoaded()` 懒加载
- splitter 比例 + 子图顺序跨工程持久化

---

## 3 待实现功能

### 3.1 DsLabeler CSV 预览（T.1）

ExportPage 内部 TabWidget："导出设置" | "预览数据"
- QTableView + QStandardItemModel（只读）
- 点击"预览数据" tab 时调用 CsvAdapter 实时生成
- 缺少前置数据的行标红

### 3.2 快捷键系统（T.2）

详见 [human-decisions.md](human-decisions.md) D-13。

各页面快捷键分配：

| 页面 | 快捷键 |
|------|--------|
| Slicer | S=自动切片, E=导出, I=导入切点, P=指针, K=切刀 |
| PhonemeLabeler | F=FA, ←/→=切换切片 |
| PitchLabeler | F=提取音高, M=MIDI, ←/→=切换切片 |
| MinLabel | R=ASR, ←/→=切换切片 |

### 3.3 子图顺序配置 UI（T.3）

Settings 页面增加"图表顺序"可拖拽排序列表。

---

## 4 已完成的重构阶段

> Phase 0–N, M.1–M.5, P.B, Q, R, S, U 全部已完成。详见 git 历史。

---

## 5 关键架构决策 (ADR)

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

## 关联文档

- [human-decisions.md](human-decisions.md) — 人工决策记录（最高优先级）
- [refactoring-roadmap-v2.md](refactoring-roadmap-v2.md) — 剩余任务清单
- [unified-app-design.md](unified-app-design.md) — 应用设计方案
- [architecture.md](architecture.md) — 架构概述
