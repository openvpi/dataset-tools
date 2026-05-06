# 人工决策记录

> 本文档记录所有由用户明确给出的设计决策，供后续实施参考。
> 与其他设计文档冲突时，以本文档为准。
>
> 最后更新：2026-05-06

---

## ⚠ 最高优先级设计准则

以下准则适用于所有新增和修改的代码，优先级高于任何具体决策。

### P-01：模块职责单一，行为不得分散重复

**原则**：相同的行为逻辑只允许存在于一个地方。如果一个操作需要在多个应用/页面中执行，必须下沉到共享基类或框架层，由该层统一实现和通知，各消费者通过接口/信号/虚方法接入。

**禁止**：
- 同一操作（如 "模型数据变化后刷新 UI"）在每个页面各写一份，行为略有不同
- Widget 内部手动调用 3-4 个兄弟 widget 的 `update()`——这属于容器的职责
- 每个页面各自拼凑 "保存 splitter → maybeSave → 关闭快捷键" 的生命周期序列

**正确做法**：
- 容器提供统一的 `invalidateXxx()` 方法和信号，一次调用刷新所有子组件
- 基类实现生命周期骨架（模板方法模式），子类只实现差异化的 hook
- 路径编码、文件 I/O 等平台差异封装在一处（PathCompat / PathUtils），所有模块调同一个函数

### P-02：被动数据接口 + 外部通知优于观察者膨胀

**原则**：纯数据接口（如 `IBoundaryModel`）保持简洁，不引入 QObject/信号。数据变更的通知由持有数据的容器负责，通过容器级方法（如 `AudioVisualizerContainer::invalidateBoundaryModel()`）统一分发。

**理由**：
- 避免让每个 model 类都继承 QObject（运行时开销 + moc 依赖）
- 通知逻辑集中在容器，调用者只需一行代码
- 新增 widget 时自动被容器刷新，无需修改 model

### P-03：阻塞操作一律异步化

**原则**：任何可能耗时超过 50ms 的操作（模型加载、推理运行、批量文件操作、大数据校验）禁止在主线程同步执行。

**实现要求**：
- 模型加载 → `QtConcurrent::run` + 信号通知就绪
- 批量推理/补全 → 后台线程 + `QMetaObject::invokeMethod` 回主线程更新进度
- 禁止使用 `QApplication::processEvents()` 作为 "防止卡死" 的手段——这是反模式
- 异步操作完成后必须检查上下文是否仍有效（sliceId 是否仍为当前切片、页面是否仍活跃）

### P-04：错误信息必须追溯到根因

**原则**：错误消息必须包含导致错误的实际原因，而非现象描述。每一层函数如果通过 out-parameter 或 Result 报告了错误，调用者必须在第一时间检查并传播，不得忽略后继续执行并报出无关的二次错误。

**反例**：`resample_to_vio` 返回错误消息 `msg`，但调用者忽略它继续读空 VIO，报 "0 samples"。
**正确做法**：检查 `msg` → 立即返回 `Err("Failed to resample: " + msg)`。

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

## D-18：重叠边界线的选择与拖动策略

**决策**：
- 当多条边界线在同一像素位置重叠时，鼠标点击/悬停优先选中**索引较大（靠后）的边界线**。
- 理由：重叠通常发生在自动对齐或导入数据后，用户意图是将后方的边界向右拖开。选中后方边界使其可以立即向右移动分离。
- **clamp 规则不变**：每条边界仍受 `(L + 1ms, R - 1ms)` 约束。当两条边界重合时，前方边界只能向左移动，后方边界只能向右移动——这正好是"分离重叠"的正确行为。
- **hitTest 优先级**：在 hit 范围内存在多个候选时，选择像素距离最近者；距离相同时选择索引较大者（tie-break to right）。
- **影响范围**：IntervalTierView、WaveformWidget、SpectrogramWidget、PowerWidget 四个组件的 `hitTestBoundary()` 方法统一采用此策略。

---

## D-16：子图默认排列顺序

**决策**：
- 垂直布局从上到下：MiniMapScrollBar → TimeRuler → 标签区域 → 波形图 → Power 图（可选）→ 口型曲线图（待开发）→ Mel 频谱图
- 用户可在 Settings 中自定义子图排列顺序

---

## D-19：所有页面统一使用全功能文件/切片列表

**决策**：
- 所有应用页面（Slicer、MinLabel、PhonemeLabeler、PitchLabeler）的左侧文件/切片列表**统一使用 Slicer 级别的全功能列表面板**
- 功能包括：添加文件/文件夹、丢弃/恢复、进度条、右键菜单
- 实现方式：将 `SlicerListPanel` 的功能合并进 `SliceListPanel`，后者作为统一底层；Slicer 的额外切点编辑功能（添加切点/删除边界）通过可选信号/菜单项扩展
- `AudioFileListPanel`（原始音频文件列表）仅在 Slicer 页面保留，其他页面使用切片列表

---

## D-20：切片不一致时弹窗提示回 Slicer

**决策**：
- 当用户从 Slicer 切换到其他页面（MinLabel/Phoneme/Pitch）时，如果检测到**现有 dsitem 的时长与 Slicer 当前切点计算出的时长不一致**（说明切点已修改但未重新导出），弹窗提示用户
- 弹窗内容："切片数据已过期（切点已修改），是否回到 Slicer 页面重新切片？"
- 点"是"→ 回到 Slicer 页面，并弹出复选框对话框让用户选择需要重新切片的音频文件（默认全选所有切点变化过的音频）
- 点"否"→ 留在当前页面，使用旧的 dsitem 数据

---

## D-21：Phoneme 页面无数据时显示"No label data"

**决策**：
- PhonemeLabeler 的 `PhonemeTextGridTierLabel` 在**没有任何层**（tierCount == 0）时，仍然显示标签区域，但内容为居中的灰色文字 "No label data"
- 不因 tierCount == 0 而将标签区域高度设为 0 或隐藏

---

## D-22：最右侧边界线自由拖动 + 边界修复

**决策**：
- TextGridDocument 中最右侧（最后一条）边界线的 clamp 约束应以**音频总时长**为右边界，而非 `tier->GetMaxTime()`
- 当前问题：`clampBoundaryTime` 使用 `tier->GetInterval(boundaryIndex).max_time` 作为 nextBoundary，但对最后一条边界线这等于该 interval 自身的 max_time（等于边界本身），导致 clamp 区间为 0，无法拖动
- 修复：最后一条边界（`boundaryIndex == count - 1`）的 nextBoundary 应使用 `tier->GetMaxTime()`（即文档总时长）
- 同时排查 SliceBoundaryModel 的 clampBoundaryTime 是否有类似问题

---

## D-23：最近工程列表标灰找不到的路径

**决策**：
- WelcomePage 的 `refreshRecentList()` 在构建列表时，对每个路径检查 `QFileInfo::exists()`
- 不存在的路径：item 文字设为灰色 + 删除线样式，双击时弹出 "工程文件不存在，是否从列表中移除？"
- 不自动移除（用户可能只是 U 盘未插入）

---

## D-24：视口统一比例尺系统

**决策**：
- 参考 vLabeler 的 `CanvasResolution` 模型，将 ViewportController 改为 **resolution（整数，每像素采样数）** 驱动
- `resolution` 取代 `pixelsPerSecond` 作为缩放状态的真相源；`pixelsPerSecond = sampleRate / resolution` 为派生量
- 默认 resolution = 40（与 vLabeler 一致），对 44100Hz 音频对应 PPS ≈ 1102
- Resolution 范围硬编码 [10, 400]，步进按对数表（10→15→20→30→40→60→80→100→150→200→300→400），不会出现浮点精度问题或"到极限后继续变化"的 bug
- 刻度线采用查表法（类似 vLabeler `Timescale.find()`）：预定义 major/minor 刻度级别表，找到 `minorSec * pps >= 60px` 的第一个级别。不再用 smoothStep 渐隐
- 每张图默认高度比例用 `heightWeight`（waveform=1.0, power=0.5, spectrogram=0.75），首次打开按比例初始化 splitter，用户拖动后持久化
- 打开新音频时自动 fitToWindow（resolution 自动计算为 totalSamples/widgetWidth）
- MiniMapScrollBar wheelEvent 统一为：无 modifier = 横向滚动，Ctrl = 缩放
- 旧 `zoomAt(center, factor)` API 替换为 `zoomIn(centerSec)` / `zoomOut(centerSec)`

---

## D-25：删除已废弃的 SliceNumberLayer

**决策**：删除 `src/apps/shared/data-sources/SliceNumberLayer.h/.cpp`，该组件已被 `SliceTierLabel` 完全取代。

---

## ADR 冲突解决

| 冲突项 | 旧决策 | 新决策（以此为准） |
|--------|--------|-------------------|
| Settings 持久化 | ADR-69：DsLabeler 用 ProjectSettingsBackend（.dsproj） | D-01：全部用 AppSettings（用户目录） |
| Ctrl+滚轮行为 | 任务 17：Ctrl+滚轮 = 振幅缩放 | D-03：Ctrl+滚轮 = 横向缩放 |
| .dsproj defaults 字段 | unified-app-design.md §9：含模型/设备/导出配置 | D-01：defaults 字段废弃 |
| 可视化组件架构 | ADR-65：IBoundaryModel + phoneme-editor 组件复用 | D-14：AudioVisualizerContainer 统一基类 |
| 标签区域位置 | 旧设计中 TierEditWidget 在频谱图下方 | D-15：标签区域在刻度线下方、所有子图上方 |
