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

## 待验证项（需人工测试）

以下为已实现功能的验证清单，需要人工运行程序确认：

| 来源 | 验证项 | 优先级 |
|------|--------|--------|
| Q.1 | CI `cmake --build build --target install` 后 deploy/ 目录完整可用 | P1 |
| Q.2 | 关闭重开工程后 slicer 状态完整恢复 | P0 |
| Q.3.1 | CLion 重新加载 CMake 验证 target 树结构 | P2 |
| Q.3.2 | `install(RUNTIME_DEPENDENCY_SET)` 可选验证 | P2 |
| Q.4 | 从 slicer 切换到歌词页再返回，文件列表不重复 | P0 |
| Q.5 | 标记丢弃后再次点击 Discard 可还原 | P1 |
| Q.6 | DsLabeler 和 LabelSuite 的 HubertFA 推理正常工作 | P0 |
| Q.7 | 新建工程 → 切片导出后 .dsproj、dstemp/ 均在工程文件夹内 | P0 |
| Q.8 | 两条边界完全重合时可选中后方边界并向右拖开 | P1 |
| Q.9 | FA 输出的边界位置正确分布；切换到未标注切片时不显示旧数据 | P0 |
| R.2 | 启动时不加载模型，首次 FA/F0/MIDI 时自动加载 | P1 |
| R.3 | 跨工程保持 splitter 比例和子图排列 | P2 |
| S.1 | 可动态添加/移除子图，顺序可调整 | P2 |
| S.2 | radio button 切换时边界线贯穿范围正确变化 | P0 |
| S.5 | Slicer 单层全贯穿；PhonemeLabeler radio 切换正确 | P0 |
| S.6 | 拖动到极限位置时被正确约束 | P1 |
| S.9 | 两个页面功能无回归 | P0 |

---

## 执行顺序

```
T.1、T.2、T.3 可并行，无依赖关系。
验证项可随时进行。
```

**预估工作量**：

| 任务 | 复杂度 | 预估 |
|------|--------|------|
| T.1 CSV 预览 | 中 | 1 天 |
| T.2 快捷键 | 低 | 0.5 天 |
| T.3 子图顺序 UI | 中 | 1 天 |
| 验证 | — | 0.5 天 |
| **合计** | | **3 天** |
