# 重构设计方案 v2

> 版本 2.1.0 — 2026-05-04
>
> 本文档整合所有未完成的设计需求，废弃过时内容。
> 人工决策见 [human-decisions.md](human-decisions.md)，与本文档冲突时以人工决策为准。

---

## 1 需解决的问题

### 1.1 Release 构建闪退（P0）✅ 已修复

**现象**：`LabelSuite.exe` 和 `DsLabeler.exe` 的 Release 构建启动即退出（0xC0000135 = STATUS_DLL_NOT_FOUND），缺少 cpp-pinyin、fftw3、sndfile 等 DLL。

**方案**：
1. 检查 `deploy.cmake` 的 install 链
2. 对 vcpkg SHARED 依赖添加 DLL 拷贝逻辑
3. 对 CLion cmake-build-release 添加 POST_BUILD 自定义命令

### 1.2 Slicer 音频列表和切点不持久化（P0）✅ 已修复

**现象**：DsLabeler 关闭重开工程后，Slicer 页面之前加载的音频文件列表和切点消失。

**根因**：`DsSlicerPage::m_fileSlicePoints` 仅存内存，未序列化到 `.dsproj`。

**方案**：在 `.dsproj` 增加 `slicer` 字段存储音频列表、切点、切片参数。

### 1.3 Slicer 页面切换弹窗导致文件列表重复（P0）✅ 已修复

**现象**：不存在 dsitem 或切点更新后，切换到其他页面的弹窗点"是"回到 slicer 后，文件列表多出一份重复的音频文件。

**根因**：
1. `main.cpp` 页面切换守卫中，无论点"是"或"否"都 `setCurrentPage(1)` 跳回 slicer，触发 `onActivated()`
2. `DsSlicerPage::onActivated()` 无条件调用 `m_audioFileList->addFiles(resolvedPaths)`，已有文件会被去重但已选中状态丢失；更重要的是弹窗不应触发 onActivated 的完整加载逻辑
3. 弹窗文案和行为不正确——应该只做"是否回到 slicer"的页面跳转确认

**方案**：
1. 弹窗改为 "未切片/切点信息更新，是否回到 Slicer？"，点"是"跳回 slicer、点"否"留在当前页
2. `onActivated()` 在音频列表已有内容时跳过 addFiles

### 1.4 文件列表 Discard 不支持还原（P1）✅ 已修复

**现象**：`DroppableFileListPanel` 的 Discard 按钮只能标记丢弃，无法还原。

**方案**：改为 toggle 逻辑，已丢弃的文件再次点击时还原。

### 1.5 HubertFA 推理错误（P0）✅ 已修复

**现象**：HubertFA 推理报错。

**根因**：
1. `HFA::recognize()` 内部硬编码读取同路径 `.lab` 文件获取歌词，但 DsLabeler 的歌词在 `TaskInput` 中、LabelSuite 的歌词在 dstext grapheme 层中，都不依赖磁盘 `.lab` 文件
2. `HubertAlignmentProcessor::process()` 和 `PhonemeLabelerPage::runFaForSlice()` 都错误地预填充了 WordList，但 recognize 会用 .lab 内容覆盖，预填充无意义且干扰后处理

**方案**：
1. `Hfa.cpp` `recognize()` 新增接受歌词文本参数的重载，将 .lab 读取逻辑从引擎内部移除
2. DsLabeler 适配层（`HubertAlignmentProcessor`）：从 TaskInput grapheme 层提取歌词文本传入
3. LabelSuite 适配层（`PhonemeLabelerPage`）：从 dstext grapheme 层提取歌词文本传入
4. 两个适配层均传入空 WordList 作为输出参数

### 1.6 工程文件保存到错误目录（P0）

**现象**：选择工程文件夹后，工程文件和 `dstemp/` 临时文件夹被创建到原始音频文件所在目录而非工程文件夹。

**可能根因**：
1. `DsProject::load()` 读取 `workingDirectory` 后未从 posix 路径转换回本地路径格式
2. 或 `ProjectDataSource::workingDir()` 返回的路径被后续代码错误解析

**方案**：排查 `workingDirectory` 在 load/save 路径的转换链，确保 `ProjectPaths` 方法始终基于正确的工程文件夹路径。

### 1.7 CMake 架构整理（P1）✅ 大部分已修复

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

## 2 AudioVisualizerContainer — 统一可视化基类（部分完成）

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
    virtual int activeTierIndex() const;
    void setActiveTierIndex(int index);
    // TODO: virtual QList<double> activeBoundaries() const = 0;
signals:
    void activeTierChanged(int index);
};
```

> **实现状态**：基类已实现，缺少 `activeBoundaries()` 方法。

#### SliceTierLabel（Slicer 用）— 未实现

- 单层，自动按顺序标数字
- 无 radio button
- `activeTierIndex()` 始终返回 0
- 当前 DsSlicerPage 使用独立的 `SliceNumberLayer` 组件，尚未迁移

#### PhonemeTextGridTierLabel（PhonemeLabeler 用）— 未实现

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

### 2.11 Slicer 具体配置（未迁移）

> **当前状态**：`DsSlicerPage` 仍使用手动布局，未迁移到 `AudioVisualizerContainer`。使用 `SliceNumberLayer`（独立组件）而非 `SliceTierLabel`（计划中的 TierLabelArea 子类）。

目标布局：
```
AudioVisualizerContainer
├── MiniMapScrollBar
├── TimeRulerWidget
├── SliceTierLabel（单层，自动编号）
├── QSplitter: WaveformWidget, SpectrogramWidget
└── BoundaryOverlayWidget（边界线贯穿所有图）
```

### 2.12 PhonemeLabeler 具体配置（未迁移）

> **当前状态**：`PhonemeLabelerPage` 未迁移到 `AudioVisualizerContainer`。

目标布局：

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

## 3 配置架构重设计 ✅ 已完成

### 3.1 配置统一到用户目录 ✅

所有配置存 `dsfw::AppSettings`（QSettings）。`ProjectSettingsBackend` 已废弃删除。

### 3.2 .dsproj 精简 ✅

仅保留工程数据：

```json
{
    "version": "3.1.0",
    "workingDirectory": "path/to/project/dir",
    "slicer": {
        "audioFiles": ["path/to/file1.wav"],
        "slicePoints": { "path/to/file1.wav": [1.234, 5.678] },
        "params": { "threshold": -40.0, "minLength": 5000, ... }
    },
    "export": { "hopSize": 512, "sampleRate": 44100, ... },
    "items": [...]
}
```

### 3.3 模型懒加载 ✅

`ModelManager::ensureLoaded()` 首次使用时加载，`invalidateModel()` 配置变更时 invalidate。

### 3.4 图表比例 + 子图顺序持久化 ✅

存 AppSettings，跨工程保持。`AudioVisualizerContainer` 内置 `saveSplitterState()` / `restoreSplitterState()`。

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
