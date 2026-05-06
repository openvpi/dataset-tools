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

## 待修复

### B-02 Phoneme 非活跃层边界贯穿线显示

**问题**：未选中的层级的边界线在下方图表（waveform/spectrogram/power）中也显示为贯穿线。

**分析**：代码审查显示 `BoundaryOverlayWidget` 的逻辑正确（非活跃层 `lineBottom = tiers * tierRowH` 止于 tierLabel 区域内）。chart widget 自身 `paintEvent` 不绘制边界线。需要实际运行验证是否为渲染时序或 geometry 问题。

### B-03 Phoneme 波形图不显示

**问题**：PhonemeLabeler 页面加载切片后波形图没有显示。

**分析**：代码逻辑正确（`setAudioData` + `rebuildMinMaxCache` + deferred `fitToWindow`）。可能原因：(1) 保存的 splitter state 将 waveform 高度压为 0；(2) `fitToWindow` 延迟执行时序问题。需要清除 QSettings 中的 `Layout/editorSplitterState` 验证。

### 6.3 快捷键系统（部分完成）

| 页面 | 规划快捷键 | 实际状态 |
|------|-----------|---------|
| Slicer | S=自动切片, E=导出, I=导入切点, P=指针, K=切刀 | ⚠️ 只有 V=指针, C=切刀，缺少 S/E/I |
| PhonemeLabeler | F=FA, ←/→=切换切片 | ✅ |
| PitchLabeler | F=提取音高, M=MIDI, ←/→=切换切片 | ✅ |
| MinLabel | R=ASR, ←/→=切换切片 | ✅ |

### 7.7 Slicer/Phoneme 界面行为不统一（部分完成）

- ✅ playhead 连接已统一到 `setPlayWidget()`
- ✅ splitter 状态延迟恢复已实现
- ⬜ dragController 连接可进一步统一，减少 SlicerPage 组装代码

---

## 已完成任务记录

| 编号 | 任务 | 完成方式 |
|------|------|---------|
| 1.1 | TierEditWidget 位置调整 | `setEditorWidget()` 插入 chartSplitter 之前 |
| 1.2 | Slicer 同步缩放 | `connectViewportToWidget()` 自动连接 |
| 1.3 | PhonemeEditor 波形显示 | splitter 延迟恢复 |
| 1.4 | 视图菜单联动 | `visibleStateChanged` 信号 |
| 2.1 | resolution 比例尺重构 | ViewportState 统一 resolution + sampleRate |
| 2.2 | TimeRuler 查表法 | `kLevels[]` 14 档 + `findLevel()` |
| 2.3 | wheelEvent 统一 | Ctrl+wheel=zoom, plain=scroll |
| 2.4 | BoundaryDragController | 统一拖动逻辑，删除 BoundaryBindingManager |
| 3.1 | 最右边界拖动 | `GetMaxTime()` 作为上界 |
| 3.2 | 无数据提示 | "No label data" + `setFixedHeight` |
| 3.3 | 最近工程灰色标记 | gray + strikeOut |
| 3.4 | SliceNumberLayer 删除 | 文件已删除 |
| 3.5 | 音频有效性管理 | `audioExists()` / `validatedAudioPath()` / 信号 |
| 3.6 | audioSource 路径 fallback | wavs/ 启发式 + qWarning |
| 3.7 | dsproj 数据模型修复 | 去重 / erase defaults / 版本检查 |
| 4.1 | EditorPageBase 合并 | 三页面继承，公共生命周期 |
| 4.2 | 旧实现清理 | 无残留 |
| 5.1 | 刷新框架 | `invalidateBoundaryModel()` |
| 5.2 | FA 异步化 | `ensureHfaEngineAsync` + QtConcurrent |
| 5.3 | 推理非阻塞 | ExportPage `continueExport()` 异步 |
| 6.1 | PathCompat | `openSndfile()` / `openIfstream()` |
| 6.2 | QTableView 主题 | dark.qss + light.qss 完整样式 |
| 6.4 | 图表顺序 UI | SettingsPage QListWidget + 上下按钮 |
| 6.5 | CSV 预览 | ExportPage QTabWidget "预览数据" |
| 7.1 | TimeRuler 刻度 | `syncStateFields()` 在 `clampAndEmit()` 前 |
| 7.2 | 播放游标 | `setPlayWidget()` 统一 playhead 连接 |
| 7.3 | 跨层 clamp | D-29 已移除（拖动期间位置失效导致不可用） |
| 7.4 | 跨层 hitTest | D-28，搜索所有层级 |
| 7.5 | splitter 恢复 | 延迟到 resizeEvent |
| 7.6 | 波形图缺失 | 同 7.5 |
| B-01 | 绑定边界拖动 | 移除跨层 clamp；binding 使用 word.start 保证时间对齐 |
| B-04 | Slicer 默认比例尺 | 默认 3000 + fitToWindow 尊重已保存值 + onActivated restoreResolution |
| B-05 | FA SP 过滤 | 不再过滤 SP/AP，完整保留 FA 输出 |

---

## 关联文档

- [human-decisions.md](human-decisions.md) — 人工决策记录（最高优先级）
- [unified-app-design.md](unified-app-design.md) — 应用设计方案
- [framework-architecture.md](framework-architecture.md) — 项目架构
- [conventions-and-standards.md](conventions-and-standards.md) — 编码规范
