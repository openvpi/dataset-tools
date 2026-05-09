# 人工决策记录

> 本文档记录所有由用户明确给出的设计决策，供后续实施参考。
> 与其他设计文档冲突时，以本文档为准。
>
> 最后更新：2026-05-09（新增 P-13/P-14/P-15/P-16/P-17：吸收 C++ Core Guidelines、Qt API 设计原则、LLVM/KDE 大型项目设计准则 + 适配器隔离原则）

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

### P-10：统一路径库，禁止各处自行拼接

**原则**：基于 `std::filesystem` 设计跨平台路径库（`dsfw::PathUtils`），路径拼接、编码转换、输出到 debug 信息等均使用此库，禁止各处自行拼接路径或实现编码转换。

**理由**：
- 当前代码中路径拼接散落各处（`path / "config.json"`、`dir + "/" + filename` 等），风格不统一
- 编码转换有 3 处重复实现：`dstools::toFsPath()`（domain）、`dsfw::PathUtils::toStdPath()`（framework）、`DsDocument::toFsPath()`（domain），行为完全相同
- `path.string()` 在 Windows 上产生 ANSI 乱码，但仍有 20+ 处使用（BUG-04/05/06/31，PATH-01~08）
- 缺少 `path → UTF-8 string` 的统一方法，各处自行用 `u8string()` + `std::string(begin, end)` 转换

**实现要求**：
1. `dsfw::PathUtils` 新增 `join()`、`toUtf8()`、`toWide()` 方法
2. 废弃 `dstools::toFsPath()` 和 `DsDocument::toFsPath()`，统一使用 `dsfw::PathUtils::toStdPath()`
3. 所有 `path.string()` 替换为 `PathUtils::toUtf8(path)`（用于错误消息/日志）或 `PathUtils::toWide(path)`（用于 Windows API）
4. 所有手动路径拼接替换为 `PathUtils::join()` 或 `std::filesystem::path::operator/`

### P-11：多线程安全优先

**原则**：批处理和多页面设计必须考虑多线程竞争问题。共享可变状态必须通过锁（`std::mutex`）或原子操作（`std::atomic`）保护。

**理由**：
- 当前 `QtConcurrent::run` 中的推理调用与主线程的模型卸载存在竞争（P-09 仅是过渡方案）
- 批量处理（MinLabelPage/PhonemeLabelerPage/PitchLabelerPage/ExportPage）的进度回调和取消逻辑缺少统一保护
- 多页面同时活跃时，共享引擎的并发访问需要 mutex 保护（已部分实现于 GameModel/CancellableOnnxModel）

**实现要求**：
1. 所有推理引擎的 `process()` 方法必须受 `std::mutex` 保护（已部分实现）
2. 批量处理任务的取消标志必须使用 `std::atomic<bool>`
3. 异步回调中访问页面成员前必须检查存活令牌（P-09）
4. 新增共享资源时必须同步考虑线程安全策略

### P-12：相似模块统一设计

**原则**：相似模块使用相似的设计思路。大部分功能相同的模块尽量使用同一个类、不同实例按需开启部分功能，而非创建多个高度相似的类。

**理由**：
- SlicerPage 和 DsSlicerPage 约 60% 代码重复（TD-11）
- WaveformWidget/SpectrogramWidget/PowerWidget 各自实现 ~150 行拖拽逻辑（TD-05）
- MinLabelPage/PhonemeLabelerPage/PitchLabelerPage 的批量处理模式几乎相同但各自实现
- 修复 bug 时需要同改多处，维护成本高

**判断标准**：
- 两个类有 >60% 相同代码 → 合并为同一类 + 配置开关
- 两个类有 30%~60% 相同代码 → 提取共同基类，差异通过虚方法/配置实现
- 两个类有 <30% 相同代码 → 可独立实现，但接口风格应保持一致

### P-13：RAII 资源管理

**原则**：所有资源（内存、文件句柄、锁、推理引擎句柄）的生命周期必须绑定到拥有对象的作用域。使用 C++ 标准库的 RAII 包装（`std::unique_ptr`、`std::shared_ptr`、`std::lock_guard`、`std::scoped_lock`）管理资源。

**理由**：
- 手动 `new`/`delete` 容易导致内存泄漏和 double-free
- 异常安全需要 RAII 保证——异常抛出时栈展开自动释放资源
- 大型项目中资源所有权的追踪成本随代码量指数增长

**禁止**：
- 裸露 `new`/`delete`（应使用 `std::make_unique`/`std::make_shared`）
- 手动 `mutex.lock()`/`unlock()`（应使用 `std::lock_guard` 或 `std::scoped_lock`）
- 文件句柄不包装在 RAII 类型中（如 `std::ifstream` 裸对象非 RAII 场景需注意）
- 推理引擎裸指针在多线程间传递（P-09 是过渡方案，长期应改为 shared_ptr 所有权）

**来源**：吸收自 C++ Core Guidelines (R: Resource Management)、Qt API Design Principles、LLVM 编码规范。

### P-14：组合优于继承

**原则**：优先通过组合/委托复用功能，而非构建深的继承层次。接口继承（纯虚类）是好的设计，实现继承需谨慎——只有在"is-a"关系明确且基类行为无需大幅修改时才使用。

**理由**：
- 深层继承导致"脆弱的基类问题"——修改基类可能意外破坏所有子类
- 组合更灵活——可在运行时替换被组合的对象（策略模式）
- 多重继承（尤其是菱形继承）增加复杂性和歧义
- KDE Frameworks 明确建议在可组合优于继承的场景使用组合

**判断标准**：
- "is-a" 关系 + 行为可复用 → 公共基类（如 `EditorPageBase`）
- "has-a" 关系 + 行为复用 → 组合/委托（如 `BoundaryDragController` 被 widget 持有，而非 widget 继承自它）
- 仅需部分行为 → 策略模式 / 配置开关（如 P-12 的 Config 结构体）

**来源**：吸收自 C++ Core Guidelines (C: Classes and class hierarchies)、KDE Frameworks Policies、Qt API Design Principles。

### P-15：依赖倒置（面向接口编程）

**原则**：高层模块不应依赖低层模块，二者都应依赖抽象（纯虚接口）。具体实现通过 `ServiceLocator` 注入（全局服务）或构造函数注入（局部依赖）。

**理由**：
- 降低耦合——更换底层实现时高层代码无需修改
- 便于单元测试——可注入 mock 实现
- 与 P-06（接口稳定）互补：P-06 强调接口的价值，本原则强调依赖的方向

**实现要求**：
- 框架层定义纯虚接口（`IFileIOProvider`、`IModelProvider`、`IG2PProvider`、`IFormatAdapter`）
- 应用层提供具体实现，通过 `ServiceLocator` 或构造函数注入
- 页面/业务逻辑代码只依赖接口，不直接依赖具体类
- 新增功能优先考虑新增接口 + 实现，而非在现有类上打补丁

**来源**：吸收自 SOLID 原则（Dependency Inversion Principle）、C++ Core Guidelines (I: Interfaces)、LLVM 库分层设计。

### P-16：开闭原则（对扩展开放，对修改关闭）

**原则**：新增功能应通过新增类/模块/插件实现，而非修改已稳定的现有代码核心逻辑。

**理由**：
- 修改已稳定代码引入回归风险
- 新增适配器/插件不影响现有功能
- LLVM 的 Pass 注册机制、Qt 的插件系统都是此原则的经典实践

**实践方式**：
- 新文件格式 → 新增 `IFormatAdapter` 子类，注册到 `FormatAdapterRegistry`，不修改 registry 核心调度逻辑
- 新推理引擎 → 新增 `IModelProvider` 实现，不修改 `ModelManager` 核心流程
- 新页面 → 新增 Page 类实现 `IPageLifecycle`/`IPageActions`，不修改 `AppShell` 核心
- 新导出格式 → 新增 `IExportFormat` 实现

**来源**：吸收自 SOLID 原则（Open-Closed Principle）、Qt Plugin System、LLVM Pass Registry。

### P-17：内部文档模型 + 适配器隔离

**原则**：本项目维护统一的内部文档模型（`DsDocument`、`TextGridDocument` 等），所有具体文件格式（CSV、TextGrid、Lab、ds、json 等）**一律通过 IFormatAdapter 适配器**对接内部模型，**不得在业务代码中直接操作文件**。

**核心要求**：

1. **内部文档模型是唯一的真相源**：所有页面、处理流程、推理引擎只与内部模型交互（如 `DsDocument`、`TextGridDocument`），不感知底层文件格式。

2. **文件 I/O 必须经过适配器**：格式读取/写入通过 `IFormatAdapter` 子类实现，适配器负责格式 ↔ 内部模型的转换。适配器内部可使用 `IFileIOProvider` 进行底层文件读写，但业务代码不得绕过适配器直接调用 `IFileIOProvider` 读写格式文件。

3. **迁移模块必须重构为适配器模式**：从其他项目迁移进来的模块，如果其原生代码直接操作文件（如 `game-infer/` 的 DiffSingerParser 直接读文件），必须重构：提取出内部模型 + 适配器，业务代码只与内部模型交互。

4. **禁止模式**：
   - 业务代码中调用 `QFile`、`std::ifstream`、`IFileIOProvider` 直接读写格式文件
   - 页面加载切片时直接 `nlohmann::json::parse(file)` 然后手动塞入 UI 控件
   - 导出功能中自己拼 CSV 字符串写入文件

5. **正确模式**：
   ```
   业务代码                                适配器层                      文件系统
   ────────                              ────────                      ────────
   DsDocument.load(path)         →     DsFileAdapter.read()       →   IFileIOProvider
   TextGridDocument.loadFromDsText()  →  TextGridAdapter.read()  →   IFileIOProvider
   exportService.export(doc, fmt)     →  CsvAdapter.write()       →   IFileIOProvider
   ```

**理由**：
- 新增文件格式只需新增适配器，业务代码零改动（P-16 的具体实践）
- 内部模型可在不触及文件格式的情况下演进
- 便于单元测试——可注入 mock 适配器和 mock IFileIOProvider
- 文件 I/O 路径统一，便于统一错误处理、编码转换、日志记录

**来源**：吸收自 Qt Model/View 架构、Hexagonal Architecture（端口-适配器模式）、C++ Core Guidelines (A: Architectural Ideas)。

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

## D-39：状态栏信号连接生命周期由框架统一管理

**决策**：`createStatusBarContent()` 返回的 widget 由 AppShell 持有并在页面切换时销毁重建，其内部子 widget 的信号连接必须以子 widget 自身为 context 对象，确保 widget 销毁时连接自动断开。框架层提供 `StatusBarBuilder` 工具类统一管理连接创建，消除各页面手动处理连接生命周期的风险。

**问题根因**：
- `AppShell::rebuildStatusBar()` 在页面切换时对旧状态栏 widget 调用 `deleteLater()`
- 页面 `createStatusBarContent()` 中 `connect(sender, signal, this, [localWidgetPtr]...)` 使用 `this` 为 context，localWidget 被销毁后连接仍活跃
- `m_editor` 发出信号时 lambda 访问已销毁的 widget → 崩溃

**具体改造**：
1. `EditorPageBase` 新增 `StatusBarBuilder` 内部类，封装状态栏 widget 创建和信号连接
2. `StatusBarBuilder::connect()` 强制使用目标 widget 为 context 对象，从 API 层杜绝 context 不匹配
3. 各页面 `createStatusBarContent()` 改用 `StatusBarBuilder` 创建连接
4. `PhonemeLabelerPage` 已有的 `m_sliceLabelConn`/`m_posLabelConn` 手动管理方式废弃，统一迁移到 `StatusBarBuilder`

**影响文件**：
- `EditorPageBase.{h,cpp}` — 新增 `StatusBarBuilder` 嵌套类
- `PitchLabelerPage.cpp` — 迁移到 `StatusBarBuilder`
- `PhonemeLabelerPage.{h,cpp}` — 移除 `m_sliceLabelConn`/`m_posLabelConn`，迁移到 `StatusBarBuilder`
- `MinLabelPage.cpp` — 迁移到 `StatusBarBuilder`

---

## D-40：移除 TierLabelArea 后贯穿线定位修复

**决策**：`AudioVisualizerContainer::removeTierLabelArea()` 后，必须确保 `BoundaryOverlayWidget` 正确覆盖编辑区顶部位置，使 active tier 的贯穿线从编辑区顶部贯穿所有子图到底部。

**根因**：`removeTierLabelArea()` 将 `TierLabelArea` 从布局中移除并设置 `m_tierLabelTotalHeight = 0`、`m_tierLabelRowHeight = 0`。但 `BoundaryOverlayWidget` 的 `repositionOverSplitter()` 使用 `totalTopOffset = m_tierLabelTotalHeight + m_extraTopOffset` 计算偏移——在 tier label 被移除后 `totalTopOffset` 可能为 0，导致 overlay 未正确覆盖到编辑区顶部。

**具体修复**：
1. `AudioVisualizerContainer::removeTierLabelArea()` 在移除 label area 后，应设置 `m_extraTopOffset = m_tierEditWidget ? m_tierEditWidget->height() : 0`（即编辑区高度）
2. `BoundaryOverlayWidget::repositionOverSplitter()` 中，当 `m_tierLabelTotalHeight == 0` 时，确保 overlay 至少覆盖从编辑区顶部到底部的区域
3. `PhonemeEditor::buildLayout()` 在 `removeTierLabelArea()` 后，调用 `m_container->boundaryOverlay()->repositionOverSplitter()` 强制刷新

**影响范围**：
- Slicer 不受影响（不调用 removeTierLabelArea）
- PhonemeLabelerPage 的贯穿线显示恢复

---

## D-41：Chart widget 绘制边界线并正确响应拖拽

**决策**：三个 chart widget（WaveformWidget、PowerWidget、SpectrogramWidget）的 `paintEvent()` 中必须调用 `drawBoundaryOverlay()`，使贯穿线在 chart 区域内有实际像素绘制（而非仅由透明 overlay 绘制），从而让鼠标事件能正确触发 `hitTestBoundary()`。

**根因**：
- `BoundaryOverlayWidget` 设置 `WA_TransparentForMouseEvents`，鼠标事件穿透到 chart widget
- Chart widget 的 `paintEvent()` 未调用 `drawBoundaryOverlay()`，chart 自身无边界线
- `hitTestBoundary()` 依赖 `m_boundaryModel` 搜索边界——虽然理论上可通过 model 找到边界，但由于无视觉反馈，用户感知为"点击贯穿线但没有反应"
- 此外，`hitTestBoundary()` 可能在 active tier 索引无效（-1）或 `m_boundaryModel` 为 nullptr 时返回 -1，导致鼠标事件进入"滚动全图"分支

**具体修改**：
1. 在每个 chart widget 的 `paintEvent()` 末尾添加 `drawBoundaryOverlay(painter)`：
   ```cpp
   void WaveformWidget::paintEvent(QPaintEvent *) {
       // ... 现有绘制代码 ...
       if (m_boundaryOverlayEnabled && m_boundaryModel)
           drawBoundaryOverlay(painter);
   }
   ```
2. 将 `m_boundaryModel` 的设置从 `loadAudio()` 提前到 `buildLayout()` 或 widget 构造后
3. 在 `mousePressEvent` 中增加 fallback：当 `hitTestBoundary()` 返回 -1 时，扫描所有层级边界计算到点击位置的距离，若最近边界在扩展命中区内则选择它
4. 确保 D-28 原则贯彻实现——hit-test 搜索所有层级边界

**影响范围**：
- Chart widget 绘制性能增加极小开销（边界线条数通常 < 200）
- Slicer 的贯穿线行为不受影响（Slicer 使用 SliceBoundaryModel）
- PhonemeLabelerPage 和 LabelSuite 的 Phoneme 页面均受益

---

## D-42：FA 原生输出完整层级从属关系

**决策**：`buildFaLayers()` 必须输出每个 word 的完整边界（同时包含 start 和 end），并为 word.end ↔ phone[end].end 建立 BindingGroup。新增 `LayerDependency` 数据结构保存 word→phone 的完整映射。`applyFaResult()` 不再跳过 grapheme 层，而是保存 FA 对齐后的 grapheme 边界。

**根因**：
- `buildFaLayers()` 当前只输出 word.start 作为 grapheme 边界，缺少 word.end
- `applyFaResult()` 跳过 grapheme 层（`if (newLayer.name == "grapheme") continue;`），FA 对齐后的精确边界被丢弃
- BindingGroup 只在 word.start == phone[0].start 时建立，word.end 与最后一个 phone 无绑定
- 用户期望：FA 输出完整的层级依赖关系，grapheme 和 phoneme 的边界能绑定同步拖动

**具体修改**：
1. `buildFaLayers()` 为每个 word 输出两个 grapheme 边界（start 和 end）：
   ```cpp
   Boundary graphemeStart;
   graphemeStart.pos = secToUs(word.start);
   graphemeStart.text = QString::fromStdString(word.text);
   
   Boundary graphemeEnd;
   graphemeEnd.pos = secToUs(word.end);
   graphemeEnd.text.clear();  // end marker
   
   r.graphemeLayer.boundaries.push_back(graphemeStart);
   r.graphemeLayer.boundaries.push_back(graphemeEnd);
   
   // 绑定 word.start ↔ phone[0].start
   r.groups.push_back({graphemeStart.id, phone[0].id});
   // 绑定 word.end ↔ phone[n].end
   r.groups.push_back({graphemeEnd.id, phone[n].id});
   ```
2. 新增 `LayerDependency` 数据结构：
   ```cpp
   struct LayerDependency {
       QString parentLayer;       // "grapheme"
       QString childLayer;        // "phoneme"
       int parentStartBoundaryId;
       int parentEndBoundaryId;
       int childStartBoundaryId;
       int childEndBoundaryId;
       std::vector<int> childBoundaryIds;  // 该 word 的所有 phone ID
   };
   ```
3. `applyFaResult()` 直接替换 `grapheme` 层（而非创建 `fa_grapheme`），FA 结果包含 SP 词边界。同时清除已存在的 `fa_grapheme` 层（向后兼容旧数据）
4. `readFaInput()` 从 `grapheme` 层读取时过滤 SP/AP 文本，确保重跑 FA 时歌词输入正确
5. `DsTextDocument` 新增 `dependencies` 字段持久化层级关系

**影响范围**：
- grapheme 层直接被 FA 结果替换（含 SP 词边界），grapheme/phoneme 从属关系正确对齐
- 不再创建 `fa_grapheme` 层，消除重复字级层问题
- `readFaInput()` 过滤 SP/AP 文本，重跑 FA 时歌词输入不受 SP 影响
- LayerDependency 可用于跨层 clamp 的自动推导（D-17/D-29）
- TextGridDocument 的 loadFromDsText 需解析 dependencies 设置跨层规则

---

## D-43：PitchLabelerPage 添加工具栏 + 音频播放修复

**决策**：为 `PitchLabelerPage` 添加独立 `QToolBar`，包含提取 F0（RMVPE）、提取 MIDI（GAME）等按钮。同时修复 `onSliceSelectedImpl()` 中音频加载逻辑，确保有效音频路径能被无条件加载。

**根因**：
- 工具栏缺失：`PitchLabelerPage` 仅有 `createMenuBar()` 菜单栏，无 `QToolBar`
- 音频播放问题：`onSliceSelectedImpl()` 中 `loadAudio()` 受 `audioPath.isEmpty()` 条件保护，`validatedAudioPath()` 返回空时跳过加载

**具体修改**：
1. 在 `PitchLabelerPage` 中创建 `QToolBar`，包含：
   - 播放/暂停、停止按钮
   - 分隔符
   - 提取音高 F0 (RMVPE) 按钮
   - 提取 MIDI (GAME) 按钮
   - 分隔符
   - 保存按钮
   - Zoom In / Zoom Out
2. 工具栏按钮使用 `m_extractPitchAction` 和 `m_extractMidiAction` 等已有 action
3. 修复音频加载：
   ```cpp
   void PitchLabelerPage::onSliceSelectedImpl(const QString &sliceId) {
       const QString audioPath = source()->validatedAudioPath(sliceId);
       
       // 加载标注数据...
       
       if (!audioPath.isEmpty()) {
           double duration = 0.0;
           if (doc.audio.out > doc.audio.in)
               duration = usToSec(doc.audio.out - doc.audio.in);
           m_editor->loadAudio(audioPath, duration);
       }
   }
   ```
4. 使用 `QToolButton` + 资源 SVG 图标，风格与 `PhonemeLabelerPage` 保持一致

**影响范围**：
- 菜单栏 actions 不受影响（工具栏映射到同一 action，状态自动同步）
- ShortcutManager 快捷键继续有效
- PhonemeLabelerPage 的工具栏实现可作为参考模板

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
| 贯穿线定位 | D-37：removeTierLabelArea 后贯穿线未正确延伸到子图区域 | D-40：移除后设置 m_extraTopOffset 确保贯穿线覆盖编辑区 |
| Chart 边界绘制 | Chart widget 不绘制边界线，仅依赖透明 overlay | D-41：Chart widget paintEvent 调用 drawBoundaryOverlay |
| FA 层级输出 | buildFaLayers 只输出 word.start 边界，applyFaResult 跳过 grapheme | D-42：输出完整 start/end 边界 + LayerDependency 结构；applyFaResult 直接替换 grapheme 层（修订：不再创建 fa_grapheme） |
| PitchLabel 工具栏 | PitchLabelerPage 仅有菜单栏，无工具栏 | D-43：添加 QToolBar 含 F0/GAME 按钮 |

---

## D-44：fa_grapheme 层改名为 grapheme

**决策**：`buildFaLayers()` 创建的层名从 `"fa_grapheme"` 改为 `"grapheme"`。LayerDependency 中的 `parentLayerName` 同步改为 `"grapheme"`。

**理由**：
- 该层的来源是 FA 结果，但层名称不应带 `fa_` 前缀（用户指定）
- 当前其他地方已经统一使用 `"grapheme"` 作为 grapheme 层名称
- 所有筛选 `"fa_grapheme"` 的代码改为筛选 `"grapheme"`

**影响文件**：
- `PhonemeLabelerPage.cpp`：`buildFaLayers()` L57, L104；`onSliceSelectedImpl()` L236, L744；`applyFaResult()` L744 等

---

## D-45：HFA 结果使用 fill_small_gaps + add_SP 填充微小空隙

**决策**：FA 推理完成后，调用 `WordList::fill_small_gaps()` 和 `WordList::add_SP()` 填充词间微小空隙（参考 main 分支 hfa 代码），再调用 `buildFaLayers()` 构建层级数据。

**理由**：
- main 分支的 HFA 处理流程在导出 TextGrid 前会调用这两个方法，确保词间无微小间隙
- `fill_small_gaps()` 将词间 ≤0.1s 的空隙合并到相邻词中
- `add_SP()` 在剩余空隙处插入 SP（静音）词
- 这样产生的 grapheme 和 phoneme 层边界更完整

**实现要求**：
```cpp
// 在 runFaForSlice() / batch FA 中，recognize 成功后：
float wavLen = static_cast<float>(audioLengthSec);
words.fill_small_gaps(wavLen, 0.1f);
words.add_SP(wavLen, "SP");
auto faResult = buildFaLayers(words);
```

**影响文件**：
- `PhonemeLabelerPage.cpp`：`runFaForSlice()` 和 batch FA 的 lambda

---

## D-46：刻度条系统绑定 ViewportController resolution

**决策**：`TimeRulerWidget` 的刻度级别选择不再使用独立的 PPS 计算，改为直接使用 ViewportController 提供的 `resolution` 和 `sampleRate` 计算像素间距。刻度线只能和波形图等子图同步缩放——不允许其他子图单独缩放到极限后刻度仍可缩放。

**根因分析**：
- 当前 `findLevel()` 使用 `m_sampleRate / m_resolution` 计算 PPS（正确），但 `kLevels` 表的最小级别是 `{0.001, 0.0005}`（1ms/0.5ms），当 resolution=10 时 PPS=4410，0.5ms × 4410 = 2.2px < kMinMinorStepPx=4，所以会自动选到下一级别
- 实际上当前的行为在 ViewportController 的 resolution 范围内（10-44100）是正常的
- 真正的问题是：PianoRollView 使用自己的 `m_hScale` 独立管理缩放，没有正确受 ViewportController 约束

**具体修改**：
1. `TimeRulerWidget` 确认使用 `state.resolution` 和 `state.sampleRate` 计算 PPS（已正确实现），确保 `kMinMinorStepPx` 足够大（从 4 提升到 8 防止过密）
2. `PianoRollView` 的刻度（`PianoRollRenderer::drawRuler`）不再使用简单的 `interval` 判断（`s.hScale > 200 ? 0.5`），改为使用与 TimeRulerWidget 相同级别的查表法
3. 确保所有子图（Waveform/Spectrogram/Power）通过 ViewportController 统一缩放

**影响文件**：
- `TimeRulerWidget.cpp`：调整 `kMinMinorStepPx`
- `PianoRollRenderer.cpp`：`drawRuler()` 改为查表法
- `PianoRollView.cpp`：确认缩放通过 ViewportController

---

## D-47：PitchLabel 缩放限制最大长度

**决策**：`PianoRollView` 缩放到极限（zoomOut）时，视口范围不能超过音频总时长。即当 resolution 达到表中最大值（44100 spx）时，如果视口宽度对应的时长仍小于音频时长，则以音频时长为视口范围。

**根因**：当前 `PianoRollView::zoomOut()` 和 `resetZoom()` 调用 `m_viewport->zoomAt()`。ViewportController 的 resolution 最大为 44100 spx（≈1 px/s @44100Hz），对于 1200px 宽视口最多显示 20 分钟音频——对大多数切片足够了。但如果音频超过 20 分钟，缩放到极限后视口范围仍小于音频时长。

**实现要求**：
1. `PianoRollView::setAudioDuration()` 和 `resizeEvent()` 中，计算 `maxResolution = audioDuration * sampleRate / drawWidth`，确保视口至少能容纳全部音频
2. ViewportController 的 `zoomOut()` 在达到表末但仍未覆盖全部时长时，允许设置更大的 resolution（使用计算值而非查表值）
3. 或者：在 `clampAndEmit()` 中增加检查——如果 `endSec - startSec < totalDuration` 且 resolution 已到最大，则强制视口 = 全时长

**影响文件**：
- `PianoRollView.cpp`：`setAudioDuration()`, `resizeEvent()`
- `ViewportController.cpp`：`zoomOut()` 或 `clampAndEmit()`

---

## D-48：修复音高线 F0Curve timestep 单位转换导致不显示（BUG-33）

**决策**：`applyPitchResult()` 中 `float timestep`（秒）必须转换为 `TimePos`（微秒）再赋值给 `F0Curve::timestep`。

**根因**：
- `runPitchExtraction()` 计算 `float timestep = 0.005f`（5ms，秒为单位）
- `applyPitchResult()` 将 `float timestep` 直接赋值给 `pitchCurve->timestep`（`TimePos` = `int64_t` 微秒）→ 0.005f → 0（截断为整数）
- `onSliceSelectedImpl()` 加载时 `file->f0.timestep = curve.timestep`（此处正确，因 curve.timestep 已为 TimePos）
- `PianoRollRenderer::drawF0Curve()` 检查 `s.dsFile->f0.timestep <= 0` → true → 直接 return，不绘制

**修复**：
```cpp
// applyPitchResult() 中：
pitchCurve->timestep = secToUs(static_cast<double>(timestep));
m_currentFile->f0.timestep = secToUs(static_cast<double>(timestep));
```

**影响文件**：
- `PitchLabelerPage.cpp`：`applyPitchResult()` L761-762

---

## D-49：PitchLabel MIDI 优先 align 模式，无 ph_dur 时弹窗降级

**决策**：`PitchLabelerPage::onExtractMidi()` 和 `runMidiTranscription()` 优先使用 GAME 的 `align()` 方法（需要 ph_dur 输入）。如果当前切片的 phoneme 层不存在（即上一步 FA 未完成），弹窗提示用户选择："是否强制提取（使用非 align 模式）？"

**详细流程**：
1. 用户点击 "Extract MIDI"
2. 加载当前切片数据，查找 phoneme 层
3. 如果 phoneme 层存在：提取 phSeq, phDur, phNum，调用 `game->align(AlignInput{...}, options, notes)`
4. 如果 phoneme 层不存在：弹出 QMessageBox：
   - "当前切片无音素对齐数据（FA 未完成）。是否使用非对齐模式强制提取 MIDI？"
   - 按钮：Yes（强制提取，使用 getNotes()）/ No（取消）
5. `AlignOptions` 中的 `uvVocab` 从用户设置读取（见 D-50）

**影响文件**：
- `PitchLabelerPage.cpp`：`onExtractMidi()`, `runMidiTranscription()`
- 设置页面：新增 uvVocab 配置项（见 D-50）

---

## D-50：各步骤特殊关键字在设置页面体现

**决策**：在设置页面的各 tab 中增加对应步骤的特殊关键字配置项。

| Tab | 配置项 | 默认值 | 用途 |
|-----|--------|--------|------|
| 强制对齐 (FA) | Non-speech 音素 | `AP, SP`（逗号分隔） | FA `nonSpeechPh` 参数 |
| 音高/MIDI | 无声音素词汇 (uvVocab) | `AP, SP, br, sil`（逗号分隔） | GAME `AlignOptions.uvVocab` |
| 音高/MIDI | UV 词条件 | Lead / All / None | GAME `AlignOptions.uvWordCond` |
| 导出 | 导出格式选项 | （各格式参数） | 导出时使用的关键字/格式选项 |

**实现要求**：
1. `createFATab()` 增加 `m_faNonSpeechPh` QLineEdit
2. `createPitchTab()` 增加 `m_uvVocab` QLineEdit 和 `m_uvWordCond` QComboBox
3. `loadFromBackend()` 和 `applySettings()` 读写这些配置
4. 各页面从 settings 读取关键字而非硬编码

**影响文件**：
- `SettingsPage.h/cpp`：新增成员和 UI
- `PhonemeLabelerPage.cpp`：`runFaForSlice()` 从设置读取 nonSpeechPh
- `PitchLabelerPage.cpp`：`runMidiTranscription()` 从设置读取 uvVocab/uvWordCond

---

## D-51：分析修复各应用控件实时刷新问题

**决策**：对各应用页面进行逐页审查，检查在操作完成后控件是否实时刷新。

**检查清单**：
| 检查项 | 当前状态 | 需修复 |
|--------|---------|--------|
| 工具栏按钮状态（enabled/checked） | - | 待审查 |
| 状态栏标签更新（切片名/位置/缩放） | Pitch/Phoneme 已使用 StatusBarBuilder ✅ | - |
| Chart widget 重绘（数据变更后） | 部分页面未显式调用 update() | 待审查 |
| 切片列表进度条更新 | `updateProgress()` 已实现 ✅ | - |
| dirty 指示器更新 | `updateDirtyIndicator()` 已有 ❓ | 待验证 |
| 保存后 undo stack clean 状态 | `undoStack()->cleanChanged` 信号连接 ✅ | - |
| 缩放后 zoom 指示器更新 | - | 待审查 |
| 音符数量/状态更新 | `noteCountChanged` 信号 ✅ | - |
| A/B 比较状态刷新 | - | 待审查 |

**影响范围**：待审查结果确定

---

## D-52：整理归纳文档

**决策**：对 `docs/` 目录进行整理，删除过时的已完成内容，按以下结构重新分类：

```
docs/
├── README.md                        # 文档索引（新增）
├── human-decisions.md               # 人工决策记录（保留）
├── refactoring-plan.md              # 重构方案（保留）
├── design/                          # 设计文档
│   ├── framework-architecture.md
│   ├── task-processor-design.md
│   ├── unified-app-design.md
│   ├── ds-format.md
│   ├── log-and-i18n-design.md
│   └── test-design.md
├── guides/                          # 指南
│   ├── build.md
│   ├── framework-getting-started.md
│   ├── conventions-and-standards.md
│   └── migration-guide.md
├── reports/                         # 审计报告
│   ├── technical-summary.md
│   └── check/
│       ├── architecture-debt.md
│       ├── bugs-and-issues.md
│       ├── coding-standards-violations.md
│       └── technical-debt.md
```

**清理内容**：
- 删除重复/过时的已修复 bug 描述（已整合到 `bugs-and-issues.md` 中标记为"已修复"的可以保留）
- 将各文档头部的时间戳统一更新为 2026-05-09
- `refactoring-plan.md` 中已完成的旧任务压缩到折叠区域
