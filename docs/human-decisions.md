# 人工决策记录

> 本文档记录所有由用户明确给出的设计决策，供后续实施参考。
> 与其他设计文档冲突时，以本文档为准。
>
> 最后更新：2026-05-04

---

## D-01：配置全部移出工程文件

**决策**：模型配置、设备选择、UI 偏好（图表比例、图表顺序等）**全部**存储在用户目录下（如 `~/.config/dataset-tools/` 或 `%APPDATA%/dataset-tools/`），不存储在 `.dsproj` 工程文件中。

**影响**：
- `.dsproj` 的 `defaults` 字段废弃，仅保留工程数据（items、languages、completedSteps、slicer）
- DsLabeler 的 Settings 页和 LabelSuite 的 Settings 页统一使用 `dsfw::AppSettings`（QSettings）
- `ISettingsBackend` 接口简化——不再需要 `ProjectSettingsBackend`
- 换工程后保留用户原先的模型配置和 UI 体验

**废弃文档**：unified-app-design.md §9 中 `.dsproj` 存储配置的设计

---

## D-02：模型懒加载

**决策**：所有推理模型（ASR、HuBERT-FA、RMVPE、GAME）改为懒加载——首次用到时再按 Settings 页配置加载，而非应用启动时加载。

**理由**：减少启动时间，避免加载未使用的模型浪费显存。

---

## D-03：Ctrl+滚轮 = 横向缩放，Shift+滚轮 = 波形幅度

**决策**：
- **Ctrl+滚轮**：横向缩放音频（所有波形图/频谱图/编辑区统一）
- **Shift+滚轮**：调整波形图垂直振幅（仅波形图生效）
- **滚轮（无修饰键）**：横向滚动

**废弃**：phoneme-editor-improvements.md 任务 17 中 "Ctrl+滚轮改为垂直振幅" 的描述

---

## D-04：顶部缩略图滚动条取代底部滚动条

**决策**：
- 移除所有波形编辑界面底部的 QScrollBar
- 最顶部放置**迷你波形缩略图 + 滑窗矩形**（MiniMapScrollBar）
- 缩略图显示完整音频的迷你波形
- 滑窗矩形指示当前视口范围，可拖动和缩放
- 移除原来顶部的 PlayWidget 音频播放工具栏

---

## D-05：分割线纵向贯穿规则

**决策**：
- **选中层级**的边界线：从标签区域**向下贯穿**所有子图（波形图、power 图、mel 频谱图等）
- **非选中层级**的边界线：仅在标签区域内显示，每个层级的边界线向下延伸到最低层级截止，不贯穿到标签区域以外的图
- Slicer 只有单层标签（自动编号），边界线始终贯穿所有图

**实现方向**：BoundaryOverlayWidget 覆盖标签区域 + 所有子图区域，根据选中层级决定绘制范围。

---

## D-06：右键播放当前分割区域 + 红色播放游标

**决策**：
- 在任意波形图或频谱图右键 → 直接播放点击位置所在的边界分割区域
- 播放时显示红色竖线游标，贯穿所有图
- 播放完毕后游标消失（200ms 超时自动清除）
- 不弹出右键菜单

---

## D-07：记录各图手动调整后的比例

**决策**：用户手动拖动 splitter 调整各图比例后，比例值持久化到用户配置目录（不存工程），下次打开任何工程仍保持。

---

## D-08：刻度线重新设计

**决策**：
- 参考 ds-editor-lite 的 `ITimelinePainter` 刻度算法（基于 `minimumSpacing` + `spacingVisibility` 渐显渐隐）
- 刻度线只能和波形图等同步缩放——不允许其他子图单独缩放到极限后刻度仍可缩放
- 所有子图共享同一个 ViewportController，刻度统一绑定

---

## D-09：y 轴仅保留 dB 刻度，去掉横向虚线

**决策**：
- 波形图左侧保留 dB 参考刻度（0dB / -6dB / -12dB / -24dB）
- 不绘制横向贯通的虚线网格
- 频谱图、功率图不需要 y 轴刻度

---

## D-10：DsLabeler CSV 预览页

**决策**：在 DsLabeler 导出页增加"预览数据" tab（TabWidget 的一个标签页），以只读表格展示输出的 transcriptions.csv 内容。

---

## D-11：Slicer 音频列表和切点需要持久化

**决策**：DsLabeler 的 Slicer 页面通过文件夹按钮加载的音频文件列表及每个文件的切点信息，必须保存到 `.dsproj`，关闭重开工程后自动恢复。

**根因**：当前 `DsSlicerPage::m_fileSlicePoints`（`std::map<QString, std::vector<double>>`）仅存在内存中，未序列化到工程文件。

---

## D-12：Release 构建依赖修复 + CMake 架构整理

**决策**：
- 修复 release 构建闪退（退出码 0xC0000135 = 找不到 DLL）
- CMake IDE 中非 exe 和非 test-exe 的 targets 必须有合理的 FOLDER 分组，不能出现在 IDE 根目录

---

## D-13：快捷键系统

**决策**：
- 每个页面的重要工具按钮分配快捷键（Slicer: S=自动切片, E=导出; PhonemeLabeler: F=FA; 等）
- 每个应用的菜单中展示各自的快捷键
- 文件列表上的 +/- 等按钮**不需要**快捷键
- 所有快捷键需在菜单和工具栏 tooltip 中可见

---

## D-14：AudioVisualizerContainer 统一可视化基类

**决策**：
- Slicer 和 PhonemeLabeler 共享一个 `AudioVisualizerContainer` 基类
- 基类设计成兼容扩展更多图类型（口型曲线图等待开发的图）
- 基类统一管理：MiniMapScrollBar → TimeRuler → 标签区域 → 子图 QSplitter
- 子图的**顺序可在 Settings 页面中自定义**（持久化到用户配置）
- PitchLabeler **完全独立**，保留自己的钢琴卷帘窗架构

---

## D-15：标签区域设计

**决策**：
- 标签区域位于**刻度线下方、波形图上方**（即在所有子图的上方）
- **Slicer**：单层标签，自动按顺序标数字，边界线向下贯穿所有图
- **PhonemeLabeler**：多层标签（TextGrid 层），标签区域**最左侧**有一组 **radio button** 用于选择当前活跃层级
  - 选中层级的边界线**向下贯穿**所有子图
  - 非选中层级的边界线仅在标签区域内显示，每个层级的边界线向下延伸到最低层级截止
- 标签区域是 AudioVisualizerContainer 的固定组成部分

---

## D-17：标签层级包含规则与拖动约束

**决策**：
- **层级包含规则**：PhonemeLabeler 中高层级（如 words）的边界强制对齐低层级（如 phones）的边界——即高层级的每个区间边界必须与某个低层级的边界重合。低层级的边界细分高层级的区间。
- **拖动约束**：拖动任何边界线/分割线时，不允许超越同层级内相邻的边界线。即边界 B 被夹在左邻边界 L 和右邻边界 R 之间时，拖动 B 的范围被限制在 (L, R) 内。
- Slicer 同理：切割线不允许拖过相邻切割线。

---

## D-16：子图默认排列顺序

**决策**：
- 垂直布局从上到下：MiniMapScrollBar → TimeRuler → 标签区域 → 波形图 → Power 图（可选）→ 口型曲线图（待开发）→ Mel 频谱图
- 用户可在 Settings 中自定义子图排列顺序

---

## ADR 冲突解决

| 冲突项 | 旧决策 | 新决策（以此为准） |
|--------|--------|-------------------|
| Settings 持久化 | ADR-69：DsLabeler 用 ProjectSettingsBackend（.dsproj） | D-01：全部用 AppSettings（用户目录） |
| Ctrl+滚轮行为 | 任务 17：Ctrl+滚轮 = 振幅缩放 | D-03：Ctrl+滚轮 = 横向缩放 |
| .dsproj defaults 字段 | unified-app-design.md §9：含模型/设备/导出配置 | D-01：defaults 字段废弃 |
| 可视化组件架构 | ADR-65：IBoundaryModel + phoneme-editor 组件复用 | D-14：AudioVisualizerContainer 统一基类 |
| 标签区域位置 | 旧设计中 TierEditWidget 在频谱图下方 | D-15：标签区域在刻度线下方、所有子图上方 |
