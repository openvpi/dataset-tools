# 重构设计方案 v2

> 版本 2.1.0 — 2026-05-04
>
> 本文档整合所有未完成的设计需求，废弃过时内容。
> 人工决策见 [human-decisions.md](human-decisions.md)，与本文档冲突时以人工决策为准。

---

## 1 需解决的问题

### 1.1 Release 构建闪退（P0）

**现象**：`LabelSuite.exe` 和 `DsLabeler.exe` 的 Release 构建启动即退出（0xC0000135 = STATUS_DLL_NOT_FOUND），缺少 cpp-pinyin、fftw3、sndfile 等 DLL。

**方案**：
1. 检查 `deploy.cmake` 的 install 链
2. 对 vcpkg SHARED 依赖添加 DLL 拷贝逻辑
3. 对 CLion cmake-build-release 添加 POST_BUILD 自定义命令

### 1.2 Slicer 音频列表和切点不持久化（P0）

**现象**：DsLabeler 关闭重开工程后，Slicer 页面之前加载的音频文件列表和切点消失。

**根因**：`DsSlicerPage::m_fileSlicePoints` 仅存内存，未序列化到 `.dsproj`。

**方案**：在 `.dsproj` 增加 `slicer` 字段存储音频列表、切点、切片参数。

### 1.3 CMake 架构整理（P1）

**问题**：
1. IDE 中部分 targets 显示在根目录而非分组 folder
2. `deploy.cmake` 的 install 逻辑将不必要的中间文件（.lib/.a 静态库）纳入 deploy 包
3. CI 的 `build.yml` 和 `release.yml` vcpkg 设置方式不统一（手动 commit vs `lukka/run-vcpkg`）
4. DsLabeler CMakeLists 有拼写错误 `WIN32_EXECETABLE`

**方案**：
1. 修正所有 `CMAKE_FOLDER` 设置
2. 分离 install 为 DEPLOY 模式（仅 exe+DLL+资源）和 SDK 模式（含 headers+.lib+cmake config）
3. 统一 CI vcpkg 版本管理
4. 可选：使用 CMake 3.21+ `install(RUNTIME_DEPENDENCY_SET)` 自动收集依赖
5. 可选：CI 步骤抽取为 reusable workflow

---

## 2 AudioVisualizerContainer — 统一可视化基类

### 2.1 设计目标

- Slicer、PhonemeLabeler 共享同一基类
- 支持动态添加/移除子图类型（波形图、Power 图、Mel 频谱图、未来的口型曲线图等）
- 子图排列顺序可在 Settings 页面自定义（持久化到用户配置）
- PitchLabeler 完全独立，保留钢琴卷帘窗架构

### 2.2 垂直布局

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
│ │ [口型曲线图]       (待开发，可选)                          │ │
│ │─────────────────────────────────────────────────────────│ │
│ │ SpectrogramWidget (Mel 频谱图)                           │ │
│ └─────────────────────────────────────────────────────────┘ │
├───────────────────────────────────────────────────────────────┤
│ BoundaryOverlayWidget（透明，覆盖标签区域 + QSplitter 区域）    │  叠加层
└───────────────────────────────────────────────────────────────┘
```

### 2.3 组件层次

```cpp
class AudioVisualizerContainer : public QWidget {
public:
    // 注册子图类型
    void addChart(const QString &id, QWidget *chart, int defaultOrder);
    void removeChart(const QString &id);
    
    // 标签区域配置
    void setTierLabelWidget(TierLabelArea *tierLabel);
    
    // 子图顺序（持久化到 AppSettings）
    void setChartOrder(const QStringList &order);
    QStringList chartOrder() const;
    
    // 共享状态
    ViewportController *viewport() const;
    IBoundaryModel *boundaryModel() const;
    void setBoundaryModel(IBoundaryModel *model);
    
protected:
    MiniMapScrollBar *m_miniMap;
    TimeRulerWidget *m_timeRuler;
    TierLabelArea *m_tierLabel;
    QSplitter *m_chartSplitter;
    BoundaryOverlayWidget *m_overlay;
    ViewportController *m_viewport;
};
```

### 2.4 标签区域

#### TierLabelArea 基类

```cpp
class TierLabelArea : public QWidget {
public:
    virtual int activeTierIndex() const = 0;
    virtual QList<double> activeBoundaries() const = 0;
signals:
    void activeTierChanged(int index);
};
```

#### SliceTierLabel（Slicer 用）

- 单层，自动按顺序标数字
- 无 radio button
- `activeTierIndex()` 始终返回 0

#### PhonemeTextGridTierLabel（PhonemeLabeler 用）

- 多层 TextGrid 编辑
- 最左侧竖排 radio button 组（每层一个）
- 选中层的边界线向下贯穿所有子图
- 非选中层的边界线仅在标签区域内，从该层向下延伸到最低层截止

### 2.5 层级包含规则与拖动约束

- **层级包含**：高层级（如 words）的边界必须与低层级（如 phones）的某个边界重合。低层级细分高层级区间。
- **拖动约束**：拖动边界线时，不允许超越同层级内相邻边界线。即边界 B 被限制在左邻 L 和右邻 R 之间：`L < B < R`。
- Slicer 切割线同理。

### 2.6 边界线贯穿规则

```
BoundaryOverlayWidget 绘制逻辑：

1. 获取 activeTierIndex
2. 对每条边界线：
   a. 属于选中层级 → 从该层顶部画到最底部子图底部（贯穿全部）
   b. 属于非选中层级 → 从该层顶部画到标签区域最底层底部（仅标签区域内）
3. 播放游标（红色）：始终贯穿标签区域 + 所有子图
```

### 2.6 MiniMapScrollBar

```
┌─────────────────────────────────────────────────────────────┐
│ ░░░░░░░░░░░░░█████████████████░░░░░░░░░░░░░░░░░░░░░░░░░░░░ │
└─────────────────────────────────────────────────────────────┘
  ↑ 完整音频 peak envelope   ↑ 滑窗矩形（当前视口）
```

- 拖拽矩形 = 滚动，拖拽边缘 = 缩放
- 高度固定 ~30px

### 2.7 刻度线重设计

参考 ds-editor-lite `ITimelinePainter`，适配时间轴：

1. `secondsPerPixel = visibleDuration / viewWidth`
2. 层级：1h / 10min / 1min / 10s / 1s / 100ms / 10ms / 1ms
3. `spacingVisibility(spacing, minimumSpacing=24)` — 渐隐
4. smoothStep opacity 渐变

所有子图共享 ViewportController，不允许独立横向缩放。

### 2.8 键鼠交互

| 操作 | 行为 |
|------|------|
| 滚轮 | 横向滚动 |
| Ctrl+滚轮 | 横向缩放 |
| Shift+滚轮 | 波形图垂直振幅调整 |
| 右键单击任意子图 | 播放当前分割区域（红色游标，播放结束消失） |
| Escape | 停止播放 |

### 2.9 y 轴刻度

- 波形图：左侧 dB 参考刻度（0/-6/-12/-24dB），无横向虚线
- 其他子图：无 y 轴刻度

### 2.10 子图顺序自定义

Settings 页面提供配置，存 AppSettings：

```ini
[AudioVisualizer]
chartOrder=waveform,power,spectrogram
```

子图 id：`waveform`、`power`、`spectrogram`、`viseme`（口型，待开发）

### 2.11 Slicer 具体配置

```
AudioVisualizerContainer
├── MiniMapScrollBar
├── TimeRulerWidget
├── SliceTierLabel（单层，自动编号）
├── QSplitter: WaveformWidget, SpectrogramWidget
└── BoundaryOverlayWidget（边界线贯穿所有图）
```

### 2.12 PhonemeLabeler 具体配置

```
AudioVisualizerContainer
├── MiniMapScrollBar
├── TimeRulerWidget
├── PhonemeTextGridTierLabel（多层 + radio button）
├── QSplitter: WaveformWidget, PowerWidget, SpectrogramWidget
└── BoundaryOverlayWidget（选中层贯穿，非选中层仅标签区域内）
```

### 2.13 PitchLabeler

完全独立，不使用 AudioVisualizerContainer。保留现有钢琴卷帘窗架构。

---

## 3 配置架构重设计

### 3.1 配置统一到用户目录

所有配置存 `dsfw::AppSettings`（QSettings）。

### 3.2 .dsproj 精简

仅保留工程数据：

```json
{
    "version": "3.0.0",
    "name": "My Dataset",
    "languages": [...],
    "slicer": {
        "audioFiles": ["path/to/file1.wav"],
        "slicePoints": { "path/to/file1.wav": [1.234, 5.678] },
        "params": { "threshold": -40.0, "minLength": 5000, ... }
    },
    "items": [...],
    "completedSteps": [...]
}
```

### 3.3 模型懒加载

首次使用时加载，配置变更时 invalidate。

### 3.4 图表比例 + 子图顺序持久化

存 AppSettings，跨工程保持。

---

## 4 DsLabeler CSV 预览

ExportPage 内部 TabWidget："导出设置" | "预览数据"

- QTableView + QStandardItemModel（只读）
- 点击"预览数据" tab 时调用 CsvAdapter 实时生成
- 缺少前置数据的行标红

---

## 5 快捷键系统

### 5.1 快捷键分配

#### 全局

| 快捷键 | 功能 |
|--------|------|
| Ctrl+Z | 撤销 |
| Ctrl+Shift+Z | 重做 |
| Ctrl+S | 保存 |
| Escape | 停止播放 |
| Space | 播放/暂停 |

#### Slicer

| 快捷键 | 功能 |
|--------|------|
| S | 自动切片 |
| E | 导出切片音频 |
| I | 导入切点 |
| Delete | 删除选中切割线 |
| P | 指针工具 |
| K | 切刀工具 |

#### PhonemeLabeler

| 快捷键 | 功能 |
|--------|------|
| F | 强制对齐当前切片 |
| ← / → | 上一个/下一个切片 |

#### PitchLabeler

| 快捷键 | 功能 |
|--------|------|
| F | 提取音高 |
| M | 提取 MIDI |
| ← / → | 上一个/下一个切片 |

#### MinLabel

| 快捷键 | 功能 |
|--------|------|
| R | ASR 识别 |
| ← / → | 上一个/下一个切片 |

### 5.2 菜单集成

所有快捷键通过 `QAction::setShortcut()` 在菜单中显示，工具按钮 tooltip 标注。

---

## 6 已完成的重构阶段

> Phase 0–N, M.1–M.5, P.B 全部已完成。详见 git 历史。

### Phase P — LabelSuite ↔ DsLabeler 高度统一（✅ 已完成）

- [x] P.B.1 ISettingsBackend + SettingsWidget 共享
- [x] P.B.2 FileDataSource 改造为 dstext 内核
- [x] P.B.3 核心页面统一（MinLabel/Phoneme/Pitch 提取为共享）
- [x] P.B.4 LabelSuite 独有页面保留并升级
- [x] P.B.5 LabelSuite 自动补全
- [x] P.B.6 旧代码清理
- [x] P.B.7 集成测试（30 单元测试通过）

---

## 7 关键架构决策 (ADR)

| ADR | 决策 | 理由 | 状态 |
|-----|------|------|------|
| 8 | 单仓库模式 | 降低维护复杂度 | 有效 |
| 43 | int64 微秒时间精度 | 消除浮点累积误差 | 有效 |
| 46 | LabelSuite + DsLabeler 两个 exe | 通用标注不绑定 DiffSinger | 有效 |
| 52 | IEditorDataSource 抽象数据源 | 页面代码不感知数据来源 | 有效 |
| 57 | 层依赖 DAG + per-slice dirty 标记 | 保证数据一致性 | 有效 |
| 62 | 右键直接播放，不弹菜单 | 标注中最频繁操作应零延迟 | 有效 |
| 65 | IBoundaryModel 解耦可视化组件 | 切片/音素共享 WaveformWidget/SpectrogramWidget | 被 D-14 扩展（AudioVisualizerContainer） |
| 66 | LabelSuite 底层统一 dstext | 消除数据模型分裂 | 有效 |
| 69 | ISettingsBackend 抽象持久化 | Settings UI 共享 | 被 D-01 修改（废弃 ProjectSettingsBackend） |
| 70 | FileDataSource 内部 dstext + FormatAdapter | 旧格式自动转入/转出 | 有效 |
| 71 | LabelSuite 保留全部独立页面 + 自动补全 | 手动控制仍可用 | 有效 |

---

## 8 文档清理计划

| 文档 | 处理 |
|------|------|
| `build.md` + `local-build.md` | **合并为一个** `build.md` |
| `unified-app-design.md` | 更新 §9（配置）和 §11（可视化架构） |
| `ds-format.md` | 更新 §2 去掉 defaults 字段 |

---

## 关联文档

- [human-decisions.md](human-decisions.md) — 人工决策记录（最高优先级）
- [refactoring-roadmap-v2.md](refactoring-roadmap-v2.md) — 路线图与任务清单
- [unified-app-design.md](unified-app-design.md) — 应用设计方案
- [architecture.md](architecture.md) — 架构概述
