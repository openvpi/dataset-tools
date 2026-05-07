# 人工决策记录

> 本文档记录所有由用户明确给出的设计决策，供后续实施参考。
> 与其他设计文档冲突时，以本文档为准。
>
> 最后更新：2026-05-07

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

### P-05：异常边界隔离

**原则**：应用层逻辑使用 `Result<T>` 传播错误，**禁止抛出异常**。`try-catch` 仅用于第三方库边界（nlohmann/json、ONNX Runtime、FFmpeg 等），将外部异常转为 `Result<T>::Error()`。每个 `catch` 块必须记录日志或返回错误，禁止静默吞掉。禁止在业务逻辑中使用 `try-catch` 做流程控制。

**来源**：吸收自 language-manager 项目的异常边界隔离原则。

### P-06：接口稳定

**原则**：公共头文件（`include/` 目录下的头文件）即契约。对外接口的变更需谨慎考虑向后兼容性。框架层（dsfw）的接口稳定性要求高于应用层。

### P-07：简洁可靠

**原则**：遇错直接返回，不设计重试或回滚（除有明确需求如 `BatchCheckpoint` 断点续处理外）。优先选择简单直接的方案，避免过度设计。

**来源**：吸收自 language-manager 项目的"Write Once, Run Forever"设计理念。

### P-08：每页独立资源，禁止全局共享（ServiceLocator 误用防护）

**原则**：ServiceLocator 仅用于**真正的全局服务**（如 `IFileIOProvider`、`IModelManager`、`IG2PProvider`）。页面级资源（如 `IAudioPlayer`、widget 实例）不得通过 ServiceLocator 全局共享。

**判断标准**：
- ✅ 全局服务：所有页面共享同一实例有意义（文件存储、模型注册表、G2P 字典）
- ❌ 页面资源：每个页面需要独立实例（音频播放器、widget 状态机）

**禁止模式**（`PlayWidget::initAudio()` 旧代码）：
```cpp
// ❌ 第一个 PlayWidget 注册全局 IAudioPlayer，后续所有 PlayWidget 被迫共享
if (auto *shared = ServiceLocator::get<IAudioPlayer>()) {
    m_player = shared;  // 复用别人的 player
    return;
}
ServiceLocator::set<IAudioPlayer>(m_player);
```

**正确做法**：每个 PlayWidget 创建自有的 AudioPlayer，不注册到 ServiceLocator。

### P-09：异步任务必须持有引擎存活令牌（防止 use-after-free）

**原则**：任何 `QtConcurrent::run` 中捕获到并在后台线程调用的原始指针对象，必须在页面中维护一个
`std::shared_ptr<std::atomic<bool>>` "存活令牌"，lambda 在使用引擎前必须先检查令牌。

**背景**：`ModelManager::invalidateModel()` 会销毁推理引擎，但后台线程可能仍持有原始指针副本。QPointer 只能保护 QObject 子类，不能保护原始指针。

**实现模式**：
```cpp
// 页面成员：
std::shared_ptr<std::atomic<bool>> m_engineAlive;

// 引擎就绪时：
m_engineAlive = std::make_shared<std::atomic<bool>>(true);

// lambda 中：
auto alive = m_engineAlive;  // shared_ptr 副本，延长令牌生命周期
QtConcurrent::run([hfa, alive, ...]() {
    if (!alive || !*alive) return;  // 引擎已销毁，安全退出
    hfa->recognize(...);            // 仍存在 TOCTOU 间隙，需进一步重构
});

// 引擎失效时（onDeactivated / onModelInvalidated）：
m_engineAlive.reset();  // 原子置 false 并释放引用
```

**注意**：此方案是**过渡性补救**，存在 TOCTOU（check-then-use）间隙。彻底解决需要引擎层自身提供 shared_ptr 所有权或线程安全的访问令牌。

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

## D-26：比例尺从固定列表改为连续范围 + 页面独立默认值与持久化

**决策**：

1. **废弃 PPS 概念，统一使用「比例尺」（resolution）**：全局统一用 `resolution`（整数，每像素采样数）作为唯一的缩放量。`pixelsPerSecond` 作为概念和 API 全部删除——所有 widget 内部存储的 `m_pixelsPerSecond` 改为 `m_resolution`，`ViewportState::pixelsPerSecond` 字段删除，替换为 `ViewportState::resolution`。时间↔像素转换统一通过 `resolution` 和 `sampleRate` 计算。

2. **去掉固定 resolution 列表**：当前 `kResolutionTable = {10, 20, 30, 40, ..., 400}` 写死了 11 个离散档位，max=400 对应 44100Hz 音频约 110s/1200px 视口——根本无法在视口内显示几分钟的音频。改为连续范围 `[resolutionMin, resolutionMax]`，**zoom 步进取整十**（对数步进后 round 到最近的整十数）。

3. **zoom 步进取整十的对数表**：
   ```
   10, 20, 30, 50, 80, 100, 150, 200, 300, 500, 800, 1000,
   1500, 2000, 3000, 5000, 8000, 10000, 15000, 20000, 30000, 44100
   ```
   每步约 ×1.3~1.7，覆盖从精细编辑到全局概览。zoomIn/zoomOut 在此表中前后移动一格。

4. **默认 resolution 按页面功能定义**：
   - **Slicer 默认**：视口显示约 **2 分钟**音频。`defaultResolution = sampleRate * 120 / viewportWidth`，对 44100Hz / 1200px ≈ 4410 → 取表中最近值 5000。
   - **Phoneme 默认**：视口显示约 **20 秒**音频。`defaultResolution = sampleRate * 20 / viewportWidth`，对 44100Hz / 1200px ≈ 735 → 取表中最近值 800。
   - `setDefaultResolution()` 传 resolution 值，`fitToWindow()` 使用该值初始化。

5. **放大/缩小极限**：
   - 表首项 `10`（放大极限，@44100Hz 每像素 0.23ms）
   - 表末项 `44100`（缩小极限，= 1 px/s @44100Hz，1200px 可显示 20 分钟）

6. **每页比例尺独立持久化**：每个 AudioVisualizerContainer 实例通过 `settingsAppName`（如 "Slicer"、"PhonemeEditor"）将当前 resolution 保存到 AppSettings，下次打开同一页面时恢复。不同页面互不影响。

7. **UI 显示**：比例尺指示器显示 `"{N} spx"`（samples per pixel）或 `"{time}/div"`，不再显示 PPS。

**废弃**：
- D-24 中 "Resolution 范围硬编码 [10, 400]" → 改为整十对数表 [10, 44100]
- `ViewportState::pixelsPerSecond` 字段 → 删除，改为 `resolution`
- `ViewportController::setPixelsPerSecond()` / `pixelsPerSecond()` → 删除
- 所有 widget 中的 `m_pixelsPerSecond` 成员 → 改为使用 `state.resolution` + `sampleRate`
- `PhonemeEditor::setPixelsPerSecond()` / `zoomChanged(double pixelsPerSecond)` → 删除或改签名

---

## D-27：Phoneme 标签区域层级边界绑定与吸附重新设计

**决策**：

### 现状分析（基于实际代码）

**架构问题 1：合成 ID 方案 + BindingGroup 数据结构**

`BoundaryBindingManager::findAlignedBoundaries()` 依赖 `TextGridDocument` 中预存的 `BindingGroup`，每个 group 是一组合成 ID（`tierIndex * 10000 + boundaryIndex + 1`）。这个方案有两个致命缺陷：

- **ID 随 index 变化失效**：边界插入/删除后 boundaryIndex 偏移，合成 ID 指向错误的边界。`autoDetectBindingGroups()` 在 `loadFromDsText()` 后调用一次，之后不再更新。
- **序列化负担**：`BindingGroup` 存储在 `DsTextDocument.groups` 中，需要在 `PhonemeLabelerPage::saveCurrentSlice()` 和 `applyFaResult()` 中同步维护。FA 结果 `buildFaLayers()` 也必须生成 groups。

**架构问题 2：边界拖动逻辑四处重复**

`startBoundaryDrag()`/`updateBoundaryDrag()`/`endBoundaryDrag()` 完全相同的逻辑存在于 **四个** widget 中：
- `WaveformWidget`（~90 行拖动代码）
- `SpectrogramWidget`（~90 行拖动代码）
- `PowerWidget`（~90 行拖动代码）
- `IntervalTierView`（~50 行拖动代码，略有不同）

每个 widget 都持有 `BoundaryBindingManager *m_bindingMgr`、`m_dragAligned`、`m_dragAlignedStartTimes` 等完全相同的成员变量。违反 P-01（模块职责单一，行为不得分散重复）。

**架构问题 3：snapToLowerTier 方向限制**

`TextGridDocument::snapToLowerTier()` 只搜索 `tierIndex < current`（只向 tier index 更小的层吸附），无法处理：
- 低层（如 phoneme）边界向高层（如 grapheme）吸附
- 同层吸附（不常见但合理）

**实际上不是问题的地方**

- `BindingGroup` 在 `DsTextDocument` 中作为序列化字段是合理的——FA 产出的关联关系确实需要持久化
- `clampBoundaryTime()` 的逻辑本身是正确的（D-22 已记录最右侧边界的独立 bug）
- binding enable/disable 的 toolbar toggle 是合理的 UX

### 新设计

**核心改变：拖动时实时搜索 partners，不依赖预存 group**

1. **废弃 `BoundaryBindingManager` 类**：其全部功能（findAlignedBoundaries + createLinkedMoveCommand）内联到一个新的 **`BoundaryDragController`** 类中。

2. **`BoundaryDragController`（新类）**：封装边界拖动的完整三阶段管线，作为 `AudioVisualizerContainer` 的组成部分。所有 widget 不再各自实现拖动逻辑——它们只做 hit-test 和鼠标事件转发，实际拖动行为由 controller 统一处理。

3. **Partners 搜索改为实时时间匹配**：拖动开始时，扫描所有其他层中时间位置在容差范围内的边界作为 partners。不依赖 `BindingGroup` 的合成 ID。

4. **Snap 改为搜索所有其他层**：释放时的 snap 不限 "lower tier"，搜索所有 `tierIndex != current` 的层。

5. **`DsTextDocument.groups` 保留但不参与运行时 binding**：FA 产出的 groups 仍然序列化到 dstext 文件中（作为关联元数据），但运行时 binding 完全由时间匹配驱动。`TextGridDocument::autoDetectBindingGroups()`、`setBindingGroups()`、`findGroupForBoundary()` 保留，但只用于序列化，不用于拖动。

```
BoundaryDragController
├── startDrag(tier, boundaryIndex, model)
│   → findPartners: 扫描所有层，时间容差内的边界
│   → 记录 [(tier, index, originalTime)]
│
├── updateDrag(proposedTime)
│   → clamp source → move source (preview)
│   → for each partner: clamp → move (preview)
│
├── endDrag(finalTime, undoStack)
│   → snap to nearest cross-tier boundary (pixel threshold)
│   → restore all to original positions
│   → push LinkedMoveBoundaryCommand
│
├── cancelDrag()
│   → restore all to original positions
│
├── setBindingEnabled(bool)
├── setSnapEnabled(bool)
├── setToleranceMs(double)
└── isDragging() const
```

**受影响的文件**：

| 文件 | 变更 |
|------|------|
| `BoundaryBindingManager.h/cpp` | **删除** |
| `BoundaryDragController.h/cpp` | **新建** |
| `WaveformWidget.h/cpp` | 删除 ~90 行拖动代码 + m_bindingMgr/m_dragAligned 等成员，改为调用 controller |
| `SpectrogramWidget.h/cpp` | 同上 |
| `PowerWidget.h/cpp` | 同上 |
| `IntervalTierView.h/cpp` | 删除 ~50 行拖动代码，改为调用 controller |
| `AudioVisualizerContainer.h/cpp` | 持有 `BoundaryDragController`，转发 widget 的鼠标事件 |
| `PhonemeEditor.h/cpp` | `m_bindingManager` → `m_dragController`（通过 container 访问） |
| `IBoundaryModel.h` | `snapToLowerTier()` → `snapToNearestBoundary()`（搜索所有层） |
| `TextGridDocument.h/cpp` | 实现新的 `snapToNearestBoundary()`；保留 groups 相关方法用于序列化 |
| `BoundaryCommands.h/cpp` | `LinkedMoveBoundaryCommand` 保持不变 |
| `PhonemeLabelerPage.cpp` | `buildFaLayers()` 中 groups 生成逻辑保留（用于序列化） |

**不变的部分**：
- `BindingGroup` 类型定义（DsTextTypes.h）
- `DsTextDocument.groups` 字段和序列化
- `BoundaryCommands`（MoveBoundaryCommand, LinkedMoveBoundaryCommand 等）
- `IBoundaryModel::clampBoundaryTime()` 逻辑
- toolbar 中 binding/snap toggle 的 UX

---

## D-28：Active tier 只控制子图边界线显示，不限制拖动

**决策**：
- `activeTierIndex` 仅决定哪个层级的边界线在波形图/频谱图等子图中**贯穿显示**
- 所有层级的边界线在标签区域、TierEditWidget 中始终可见且**均可拖动**
- 子图中的 hit-test 搜索**所有层级**的边界（不只 active tier），返回距离最近的边界
- 拖动某个层级的边界不需要先切换到该层级

**影响**：`hitTestBoundary()` 改为遍历所有 tier 的边界。返回 `(tierIndex, boundaryIndex)` pair。

---

## D-29：低层级边界不得越出高层级区间（跨层 clamp）

**决策**：
- D-17 的**具体实现要求**：phoneme 层的某个边界 B 位于 grapheme 层区间 [Gstart, Gend] 内时，B 的拖动范围进一步收紧为 `[max(sameLayerLeft, Gstart), min(sameLayerRight, Gend)]`
- 这个约束作用于 `clampBoundaryTime()` 的扩展，而非独立阶段——保持 clamp 的单一入口
- tier index 越小 = 层级越高。tier 0 = 最高层（如 grapheme），不受任何层约束
- 只约束**直接上层**（tier-1），不递归

---

## D-30：Slicer/Phoneme 统一视图组合系统（组件自由显隐+排序）

**决策**：Slicer 和 Phoneme 不是"共享同一基类"——而是**使用完全相同的视图 UI 实例**，仅通过配置选择性地显示/隐藏不同组件。所有图表组件统一注册到 `AudioVisualizerContainer`，由配置决定哪些可见、以什么顺序排列。

### 核心原则

1. **同一套 UI，不是两套 UI**：SlicerPage 和 PhonemeEditor 使用完全相同的 `AudioVisualizerContainer` 实例化方式。差异仅在于：哪些 chart/widget 被注册、哪些默认可见、哪些默认隐藏。

2. **所有组件平等注册**：所有支持的 chart 类型（Waveform、Spectrogram、Power、未来扩展的曲线图等）在 `AudioVisualizerContainer` 初始化时全部注册，但根据页面类型和用户配置选择性显示。

3. **组件可见性由配置驱动**：每个 chart widget 的 `setVisible(bool)` 由配置控制。配置源优先级：页面级默认 > 用户设置覆盖。

4. **组件顺序由配置驱动**：用户可在 Settings 页面通过拖拽列表自定义 chart 的显示顺序（持久化到用户配置目录，不存工程）。

### 组件矩阵

| 组件 | Slicer 默认 | Phoneme 默认 | 用户可配置 |
|------|:-----------:|:------------:|:----------:|
| MiniMapScrollBar | ✅ 显示 | ✅ 显示 | -（固定） |
| TimeRuler | ✅ 显示 | ✅ 显示 | -（固定） |
| TierLabelArea | ✅（SliceTierLabel） | ✅（PhonemeTextGridTierLabel） | -（固定） |
| **WaveformWidget** | ✅ 显示 | ✅ 显示 | ✅ 显隐+顺序 |
| **PowerWidget** | ❌ 隐藏 | ✅ 显示 | ✅ 显隐+顺序 |
| **SpectrogramWidget** | ✅ 显示 | ✅ 显示 | ✅ 显隐+顺序 |
| BoundaryOverlayWidget | ✅ 贯穿线 | ✅ 活跃层贯穿 | -（固定） |
| TierEditWidget | ❌ | ✅ 显示 | -（页面决定） |

> 注：`TierLabelArea` 实现不同（SliceTierLabel vs PhonemeTextGridTierLabel）是因为标签语义不同（单层编号 vs 多层 TextGrid），但位置和容器角色完全一致。这是合理差异，不作统一。

### Settings 页面设计

在 Settings 页面新增一个 **"视图布局"** 区域：

```
┌─────────────────────────────────────────────┐
│  视图布局                                    │
│                                             │
│  ☑ 波形图          [═══ 拖拽排序 ═══]       │
│  ☑ Power 图        [═══ 拖拽排序 ═══]       │
│  ☑ Mel 频谱图      [═══ 拖拽排序 ═══]       │
│                                             │
│  [恢复默认]                                 │
└─────────────────────────────────────────────┘
```

- 每个 chart 一行，包含：**Checkbox**（显隐控制）+ **Label**（chart 名称）+ 拖拽手柄（排序）
- 使用 `QListWidget` + 自定义 item widget（checkbox + drag handle）
- 排序结果即时保存到 `AppSettings`，通过 `chartOrderChanged` 信号通知所有容器
- "恢复默认"按钮清除用户覆盖，回到页面默认配置

### 持久化

| Key | 位置 | 格式 |
|-----|------|------|
| `ViewLayout/chartOrder` | AppSettings | `"waveform,spectrogram,power"` |
| `ViewLayout/chartVisible` | AppSettings | `"waveform,spectrogram"` (仅列出可见的) |
| `ViewLayout/chartSplitterState` | AppSettings（per-page） | QSplitter::saveState() base64 |

### 对现有代码的影响

1. **`AudioVisualizerContainer`**：增加 `setChartVisible(id, bool)`、`chartVisible(id)` API。`addChart()` 不再立即 `widget->show()`，而是根据配置决定可见性。

2. **`SlicerPage`**：移除参数面板在 container 上方的固定布局；slicer 专属参数面板移至页面级 toolbar 区域（已存在）。PowerWidget 默认添加但不显示。

3. **`PhonemeEditor`**：PowerWidget、SpectrogramWidget 的可见性不再由单独的 action toggle 控制——改为由统一配置系统控制。现有的 `m_actTogglePower` / `m_actToggleSpectrogram` 改为切换配置。

4. **Settings 页面**：新增 "视图布局" section，包含带 checkbox 的可拖拽列表。

### 与 D-14 的关系

D-14 提出了"共享 AudioVisualizerContainer 基类"的方向。本决策（D-30）在此基础上明确：
- 不是"共享基类"而是"**同一套 UI 实例化方式**"
- 组件显隐和顺序由配置系统统一管理，非硬编码
- 用户可通过 Settings 页面自由定制

**废弃**：D-14 中 "子图的顺序可在 Settings 页面中自定义" → 整合到 D-30 的具体设计中。

---

## D-31：每个 PlayWidget 拥有独立的 AudioPlayer（禁止 ServiceLocator 共享）

**决策**：废弃原先 ServiceLocator 全局共享 `IAudioPlayer` 的设计。每个 PlayWidget 实例创建自有的 `AudioPlayer`，不注册到 ServiceLocator。

**原因**：
1. ServiceLocator 单槽位机制导致第一个创建 PlayWidget 的页面"抢占"全局音频资源，后续页面被迫共享同一播放器
2. Slicer 页面播放音频后，Phoneme 页面无法独立播放——因为共享的 AudioPlayer 已被占用
3. 非 parent 的 PlayWidget 发生内存泄漏，其 AudioPlayer 永远不会释放，导致 AppShell 析构时访问野指针崩溃（0xc0000005）

**影响**：
- `PlayWidget::initAudio()` 移除 ServiceLocator 共享逻辑，始终创建自有 `AudioPlayer`
- `PlayWidget::uninitAudio()` 移除 ServiceLocator 注销逻辑
- **所有** PlayWidget 必须设置 QObject parent（防止泄漏）
- `AudioPlayer::position()` / `setPosition()` / `duration()` 等路由到 `AudioPlayback` 的线程安全方法

**文件**：`PlayWidget.cpp`、`AudioPlayer.cpp`、`PhonemeEditor.cpp`、`MinLabelEditor.cpp`、`DsSlicerPage.cpp`、`SlicerPage.cpp`

---

## D-32：ModelManager::invalidateModel 先发信号再卸载（防止 use-after-free）

**决策**：`ModelManager::invalidateModel()` 必须先 `emit modelInvalidated(taskKey)`，再 `unload(typeId)`。原始顺序（先 unload 再 emit）导致页面在收到信号时引擎已被销毁，无法保护已排队的后台任务。

**影响**：
- 页面 `onModelInvalidated` 槽在引擎销毁前执行，可设置取消标志、清空原始指针
- 各推理页面增加 `std::shared_ptr<std::atomic<bool>>` 存活令牌（见 P-09）
- PhonemeLabelerPage、PitchLabelerPage、MinLabelPage、ExportPage 需遵循 P-09 模式

**文件**：`ModelManager.cpp`、`PhonemeLabelerPage.{h,cpp}`

---

## D-33：PhonemeLabelerPage 状态栏 connect 生命周期管理

**决策**：`createStatusBarContent()` 每次调用创建的 widget 及其 signal-slot 连接，必须在下次调用前显式断开。使用 `QPointer` 守卫捕获的 widget 指针，防止 `deleteLater()` 后悬空指针被信号触发。

**原因**：`AppShell::rebuildStatusBar()` 在页面切换时 `deleteLater()` 删除所有状态栏 widget，但旧的 connect 仍在，导致 `positionChanged` 信号访问已删除的 QLabel → 0xc0000005 崩溃。

---

## D-34：AudioPlaybackManager 应用级音频仲裁

**决策**：引入 `AudioPlaybackManager`（QObject，由 AppShell 持有），作为音频播放的唯一仲裁者。所有 PlayWidget 在开始播放前必须向 Manager 请求许可，Manager 确保同一时刻只有一个 PlayWidget 在播放。

**设计约束**：
- 不使用 ServiceLocator（P-08），通过 AppShell 构造时注入
- PlayWidget 保持自有 AudioPlayer（D-31 不变），Manager 仅做许可仲裁
- Manager 不持有音频资源，不参与音频数据流
- 新播放请求自动停止当前播放者（无需用户确认）

---

## D-35：贯穿线区域鼠标光标反馈

**决策**：在子图 widget（Waveform/Spectrogram/Power）中，当 `hitTestBoundary()` 命中边界时，鼠标光标切换为 `Qt::SizeHorCursor`，离开边界区域时恢复默认光标。拖动过程中保持 `SizeHorCursor`。

**原因**：当前贯穿线可拖动但无视觉反馈，用户无法感知"此处可拖动"。

---

## D-36：日志持久化 FileLogSink

**决策**：新增 `FileLogSink`，将日志写入文件（`dsfw::AppPaths::logsDir()` / `dslabeler_YYYYMMDD.log`），自动 7 天轮转。在 AppShell 初始化时注册。

**设计约束**：
- FileLogSink 内部使用 `std::mutex` 保护文件写入
- 文件 I/O 使用 `std::ofstream`，路径通过 `dstools::toFsPath()` 转换
- 不引入新的第三方依赖
- 日志格式：`[HH:mm:ss.zzz] [LEVEL] [category] message`

---

---

## D-37：AudioVisualizerContainer 支持移除 TierLabelArea

**决策**：为 `AudioVisualizerContainer` 增加 `removeTierLabelArea()` 方法，允许 phoneme 页面移除 `PhonemeTextGridTierLabel` 层级标签区域。标签区域的高度回收给 `TimeRuler` 和 `TierEditWidget`/`chartSplitter` 使用。

**接口**：
```cpp
void AudioVisualizerContainer::removeTierLabelArea();
```

**影响**：
- `PhonemeEditor::buildLayout()` 在 `setupLayout` 完成后调用 `m_container->removeTierLabelArea()`
- `updateOverlayTopOffset()` 需要处理 `m_tierLabelArea == nullptr`
- `invalidateBoundaryModel()` 中的 `if (m_tierLabelArea) m_tierLabelArea->onModelDataChanged()` 保持 guard 不变
- Slicer 不受影响

**复核准则**：
- ✅ P-01：新增行为收敛在 `AudioVisualizerContainer`，不扩散
- ✅ D-15：标签区域设计不强制要求可见
- ✅ D-30：同一容器不同配置

---

## D-38：文件列表面板分层设计（按钮风格统一，不强行合并数据模型）

**决策**：保留两个面板的独立职责，但统一按钮布局和视觉风格：

- **`DroppableFileListPanel`**（框架层）：通用文件列表，拖放+筛选+按钮栏+进度
- **`SliceListPanel`**（应用层）：切片列表，数据源绑定+进度+脏状态

两者**不强行合并**——因为数据模型不同（原始文件 vs 项目切片）。但按钮栏风格统一、进度条组件复用。

**具体改造**：
1. `AudioFileListPanel` 按钮从 emoji/ASCII 改为 QIcon（使用资源 SVG 图标）
2. `SliceListPanel` 是否启用按钮栏取决于模式（editor/slicer），由参数控制
3. 引入 `FileListButtonBar` 可复用类，统一按钮样式

**影响文件**：
- `DroppableFileListPanel.{h,cpp}` — 添加 `setButtonBarEnabled(bool)`、`setButtonVisible(id, bool)`
- `AudioFileListPanel.h` — 使用新按钮图标
- `SliceListPanel.{h,cpp}` — 可选启用按钮栏

---

## ADR 冲突解决

| 冲突项 | 旧决策 | 新决策（以此为准） |
|--------|--------|-------------------|
| Settings 持久化 | ADR-69：DsLabeler 用 ProjectSettingsBackend（.dsproj） | D-01：全部用 AppSettings（用户目录） |
| Ctrl+滚轮行为 | 任务 17：Ctrl+滚轮 = 振幅缩放 | D-03：Ctrl+滚轮 = 横向缩放 |
| .dsproj defaults 字段 | unified-app-design.md §9：含模型/设备/导出配置 | D-01：defaults 字段废弃 |
| 可视化组件架构 | ADR-65：IBoundaryModel + phoneme-editor 组件复用 | D-14 + D-30：AudioVisualizerContainer 统一 UI + 组件自由显隐排序 |
| 标签区域位置 | 旧设计中 TierEditWidget 在频谱图下方 | D-15：标签区域在刻度线下方、所有子图上方 |
| 子图配置 | D-14：子图顺序可在 Settings 自定义 | D-30：显隐+顺序统一配置，checkbox 列表 + 拖拽排序 |
