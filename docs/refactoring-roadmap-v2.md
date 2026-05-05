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

**状态**：ExportPage.h 已添加 QTabWidget/QTableView/QStandardItemModel 成员声明（scaffolding），实现未完成。

- [ ] ExportPage 构建 TabWidget："导出设置" | "预览数据"
- [ ] "预览数据" tab：点击时调用 CsvAdapter 生成内存 CSV
- [ ] QTableView + QStandardItemModel 只读展示
- [ ] 缺少前置数据的行标红（grapheme 层缺失时）

### T.2 快捷键系统

**状态**：DsSlicerPage 菜单已添加快捷键（S=自动切片, Ctrl+O=打开文件等）。其他页面未实现。

- [x] DsSlicerPage 菜单快捷键
- [ ] PhonemeLabelerPage 快捷键（F=强制对齐, ←/→=切换切片）
- [ ] PitchLabelerPage 快捷键（F=提取音高, M=提取MIDI, ←/→=切换切片）
- [ ] MinLabelPage 快捷键（R=ASR 识别, ←/→=切换切片）
- [ ] 工具按钮 tooltip 标注快捷键
- [ ] AppShell 页面切换时激活/停用对应快捷键组

### T.3 Settings 页子图顺序配置 UI

- [ ] Settings 页增加"图表顺序"配置项
- [ ] 可拖拽排序的列表（waveform / power / spectrogram / viseme 等）
- [ ] 变更后实时更新 AudioVisualizerContainer

---

## Phase V — Bug 修复与 UX 改进

依赖：无，可立即执行。优先级高于 Phase T。

### V.1 ViewportController 缩放极限硬停（D-24）

**问题**：Ctrl+滚轮在波形图已缩放到极限后继续操作，刻度线还能无限缩放/消失。ViewportController 虽然 clamp 了 PPS，但 `zoomAt()` 仍修改 view range。

**修复**：

- [ ] `ViewportController::zoomAt()`：clamp 后检查 `newPPS == m_state.pixelsPerSecond` 则直接 return
- [ ] `ViewportController::setPixelsPerSecond()`：同理，clamp 后无变化则 return
- [ ] 验证：Ctrl+滚轮在极限处不再产生任何视口变化

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

- [ ] `SliceListPanel` 增加右键菜单（丢弃/恢复），复用 SlicerListPanel 的逻辑
- [ ] `SliceListPanel` 增加 `setDiscarded()` / `discardedIndices()` API
- [ ] `SliceListPanel` 增加进度条功能（已有 FileProgressTracker，确保所有页面启用）
- [ ] `SliceListPanel` 增加时长信息显示（当前仅显示 sliceId，应加上时长）
- [ ] Slicer 页面额外的切点编辑菜单项（添加切点/删除边界）通过子类或可选信号扩展
- [ ] 迁移 DsSlicerPage 使用新的统一 SliceListPanel 替代 SlicerListPanel
- [ ] 删除 `SlicerListPanel.h/.cpp`
- [ ] 验证：所有页面的切片列表功能一致

### V.7 切片不一致弹窗（D-20）

**依赖**：V.6 完成后。

- [ ] 页面切换守卫（`main.cpp`）增加 dsitem 时长 vs 当前切点时长的一致性检查
- [ ] 不一致时弹窗：提示文案 + 复选框选择需要重新切片的音频
- [ ] 复选框默认全选所有切点变化过的音频
- [ ] 用户确认后跳回 Slicer 页面
- [ ] 验证：修改切点后切换到 MinLabel 页面触发提醒

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

Phase T（V 完成后或并行）:
  T.1 ──┐
  T.2 ──┤ 可并行
  T.3 ──┘
```

**预估工作量**：

| 任务 | 复杂度 | 预估 |
|------|--------|------|
| V.1 缩放极限 | 低 | 0.5h |
| V.2 边界拖动修复 | 中 | 2h |
| V.3 无数据提示 | 低 | 0.5h |
| V.4 最近工程标灰 | 低 | 1h |
| V.5 删除废弃组件 | 低 | 0.5h |
| V.6 列表面板统一 | 高 | 1-2 天 |
| V.7 切片不一致弹窗 | 中 | 0.5 天 |
| T.1 CSV 预览 | 中 | 1 天 |
| T.2 快捷键 | 低 | 0.5 天 |
| T.3 子图顺序 UI | 中 | 1 天 |
| **合计** | | **5-7 天** |
