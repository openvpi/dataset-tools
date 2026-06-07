# 设计准则与决策

> 本文档是项目唯一的权威设计准则与人工决策来源。
> 合并自 `docs/decisions/principles.md`、原 `docs/human-decisions.md`，按领域分类整理为完整体系。
> 与任何其他设计文档冲突时，以本文档为准。
>
> 最后更新：2026-06-08（重构阶段一完成：消除 33 个头文件的 using namespace dsfw 命名空间污染；阶段二完成：更新 audio-library-design.md 文档状态）

---

## 速查表

| 编号        | 领域 | 摘要                                                  | 原文    |
|-----------|----|-----------------------------------------------------|-------|
| ARCH-01   | 架构 | 模块职责单一，相同行为只存在一处                                    | P-01  |
| ARCH-02   | 架构 | 被动数据接口+容器通知，接口不加 QObject                            | P-02  |
| ARCH-03   | 架构 | 公共头文件即契约，接口稳定性高于应用层                                 | P-06  |
| ARCH-04   | 架构 | 相似模块 >60% 相同→合并类；30-60%→提取基类                        | P-12  |
| ARCH-05   | 架构 | 组合/委托优先于实现继承                                        | P-14  |
| ARCH-06   | 架构 | 高层依赖抽象接口，构造函数注入优于 ServiceLocator                    | P-15  |
| ARCH-07   | 架构 | 新增功能通过新增类实现，不修改核心逻辑                                 | P-16  |
| ARCH-08   | 架构 | 内部文档模型 + IFormatAdapter 适配器隔离所有文件格式                 | P-17  |
| ARCH-09   | 架构 | 标签区域位于刻度线下方、所有子图上方                                  | D-15  |
| ARCH-10   | 架构 | 所有页面统一使用 SliceListPanel 全功能列表                       | D-19  |
| ARCH-11   | 架构 | Slicer/Phoneme 同一套 UI 实例，组件显隐+排序可配                  | D-30  |
| CONCUR-01 | 并发 | >50ms 操作禁止主线程同步，禁止 processEvents                    | P-03  |
| CONCUR-02 | 并发 | QtConcurrent::run 中原始指针须持 shared_ptr\<atomic\> 存活令牌 | P-09  |
| CONCUR-03 | 并发 | 共享可变状态必须锁或原子操作保护                                    | P-11  |
| ROBUST-01 | 健壮 | 错误消息追溯到根因，不得忽略后报二次错误                                | P-04  |
| ROBUST-02 | 健壮 | Result\<T\> 传播错误，try-catch 仅限第三方边界                  | P-05  |
| ROBUST-03 | 健壮 | 遇错直接返回，不设计重试/回滚                                     | P-07  |
| INFRA-01  | 基础 | ServiceLocator 仅用于全局服务，页面资源不得全局共享                   | P-08  |
| INFRA-02  | 基础 | 接口必要性原则：新增接口至少两个预期实现或明确测试隔离需求                       | 新增    |
| INFRA-03  | 基础 | 实现优先原则：先具体类再抽取接口，接口随需求演进                            | 新增    |
| INFRA-04  | 基础 | 简化全局服务访问：instance() 直接访问优于 ServiceLocator           | 新增    |
| INFRA-05  | 基础 | std::filesystem 统一路径库，禁止各处自行拼接                      | P-10  |
| INFRA-06  | 基础 | RAII 资源管理，禁止裸 new/delete 和手动 lock/unlock            | P-13  |
| INFRA-07  | 基础 | 接口可 mock，构造函数注入优于 ServiceLocator                    | P-18  |
| INFRA-08  | 基础 | SettingsKey\<T\> 强类型，集中定义                           | P-19  |
| INFRA-09  | 基础 | 配置全部存用户目录，不存 .dsproj 工程文件                           | D-01  |
| INFRA-10  | 基础 | 推理模型懒加载，首次用到时按配置加载                                  | D-02  |
| INFRA-11  | 基础 | 音频切点持久化到 .dsproj，关闭重开自动恢复                           | D-11  |
| INFRA-12  | 基础 | 快捷键系统，菜单/tooltip 中可见                                | D-13  |
| INFRA-13  | 基础 | PIMPL 隔离第三方头文件，防止编译期泄漏                              | NC-01 |
| INFRA-14  | 基础 | 接口版本化，静态声明 kInterfaceVersion                        | NC-08 |
| INFRA-15  | 基础 | 信号签名规范：大对象 const ref，小类型值传递                         | NC-09 |
| INFRA-16  | 基础 | 流水线执行期间使用不可变配置快照                                    | NC-05 |
| INFRA-17  | 基础 | 测试不依赖外部资源，单元测试无 I/O                                 | NC-10 |
| VIEW-01   | 视图 | Ctrl+滚轮横向缩放，Shift+滚轮波形振幅，无修饰键滚动                     | D-03  |
| VIEW-02   | 视图 | 顶部 MiniMapScrollBar 缩略图取代底部 QScrollBar              | D-04  |
| VIEW-03   | 视图 | 选中层级边界线贯穿所有子图；非选中仅标签区域                              | D-05  |
| VIEW-04   | 视图 | 右键播放分割区域+红色游标贯穿，200ms 超时清除                          | D-06  |
| VIEW-05   | 视图 | splitter 各图比例持久化到用户配置                               | D-07  |
| VIEW-06   | 视图 | 统一 ViewportController + 查表法刻度算法，所有子图共享              | D-08  |
| VIEW-07   | 视图 | y 轴仅保留 dB 参考刻度，不绘制横向虚线                              | D-09  |
| VIEW-08   | 视图 | 导出页增加只读表格预览 CSV 内容                                  | D-10  |
| VIEW-09   | 视图 | 子图默认排列：波形图→Power(可选)→口型(待开发)→频谱图                    | D-16  |
| VIEW-10   | 视图 | 高层级边界强制对齐低层级 + 拖动受同层邻边界约束                           | D-17  |
| VIEW-11   | 视图 | 重叠边界线选中索引大者，clamp 确保前后方可分离                          | D-18  |
| VIEW-12   | 视图 | 切点修改后切换页面弹窗提示返回 Slicer 重新切片                         | D-20  |
| VIEW-13   | 视图 | tierCount==0 时标签区域显示居中灰色 "No label data"            | D-21  |
| VIEW-14   | 视图 | 最右侧边界以音频总时长为右边界                                     | D-22  |
| VIEW-15   | 视图 | 不存在的工程路径灰色+删除线，双击弹窗移除                               | D-23  |
| VIEW-16   | 视图 | 废弃 PPS，统一 resolution 连续范围+对数步进表                     | D-26  |
| VIEW-17   | 视图 | activeTierIndex 仅控制子图贯穿显示，所有层级均可拖动                  | D-28  |
| VIEW-18   | 视图 | 低层级边界不得越出高层级区间（跨层 clamp）                            | D-29  |

### 附录速查

| 附录                    | 内容                    |
|-----------------------|-----------------------|
| [附录A](#附录a已废止决策索引)    | 已废止决策索引（D-14、D-24）    |
| [附录B](#附录badr-冲突解决记录) | ADR 冲突解决——旧决策→新决策演变记录 |

---

## 关联关系说明

本文档中，以下关系用于表示条目之间的关联：

- **实施自**：某决策是某准则的具体实施方案（如 VIEW-16 实施自 ARCH-07 的开闭原则——新 resolution 方案通过新增配置替换旧方案）
- **关联**：两个条目在功能上相互依赖或共同作用（如 CONCUR-02「存活令牌」是 CONCUR-03「多线程安全」的过渡方案）
- **约束**：某条目对另一条目的行为施加限制（如 VIEW-18「跨层 clamp」约束 VIEW-10「拖动约束」）
- **取代**：新条目废弃旧条目（见附录 A）

---

## 第一章：ARCH — 架构与模块设计

模块边界、接口契约、组合/继承、适配器模式——决定代码如何组织和演进。

### ARCH-01：模块职责单一，行为不得分散重复

**原文**：P-01

**原则**：相同的行为逻辑只允许存在于一个地方。如果一个操作需要在多个应用/页面中执行，必须下沉到共享基类或框架层，由该层统一实现和通知，各消费者通过接口/信号/虚方法接入。

**禁止**：

- 同一操作（如 "模型数据变化后刷新 UI"）在每个页面各写一份，行为略有不同
- Widget 内部手动调用多个兄弟 widget 的 `update()`——这属于容器的职责
- 每个页面各自拼凑 "保存 splitter → maybeSave → 关闭快捷键" 的生命周期序列

**正确做法**：

- 容器提供统一的 `invalidateXxx()` 方法和信号，一次调用刷新所有子组件
- 基类实现生命周期骨架（模板方法模式），子类只实现差异化的 hook
- 路径编码、文件 I/O 等平台差异封装在一处，所有模块调同一个函数

**关联**：ARCH-06（依赖倒置）——统一行为往往通过抽象接口暴露；ARCH-11（统一视图）——本原则的具体实践。

---

### ARCH-02：被动数据接口 + 外部通知优于观察者膨胀

**原文**：P-02

**原则**：纯数据接口（如 `IBoundaryModel`）保持简洁，不引入 QObject/信号。数据变更的通知由持有数据的容器负责，通过容器级方法（如
`AudioVisualizerContainer::invalidateBoundaryModel()`）统一分发。

**理由**：

- 避免让每个 model 类都继承 QObject（运行时开销 + moc 依赖）
- 通知逻辑集中在容器，调用者只需一行代码
- 新增 widget 时自动被容器刷新，无需修改 model

**关联**：ARCH-01（职责单一）——通知职责归容器而非数据模型，正是职责单一原则的体现。

---

### ARCH-03：接口稳定

**原文**：P-06

**原则**：公共头文件（`include/` 目录下的头文件）即契约。对外接口的变更需谨慎考虑向后兼容性。框架层（dsfw）的接口稳定性要求高于应用层。

**关联**：ARCH-06（依赖倒置）——接口稳定是 DIP 的前提：不稳定接口无法作为依赖抽象。

---

### ARCH-04：相似模块统一设计

**原文**：P-12

**原则**：相似模块使用相似的设计思路。大部分功能相同的模块尽量使用同一个类、不同实例按需开启部分功能，而非创建多个高度相似的类。

**判断标准**：

- 两个类有 >60% 相同代码 → 合并为同一类 + 配置开关
- 两个类有 30%~60% 相同代码 → 提取共同基类，差异通过虚方法/配置实现
- 两个类有 <30% 相同代码 → 可独立实现，但接口风格应保持一致

**关联**：ARCH-11（统一视图）——SlicerPage 和 PhonemeEditor 的统合是本原则的直接实践。

---

### ARCH-05：组合优于继承

**原文**：P-14

**原则**：优先通过组合/委托复用功能，而非构建深的继承层次。接口继承（纯虚类）是好的设计，实现继承需谨慎——只有在"is-a"
关系明确且基类行为无需大幅修改时才使用。

**理由**：

- 深层继承导致"脆弱的基类问题"——修改基类可能意外破坏所有子类
- 组合更灵活——可在运行时替换被组合的对象（策略模式）
- 多重继承（尤其是菱形继承）增加复杂性和歧义
- KDE Frameworks 明确建议在可组合优于继承的场景使用组合

**判断标准**：

- "is-a" 关系 + 行为可复用 → 公共基类（如 `EditorPageBase`）
- "has-a" 关系 + 行为复用 → 组合/委托（如 `BoundaryDragController` 被 widget 持有，而非 widget 继承自它）
- 仅需部分行为 → 策略模式 / 配置开关（如 ARCH-04 的 Config 结构体）

**来源**：C++ Core Guidelines (C: Classes and class hierarchies)、KDE Frameworks Policies、Qt API Design Principles。

---

### ARCH-06：依赖倒置（面向接口编程）

**原文**：P-15

**原则**：高层模块不应依赖低层模块，二者都应依赖抽象（纯虚接口）。具体实现通过 `ServiceLocator` 注入（全局服务）或构造函数注入（局部依赖）。

**理由**：

- 降低耦合——更换底层实现时高层代码无需修改
- 便于单元测试——可注入 mock 实现
- 与 ARCH-03（接口稳定）互补：ARCH-03 强调接口的价值，本原则强调依赖的方向

**实现要求**：

- 框架层定义纯虚接口（`IFileIOProvider`、`IModelProvider`、`IG2PProvider`、`IFormatAdapter`）
- 应用层提供具体实现，通过 `ServiceLocator` 或构造函数注入
- 页面/业务逻辑代码只依赖接口，不直接依赖具体类
- 新增功能优先考虑新增接口 + 实现，而非在现有类上打补丁

**来源**：SOLID 原则（Dependency Inversion Principle）、C++ Core Guidelines (I: Interfaces)、LLVM 库分层设计。

**关联**：INFRA-01（ServiceLocator 限定）——ServiceLocator 是本原则的注入机制，但仅限全局服务。

---

### ARCH-07：开闭原则（对扩展开放，对修改关闭）

**原文**：P-16

**原则**：新增功能应通过新增类/模块/插件实现，而非修改已稳定的现有代码核心逻辑。

**理由**：

- 修改已稳定代码引入回归风险
- 新增适配器/插件不影响现有功能
- LLVM 的 Pass 注册机制、Qt 的插件系统都是此原则的经典实践

**实践方式**：

- 新文件格式 → 新增 `IFormatAdapter` 子类，注册到 `FormatAdapterRegistry`
- 新推理引擎 → 新增 `IModelProvider` 实现，不修改 `ModelManager` 核心流程
- 新页面 → 新增 Page 类实现 `IPageLifecycle`/`IPageActions`，不修改 `AppShell` 核心
- 新导出格式 → 新增 `IExportFormat` 实现

**来源**：SOLID 原则（Open-Closed Principle）、Qt Plugin System、LLVM Pass Registry。

**关联**：ARCH-08（适配器隔离）——适配器模式是开闭原则在文件格式领域的具体落地。

---

### ARCH-08：内部文档模型 + 适配器隔离

**原文**：P-17

**原则**：本项目维护统一的内部文档模型（`DsDocument`、`TextGridDocument` 等），所有具体文件格式（CSV、TextGrid、Lab、ds、json 等）*
*一律通过 IFormatAdapter 适配器**对接内部模型，**不得在业务代码中直接操作文件**。

**核心要求**：

1. **内部文档模型是唯一的真相源**：所有页面、处理流程、推理引擎只与内部模型交互，不感知底层文件格式。

2. **文件 I/O 必须经过适配器**：格式读取/写入通过 `IFormatAdapter` 子类实现，适配器负责格式 ↔ 内部模型的转换。适配器内部可直接使用
   QFile、JsonHelper、PathUtils 等底层工具，无需额外抽象层。

3. **迁移模块必须重构为适配器模式**：从其他项目迁移进来的模块，如果原生代码直接操作文件，必须重构：提取出内部模型 + 适配器。

4. **禁止模式**：
    - 业务代码中调用 `QFile`、`std::ifstream` 直接读写格式文件
    - 页面加载切片时直接 `nlohmann::json::parse(file)` 然后手动塞入 UI 控件
    - 导出功能中自己拼 CSV 字符串写入文件

5. **正确模式**：
   ```
   业务代码                    适配器层                   文件系统
   ────────                   ────────                   ────────
   DsDocument.load(path)  →  DsFileAdapter.read()   →  QFile + PathUtils
   TextGridDocument.load() → TextGridAdapter.read()  →  QFile + PathUtils
   exportService.export()  →  CsvAdapter.write()     →  QFile + PathUtils
   DsProject.load()        →  JsonHelper::loadFile()  →  QFile + PathUtils
   ```

**来源**：Qt Model/View 架构、Hexagonal Architecture（端口-适配器模式）、C++ Core Guidelines (A: Architectural Ideas)。

**关联**：ARCH-07（开闭原则）——适配器模式是本原则在文件格式领域的具体实践；INFRA-05（统一路径库）——适配器内部使用 PathUtils
做路径编码。

**2026-05-25 修订**：移除 IFileIOProvider 强制通道要求。适配器内部直接使用 QFile + PathUtils 即可，无需引入只有 1 个实现的
IFileIOProvider 抽象层。

**2026-05-25 补充**：文件 I/O 分为两类，处理方式不同：

1. **格式读写 I/O**（必须通过适配器）：读取/写入业务文档（.ds、.csv、.textgrid 等结构化格式）。业务代码必须通过 `IFormatAdapter`
   进行。
2. **工具性文件操作**（可直接调用）：文件拷贝、存在性检查、目录遍历、获取文件信息等非格式性操作。业务代码可直接使用
   `QFile::copy()`、`QFile::exists()`、`QDir` 等工具函数。

示例：

- ✅ `ExportPage::QFile::copy()` 音频文件到输出目录 — 工具性操作，允许直接调用
- ✅ `DirectoryDataSource::refresh()` 用 `QDir` 遍历目录 — 工具性操作，允许
- ❌ `FileDataSource::loadSlice()` 直接 `QFile::readAll()` 读取 .ds 文件 — 格式读写，必须通过 `DsFileAdapter`

---

### ARCH-09：标签区域设计

**原文**：D-15

**决策**：

- 标签区域位于**刻度线下方、波形图上方**（即在所有子图的上方）
- **Slicer**：单层标签，自动按顺序标数字，边界线向下贯穿所有图
- **PhonemeLabeler**：多层标签（TextGrid 层），标签区域**最左侧**有一组 **radio button** 用于选择当前活跃层级
    - 选中层级的边界线**向下贯穿**所有子图
    - 非选中层级的边界线仅在标签区域内显示
- 标签区域是 AudioVisualizerContainer 的固定组成部分

**关联**：VIEW-17（activeTier 贯穿控制）——本决策定义了层级选择的 UI 方式；VIEW-03（贯穿规则）——边界线贯穿的具体规则。

---

### ARCH-10：所有页面统一使用全功能文件/切片列表

**原文**：D-19

**决策**：

- 所有应用页面（Slicer、MinLabel、PhonemeLabeler、PitchLabeler）的左侧文件/切片列表**统一使用 SliceListPanel**
- 功能包括：添加文件/文件夹、丢弃/恢复、进度条、右键菜单
- `AudioFileListPanel`（原始音频文件列表）仅在 Slicer 页面保留，其他页面使用切片列表

**关联**：ARCH-01（职责单一）——统一列表组件避免各处重复实现。

---

### ARCH-11：Slicer/Phoneme 统一视图组合系统

**原文**：D-30

**决策**：Slicer 和 Phoneme 不是"共享同一基类"——而是**使用完全相同的视图 UI 实例**，仅通过配置选择性地显示/隐藏不同组件。

**核心原则**：

1. **同一套 UI，不是两套 UI**：SlicerPage 和 PhonemeEditor 使用完全相同的 `AudioVisualizerContainer` 实例化方式。

2. **所有组件平等注册**：所有 chart 类型在 `AudioVisualizerContainer` 初始化时全部注册，根据页面类型和用户配置选择性显示。

3. **组件可见性和顺序由配置驱动**：`chartVisible` + `chartOrder` 配置项持久化到用户目录。

4. **组件矩阵**：

| 组件                    |     Slicer 默认     |         Phoneme 默认          |  用户可配置  |
|-----------------------|:-----------------:|:---------------------------:|:-------:|
| MiniMapScrollBar      |         ✅         |              ✅              |  -（固定）  |
| TimeRuler             |         ✅         |              ✅              |  -（固定）  |
| TierLabelArea         | ✅（SliceTierLabel） | ✅（PhonemeTextGridTierLabel） |  -（固定）  |
| WaveformWidget        |         ✅         |              ✅              | ✅ 显隐+顺序 |
| PowerWidget           |         ❌         |              ✅              | ✅ 显隐+顺序 |
| SpectrogramWidget     |         ✅         |              ✅              | ✅ 显隐+顺序 |
| BoundaryOverlayWidget |         ✅         |           ✅ 活跃层贯穿           |  -（固定）  |
| TierEditWidget        |         ❌         |              ✅              | -（页面决定） |

5. **持久化**：`ViewLayout/chartOrder`、`ViewLayout/chartVisible`、`ViewLayout/chartSplitterState` 保存在 AppSettings。

**关联**：ARCH-04（相似模块统一）——本决策是该原则的核心实践；取代了 [附录A] D-14「AudioVisualizerContainer 基类方案」。

---

## 第二章：CONCUR — 异步与并发安全

异步化策略、线程安全保证、资源存活保护。

### CONCUR-01：阻塞操作一律异步化

**原文**：P-03

**原则**：任何可能耗时超过 50ms 的操作（模型加载、推理运行、批量文件操作、大数据校验）禁止在主线程同步执行。

**实现要求**：

- 模型加载 → `QtConcurrent::run` + 信号通知就绪
- 批量推理/补全 → 后台线程 + `QMetaObject::invokeMethod` 回主线程更新进度
- 禁止使用 `QApplication::processEvents()` 作为 "防止卡死" 的手段——这是反模式
- 异步操作完成后必须检查上下文是否仍有效（sliceId 是否仍为当前切片、页面是否仍活跃）

**关联**：CONCUR-02（存活令牌）——异步操作的资源安全前提。

---

### CONCUR-02：异步任务必须持有引擎存活令牌

**原文**：P-09

**原则**：任何 `QtConcurrent::run` 中捕获到并在后台线程调用的原始指针对象，必须在页面中维护一个
`std::shared_ptr<std::atomic<bool>>` "存活令牌"，lambda 在使用引擎前必须先检查令牌。

**背景**：`ModelManager::invalidateModel()` 会销毁推理引擎，但后台线程可能仍持有原始指针副本。QPointer 只能保护 QObject
子类，不能保护原始指针。

**实现模式**：

```cpp
// 页面成员：
std::shared_ptr<std::atomic<bool>> m_engineAlive;

// 引擎就绪时：
m_engineAlive = std::make_shared<std::atomic<bool>>(true);

// lambda 中：
auto alive = m_engineAlive;
QtConcurrent::run([engine, alive, ...]() {
    if (!alive || !*alive) return;
    engine->process(...);
});

// 引擎失效时：
m_engineAlive.reset();
```

**注意**：此方案是**过渡性补救**，存在 TOCTOU（check-then-use）间隙。彻底解决需要引擎层自身提供 shared_ptr 所有权或线程安全的访问令牌。

**关联**：CONCUR-03（多线程安全）——存活令牌是多线程安全策略的一部分。

---

### CONCUR-03：多线程安全优先

**原文**：P-11

**原则**：批处理和多页面设计必须考虑多线程竞争问题。共享可变状态必须通过锁（`std::mutex`）或原子操作（`std::atomic`）保护。

**实现要求**：

1. 所有推理引擎的 `process()` 方法必须受 `std::mutex` 保护
2. 批量处理任务的取消标志必须使用 `std::atomic<bool>`
3. 异步回调中访问页面成员前必须检查存活令牌（CONCUR-02）
4. 新增共享资源时必须同步考虑线程安全策略

**关联**：CONCUR-01（异步化）——线程安全是异步化的必然要求；INFRA-06（RAII）——锁的 RAII 包装是线程安全的基础保证。

---

## 第三章：ROBUST — 健壮性与错误处理

错误传播链、异常隔离策略、简洁原则。

### ROBUST-01：错误信息必须追溯到根因

**原文**：P-04

**原则**：错误消息必须包含导致错误的实际原因，而非现象描述。每一层函数如果通过 out-parameter 或 Result
报告了错误，调用者必须在第一时间检查并传播，不得忽略后继续执行并报出无关的二次错误。

**反例**：`resample_to_vio` 返回错误消息 `msg`，但调用者忽略它继续读空 VIO，报 "0 samples"。
**正确做法**：检查 `msg` → 立即返回 `Err("Failed to resample: " + msg)`。

**关联**：ROBUST-02（Result\<T\>）——错误传播机制的实现基础。

---

### ROBUST-02：异常边界隔离

**原文**：P-05

**原则**：应用层逻辑使用 `Result<T>` 传播错误，**禁止抛出异常**。`try-catch` 仅用于第三方库边界（nlohmann/json、ONNX
Runtime、FFmpeg 等），将外部异常转为 `Result<T>::Error()`。每个 `catch` 块必须记录日志或返回错误，禁止静默吞掉。禁止在业务逻辑中使用
`try-catch` 做流程控制。

**来源**：吸收自 language-manager 项目的异常边界隔离原则。

**关联**：ROBUST-03（简洁可靠）——不依赖异常做流程控制，保证代码路径可预测。

---

### ROBUST-03：简洁可靠

**原文**：P-07

**原则**：遇错直接返回，不设计重试或回滚（除有明确需求如 `BatchCheckpoint` 断点续处理外）。优先选择简单直接的方案，避免过度设计。

**来源**：吸收自 language-manager 项目的 "Write Once, Run Forever" 设计理念。

**关联**：ROBUST-01（错误根因）——简洁的错误直接返回才能保证根因信息不被中间层污染。

---

## 第四章：INFRA — 基础设施与配置

路径、资源管理、配置系统、快捷键——支撑所有上层功能的底层约定。

### INFRA-01：每页独立资源，禁止全局共享（ServiceLocator 误用防护）

**原文**：P-08

**原则**：ServiceLocator 仅用于**真正的全局服务**（如 `ModelManager`、`IG2PProvider`、`FormatAdapterRegistry`）。页面级资源（如
`IAudioPlayer`、widget 实例）不得通过 ServiceLocator 全局共享。

**判断标准**：

- ✅ 全局服务：所有页面共享同一实例有意义（文件存储、模型注册表、G2P 字典）
- ❌ 页面资源：每个页面需要独立实例（音频播放器、widget 状态机）

**禁止模式**（旧代码）：

```cpp
// ❌ 第一个 PlayWidget 注册全局 IAudioPlayer，后续所有共享
if (auto *shared = ServiceLocator::get<IAudioPlayer>()) {
    m_player = shared;
    return;
}
ServiceLocator::set<IAudioPlayer>(m_player);
```

**正确做法**：每个 PlayWidget 创建自有的 AudioPlayer，不注册到 ServiceLocator。

**关联**：ARCH-06（依赖倒置）——ServiceLocator 是 DIP 的注入机制，本原则限制其适用范围；INFRA-07（构造函数注入）——页面依赖的推荐注入方式。

---

### INFRA-02：接口必要性原则（新增）

**新增原则**

**原则**：新增接口前必须确认存在**至少两个预期实现**，或有明确的测试隔离需求。单一实现场景优先使用具体类。

**理由**：

- 接口抽象的价值在于支持多实现和可替换性
- 无多实现时，接口增加了一层间接调用和认知负担，却无收益
- 项目历史教训：`IFileIOProvider`、`ISettingsBackend`、`ISlicerService` 等接口仅有一个实现，最终被删除

**判断标准**：

- ✅ 有明确的多后端需求（如 `IFormatAdapter` 支持多种文件格式）
- ✅ 有测试 mock 需求（如 `ITaskProcessor` 需要 mock 测试）
- ❌ "理论上可能需要"但当前无具体计划
- ❌ 仅为了"符合 DIP 原则"而创建接口

**正确做法**：

```cpp
// ✅ 正确：有多个实现
class IFormatAdapter { ... };  // CsvAdapter, DsFileAdapter, TextGridAdapter
class ITaskProcessor { ... }; // SlicerProcessor, HubertAlignmentProcessor, etc.

// ✅ 正确：单一实现，但有测试需求
class ModelManager { ... };    // 可在测试中直接实例化

// ❌ 错误：仅为"DIP"创建无用接口
class IModelManager { ... };   // 只有一个 ModelManager 实现
```

**关联**：ARCH-06（依赖倒置）——本原则防止 DIP 过度应用；INFRA-07（可测试性）——可测试性不等于必须用接口。

---

### INFRA-03：实现优先原则（新增）

**新增原则**

**原则**：先实现具体类，待出现多实现需求时再抽取接口。接口应该是演进的，而非一开始就设计好。

**理由**：

- 过早抽象（YAGNI 原则）是过度工程的根源
- 具体实现更容易理解和修改
- 当真正需要多实现时，接口设计会更准确（基于实际需求而非猜测）

**实践**：

1. **第一阶段**：用具体类实现功能
2. **第二阶段**：当需要第二种实现时，识别共同抽象，抽取接口
3. **第三阶段**：将第一种实现作为接口的实现类

**示例**：

```cpp
// 阶段1：直接使用具体类
class ModelManager { ... };  // 直接实现

// 阶段2：出现第二种实现时，抽取接口
class IModelManager { ... }; // 抽象接口
class ModelManager : public IModelManager { ... };  // 具体实现
class MockModelManager : public IModelManager { ... };  // 测试用
```

**关联**：ARCH-07（开闭原则）——本原则防止开闭原则过度应用；INFRA-02（接口必要性）——接口必要性是实现优先的前提。

---

### INFRA-04：简化全局服务访问（新增）

**新增原则**

**原则**：全局服务优先通过**直接访问函数**（如 `ModelManager::instance()`）获取，而非通过 ServiceLocator。ServiceLocator
仅作为测试兼容层保留。

**理由**：

- 直接访问更简单直接，减少间接调用层
- 编译期可检查可用性（而非运行时 nullptr）
- ServiceLocator 的类型擦除特性不利于调试

**实现方式**：

```cpp
// 方式A：单例模式（推荐用于全局唯一服务）
class ModelManager {
public:
    static ModelManager &instance();  // 线程安全初始化
private:
    ModelManager();  // 私有构造函数
    static std::unique_ptr<ModelManager> s_instance;
};

// 方式B：保留 ServiceLocator 仅用于测试
namespace dstools {
    ModelManager &modelManager();  // 内部调用 ServiceLocator 或直接单例
}
```

**禁止模式**：

```cpp
// ❌ 禁止：强制通过 ServiceLocator 获取单一实现服务
auto *mm = ServiceLocator::get<IModelManager>();
```

**关联**：INFRA-01（ServiceLocator 限定）——本原则进一步限制了 ServiceLocator 的使用场景；INFRA-03（实现优先）——单一实现可直接使用具体类。

---

### INFRA-05：统一路径库，禁止各处自行拼接

**原文**：P-10

**原则**：基于 `std::filesystem` 设计跨平台路径库（`dsfw::PathUtils`），路径拼接、编码转换、输出到 debug
信息等均使用此库，禁止各处自行拼接路径或实现编码转换。

**理由**：

- 编码转换有 3 处重复实现（`toFsPath` / `toStdPath` / `toFsPath`），行为完全相同
- `path.string()` 在 Windows 上产生 ANSI 乱码
- 缺少 `path → UTF-8 string` 的统一方法

**实现要求**：

1. `dsfw::PathUtils` 提供 `join()`、`toUtf8()`、`toWide()` 方法
2. 统一使用 `dsfw::PathUtils::toStdPath()`
3. 所有 `path.string()` 替换为 `PathUtils::toUtf8(path)` 或 `PathUtils::toWide(path)`
4. 所有手动路径拼接替换为 `PathUtils::join()` 或 `std::filesystem::path::operator/`

**关联**：ARCH-01（职责单一）——路径操作收敛到单一函数。

---

### INFRA-06：RAII 资源管理

**原文**：P-13

**原则**：所有资源（内存、文件句柄、锁、推理引擎句柄）的生命周期必须绑定到拥有对象的作用域。使用 C++ 标准库的 RAII 包装管理资源。

**禁止**：

- 裸露 `new`/`delete`（应使用 `std::make_unique`/`std::make_shared`）
- 手动 `mutex.lock()`/`unlock()`（应使用 `std::lock_guard` 或 `std::scoped_lock`）
- 推理引擎裸指针在多线程间传递（CONCUR-02 是过渡方案）

**来源**：C++ Core Guidelines (R: Resource Management)、Qt API Design Principles、LLVM 编码规范。

**关联**：CONCUR-03（多线程安全）——锁的 RAII 包装；CONCUR-02（存活令牌）——引擎生命周期的 RAII 过渡方案。

---

### INFRA-07：可测试性优先

**原文**：P-18

**原则**：接口设计考虑可 mock 性，构造函数注入优于 ServiceLocator。ServiceLocator 仅用于不便通过构造函数传递的全局服务。

**关联**：ARCH-06（依赖倒置）——可 mock 的接口是 DIP 的直接结果；INFRA-01（ServiceLocator 限定）——明确了注入方式的选择标准。

---

### INFRA-08：配置键统一

**原文**：P-19

**原则**：使用 `SettingsKey<T>` 强类型封装配置键，所有配置键集中定义在 `Keys.h` 中，禁止各处散落字符串 key。

**关联**：ARCH-01（职责单一）——配置定义集中管理。

---

### INFRA-09：配置全部移出工程文件

**原文**：D-01

**决策**：模型配置、设备选择、UI 偏好（图表比例、图表顺序等）**全部**存储在用户目录下（`~/.config/dataset-tools/` 或
`%APPDATA%/dataset-tools/`），不存储在 `.dsproj` 工程文件中。

**影响**：

- `.dsproj` 仅保留工程数据（items、languages、completedSteps、slicer）
- 换工程后保留用户原先的模型配置和 UI 体验

**关联**：VIEW-05（splitter 比例持久化）——比例值也走用户配置，不存工程。

---

### INFRA-10：模型懒加载

**原文**：D-02

**决策**：所有推理模型（ASR、HuBERT-FA、RMVPE、GAME）改为懒加载——首次用到时再按 Settings 页配置加载，而非应用启动时加载。

**理由**：减少启动时间，避免加载未使用的模型浪费显存。

**关联**：CONCUR-01（异步化）——模型加载本身需异步执行。

---

### INFRA-11：Slicer 音频切点持久化

**原文**：D-11

**决策**：通过文件夹按钮加载的音频文件列表及每个文件的切点信息，必须保存到 `.dsproj`，关闭重开工程后自动恢复。

**关联**：INFRA-09（配置移出工程文件）——切点属于工程数据而非用户配置，故存 `.dsproj`。

---

### INFRA-12：快捷键系统

**原文**：D-13

**决策**：

- 每个页面的重要工具按钮分配快捷键（Slicer: S=自动切片, E=导出; PhonemeLabeler: F=FA; 等）
- 每个应用的菜单中展示各自的快捷键
- 文件列表上的 +/- 等按钮**不需要**快捷键
- 所有快捷键需在菜单和工具栏 tooltip 中可见

---

### INFRA-13：PIMPL 隔离第三方头文件

**原文**：NC-01

**原则**：所有包含第三方库头文件的公共接口类必须使用 PIMPL 惯用法，将第三方依赖限制在 .cpp 文件中。header-only 的第三方库（如
nlohmann/json）不受此限制。

**理由**：

- 减少编译时间（第三方头文件通常体积大）
- 避免符号冲突（ONNX、FFmpeg 等库的宏定义可能污染用户代码）
- 保持 ABI 兼容性（第三方库升级不影响公共接口）

**实施**：

```cpp
// 公共头文件
class AudioDecoder {
public:
    AudioDecoder();
    ~AudioDecoder();  // 必须在 .cpp 中定义
    Result<AudioData> decode(const QString &path);
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
```

**关联**：INFRA-06（RAII）——PIMPL 的 unique_ptr<Impl> 依赖 RAII 自动析构。

---

### INFRA-14：接口版本化与弃用策略

**原文**：NC-08

**原则**：所有公共接口必须声明版本号（`static constexpr int kInterfaceVersion`）。接口变更时递增版本号，旧接口保留一个版本周期后删除。

**理由**：

- 支持渐进式重构（不需要一次性改完所有实现）
- 明确弃用窗口，给下游代码迁移时间
- 版本号帮助排查接口不兼容问题

**实施**：

```cpp
class IFormatAdapter {
public:
    static constexpr int kInterfaceVersion = 1;
    // ...
};
```

**已实施接口**
：IFormatAdapter、ITaskProcessor、IModelProvider、IG2PProvider、IExportFormat、IInferenceEngine、IBoundaryModel、IEditorDataSource、IEnginePoolHost、IPlaybackEvents、IPageDescriptor。

---

### INFRA-15：信号签名规范

**原文**：NC-09

**原则**：信号参数遵循以下规则：

- `sizeof(T) <= 2 * sizeof(void*)` → 值传递
- `sizeof(T) > 2 * sizeof(void*)` → const ref 传递（需 QMetaType 注册）
- 跨线程信号 → 值传递或 QMetaType 注册的 const ref

**理由**：

- 减少不必要的拷贝
- 确保跨线程信号安全
- 与 Qt 元对象系统兼容

---

### INFRA-16：不可变配置快照

**原文**：NC-05

**原则**：流水线执行期间使用的配置必须是从 `AppSettings` 快照的不可变对象，不得在流水线运行中读取可变设置。

**理由**：

- 防止流水线执行期间 UI 设置变更导致数据不一致
- 简化并发控制（不可变对象天然线程安全）
- 支持配置审计和问题复现

**实施**：`PipelineRunner::run()` 接收 `const PipelineOptions &`，成员非 const（保持 move/copy），通过 const 引用传递保证不可变性。

---

### INFRA-17：测试边界清晰

**原文**：NC-10

**原则**：单元测试不依赖外部资源（文件系统、网络、模型文件）；集成测试使用内置测试数据。

**理由**：

- 测试可重复运行，不受环境变化影响
- 测试速度更快（无 I/O 等待）
- CI 环境友好

**关联**：ARCH-06（依赖倒置）——接口注入 mock 实现是测试隔离的前提。

---

## 第五章：VIEW — 视图与交互

缩放、边界线、拖拽约束、布局切换——所有用户可见的视觉与操作行为。

### VIEW-01：Ctrl+滚轮横向缩放，Shift+滚轮波形幅度

**原文**：D-03

**决策**：

- **Ctrl+滚轮**：横向缩放音频（所有波形图/频谱图/编辑区统一）
- **Shift+滚轮**：调整波形图垂直振幅（仅波形图生效）
- **滚轮（无修饰键）**：横向滚动

---

### VIEW-02：顶部缩略图滚动条取代底部滚动条

**原文**：D-04

**决策**：

- 移除所有波形编辑界面底部的 QScrollBar
- 最顶部放置**迷你波形缩略图 + 滑窗矩形**（MiniMapScrollBar）
- 缩略图显示完整音频的迷你波形
- 滑窗矩形指示当前视口范围，可拖动和缩放

---

### VIEW-03：分割线纵向贯穿规则

**原文**：D-05

**决策**：

- **选中层级**的边界线：从标签区域**向下贯穿**所有子图（波形图、power 图、mel 频谱图等）
- **非选中层级**的边界线：仅在标签区域内显示
- Slicer 只有单层标签（自动编号），边界线始终贯穿所有图

**实现方向**：BoundaryOverlayWidget 覆盖标签区域 + 所有子图区域，根据选中层级决定绘制范围。

**关联**：VIEW-17（activeTier 贯穿控制）——activeTierIndex 决定"选中层级"。

---

### VIEW-04：右键播放当前分割区域 + 红色播放游标

**原文**：D-06

**决策**：

- 在任意波形图或频谱图右键 → 直接播放点击位置所在的边界分割区域
- 播放时显示红色竖线游标，贯穿所有图
- 播放完毕后游标消失（200ms 超时自动清除）
- 不弹出右键菜单

---

### VIEW-05：记录各图手动调整后的比例

**原文**：D-07

**决策**：用户手动拖动 splitter 调整各图比例后，比例值持久化到用户配置目录（不存工程），下次打开任何工程仍保持。

**关联**：INFRA-09（配置移出工程）——比例属于 UI 偏好，走用户目录。

---

### VIEW-06：刻度线重新设计

**原文**：D-08

**决策**：

- 参考 ds-editor-lite 的 `ITimelinePainter` 刻度算法（基于 `minimumSpacing` + `spacingVisibility` 渐显渐隐）
- 刻度线只能和波形图等同步缩放——不允许其他子图单独缩放到极限后刻度仍可缩放
- 所有子图共享同一个 ViewportController，刻度统一绑定

---

### VIEW-07：y 轴仅保留 dB 刻度，去掉横向虚线

**原文**：D-09

**决策**：

- 波形图左侧保留 dB 参考刻度（0dB / -6dB / -12dB / -24dB）
- 不绘制横向贯通的虚线网格
- 频谱图、功率图不需要 y 轴刻度

---

### VIEW-08：导出页 CSV 预览

**原文**：D-10

**决策**：在导出页增加"预览数据" tab，以只读表格展示输出的 transcriptions.csv 内容。

---

### VIEW-09：子图默认排列顺序

**原文**：D-16

**决策**：垂直布局从上到下：MiniMapScrollBar → TimeRuler → 标签区域 → 波形图 → Power 图（可选）→ 口型曲线图（待开发）→ Mel
频谱图。用户可在 Settings 中自定义子图排列顺序。

**关联**：ARCH-11（统一视图组合系统）——排列顺序的配置化管理。

---

### VIEW-10：标签层级包含规则与拖动约束

**原文**：D-17

**决策**：

- **层级包含规则**：高层级（如 words）的边界强制对齐低层级（如 phones）的边界——高层级的每个区间边界必须与某个低层级的边界重合。
- **拖动约束**：拖动任何边界线/分割线时，不允许超越同层级内相邻的边界线。即边界 B 被夹在左邻边界 L 和右邻边界 R 之间时，拖动
  B 的范围被限制在 (L, R) 内。
- Slicer 同理：切割线不允许拖过相邻切割线。

**关联**：VIEW-18（跨层 clamp）——本原则的纵向扩展。

---

### VIEW-11：重叠边界线的选择与拖动策略

**原文**：D-18

**决策**：

- 当多条边界线在同一像素位置重叠时，鼠标点击/悬停优先选中**索引较大（靠后）的边界线**
- 理由：重叠通常发生在自动对齐或导入数据后，用户意图是将后方的边界向右拖开
- clamp 规则不变：前方边界只能向左移动，后方边界只能向右移动——这正好是"分离重叠"的正确行为
- hitTest 范围内存在多个候选时，像素距离最近者优先；距离相同时索引较大者优先（tie-break to right）

**关联**：VIEW-10（拖动约束）——重叠分离依赖 clamp 约束。

---

### VIEW-12：切片不一致时弹窗提示回 Slicer

**原文**：D-20

**决策**：

- 当用户从 Slicer 切换到其他页面时，如果 dsitem 时长与 Slicer 当前切点计算出的时长不一致，弹窗提示
- 弹窗内容："切片数据已过期（切点已修改），是否回到 Slicer 页面重新切片？"
- 点"是"→ 回到 Slicer 页面，弹复选框对话框选择需重新切片的文件（默认全选变化过的）
- 点"否"→ 留在当前页面，使用旧的 dsitem 数据

---

### VIEW-13：Phoneme 页面无数据时显示"No label data"

**原文**：D-21

**决策**：

- PhonemeLabeler 在**没有任何层**（tierCount == 0）时，标签区域仍显示，内容为居中的灰色文字 "No label data"
- 不因 tierCount == 0 而将标签区域高度设为 0 或隐藏

---

### VIEW-14：最右侧边界线自由拖动 + 边界修复

**原文**：D-22

**决策**：

- TextGridDocument 中最右侧（最后一条）边界线的 clamp 约束应以**音频总时长**为右边界，而非 `tier->GetMaxTime()`
- 修复：最后一条边界（`boundaryIndex == count - 1`）的 nextBoundary 应使用 `tier->GetMaxTime()`（即文档总时长）
- 同时排查 SliceBoundaryModel 是否有类似问题

---

### VIEW-15：最近工程列表标灰找不到的路径

**原文**：D-23

**决策**：

- WelcomePage 的 `refreshRecentList()` 对每个路径检查 `QFileInfo::exists()`
- 不存在的路径：item 文字设为灰色 + 删除线，双击时弹出 "工程文件不存在，是否从列表中移除？"
- 不自动移除（用户可能只是 U 盘未插入）

---

### VIEW-16：比例尺从固定列表改为连续范围 + 页面独立默认值

**原文**：D-26

**决策**：

1. **废弃 PPS 概念，统一使用 resolution**：所有 widget 内部 `m_pixelsPerSecond` 改为 `m_resolution`。时间↔像素转换统一通过
   `resolution` 和 `sampleRate` 计算。

2. **去掉固定 resolution 列表**：改为连续范围，zoom 步进取整十的对数表：
   ```
   10, 20, 30, 50, 80, 100, 150, 200, 300, 500, 800, 1000,
   1500, 2000, 3000, 5000, 8000, 10000, 15000, 20000, 30000, 44100
   ```
   每步约 ×1.3~1.7，zoomIn/zoomOut 在此表中前后移动一格。

3. **默认 resolution 按页面功能定义**：
    - Slicer 默认：视口显示约 2 分钟音频 → 表中最近值 5000
    - Phoneme 默认：视口显示约 20 秒音频 → 表中最近值 800

4. **放大/缩小极限**：表首 10（@44100Hz 每像素 0.23ms）；表末 44100（=1 px/s，1200px 显示 20 分钟）

5. **每页比例尺独立持久化**：通过 `settingsAppName`（如 "Slicer"、"PhonemeEditor"）保存到 AppSettings。

6. **UI 显示**：比例尺指示器显示 `"{N} spx"`（samples per pixel）。

**关联**：VIEW-06（刻度系统）——ViewportController 统一驱动刻度和缩放；取代了 [附录A] D-24「离散分辨率表」。

---

### VIEW-17：Active tier 只控制子图边界线显示，不限制拖动

**原文**：D-28

**决策**：

- `activeTierIndex` 仅决定哪个层级的边界线在子图中**贯穿显示**
- 所有层级的边界线在标签区域中始终可见且**均可拖动**
- 子图中的 hit-test 搜索**所有层级**的边界（不只 active tier），返回最近边界
- 拖动某个层级的边界不需要先切换到该层级

**关联**：ARCH-09（标签区域设计）——radio button 选 active tier；VIEW-03（贯穿规则）。

---

### VIEW-18：低层级边界不得越出高层级区间（跨层 clamp）

**原文**：D-29

**决策**：

- VIEW-10 的扩展：phoneme 层的某个边界 B 位于 grapheme 层区间 [Gstart, Gend] 内时，B 的拖动范围收紧为
  `[max(sameLayerLeft, Gstart), min(sameLayerRight, Gend)]`
- 该约束作用于 `clampBoundaryTime()` 内部——保持 clamp 的单一入口
- tier index 越小 = 层级越高。tier 0 = 最高层（如 grapheme），不受任何层约束
- 只约束**直接上层**（tier-1），不递归

**关联**：VIEW-10（同层拖动约束）——本原则是其纵向扩展。

---

## 附录A：已废止决策索引

| 原编号  | 摘要                                                | 废止原因                                                 | 取代项                                             |
|------|---------------------------------------------------|------------------------------------------------------|-------------------------------------------------|
| D-14 | AudioVisualizerContainer 基类方案 + Settings 自定义子图顺序  | 被统一视图组合系统取代                                          | [ARCH-11](#arch-11slicerphoneme-统一视图组合系统)       |
| D-24 | Resolution 范围硬编码 `[10, 400]` + `kResolutionTable` | 被连续 resolution + 对数步进取代                              | [VIEW-16](#view-16比例尺从固定列表改为连续范围--页面独立默认值)      |
| —    | `IFileIOProvider` 抽象接口                            | 仅 1 个实现 `LocalFileIOProvider`，适配器直接使用 QFile          | [ARCH-08](#arch-08内部文档模型--适配器隔离)（2026-05-25 修订） |
| —    | `ISettingsBackend` 抽象接口                           | `ProjectSettingsBackend` 已删除，仅剩 `AppSettingsBackend` | 直接使用 `AppSettingsBackend`                       |
| —    | `ISlicerService` 抽象接口                             | 仅 1 个实现 `SlicerService`，无多后端需求                       | `SlicerService` 改为具体类                           |

以及以下已在实施中废弃的概念：

- `ViewportState::pixelsPerSecond` → 删除，改为
  `resolution`（[VIEW-16](#view-16比例尺从固定列表改为连续范围--页面独立默认值)）
- `ViewportController::setPixelsPerSecond()` / `pixelsPerSecond()` → 删除
- `kResolutionTable` 固定列表 → 删除
- `BoundaryBindingManager` → 废弃，由 `BoundaryDragController` 取代
- `IFileIOProvider` + `LocalFileIOProvider` → 删除，适配器内直接用 QFile + PathUtils
- `ISettingsBackend` + `AppSettingsBackend` → 前者删除，后者保留为普通类
- `ISlicerService` → 删除，`SlicerService` 改为独立具体类

---

## 附录B：ADR 冲突解决记录

当同一问题在不同时期有不同决策时，以**最新决策**为准：

| 冲突项            | 旧决策                                | 新决策                                                                                     |
|----------------|------------------------------------|-----------------------------------------------------------------------------------------|
| 标签区域位置         | TierEditWidget 在频谱图下方              | [ARCH-09](#arch-09标签区域设计)：标签区域在刻度线下方、所有子图上方                                             |
| 子图配置           | D-14：子图顺序可在 Settings 自定义           | [ARCH-11](#arch-11slicerphoneme-统一视图组合系统)：显隐+顺序统一配置，checkbox 列表+拖拽排序                    |
| 贯穿线定位          | D-37：removeTierLabelArea 后贯穿线未正确延伸 | D-40：移除后设置 m_extraTopOffset 确保贯穿线覆盖编辑区                                                  |
| Chart 边界绘制     | Chart widget 不绘制边界线，仅依赖透明 overlay  | 统一由 BoundaryOverlayWidget 叠加层绘制，确保贯穿线不被 splitter 分隔线打断                                  |
| FA 层级输出        | buildFaLayers 只输出 word.start 边界    | D-42：输出完整 start/end 边界 + LayerDependency 结构                                             |
| PitchLabel 工具栏 | PitchLabelerPage 仅有菜单栏             | D-43：添加 QToolBar 含 F0/GAME 按钮                                                           |
| 边界线渲染          | ChartPanelBase 内部绘制边界线             | 统一由 BoundaryOverlayWidget + BoundaryLineRenderer 绘制；移除 ChartPanelBase::drawBoundaries() |
| 推理引擎依赖         | ExportService/AutoCompleteService 直接依赖 HFA/RMVPE/GAME 具体类型 | ARCH-06：引入 IInferenceService 接口 + CompositeInferenceService 组合模式，消除具体引擎依赖 |
| 推理命名空间         | IInferenceEngine 在 dstools 命名空间     | 迁移到 dsfw::infer 命名空间，框架层接口不应在应用命名空间 |
| PitchLabelerPage 封装 | rmvpeRef()/gameRef() 返回引用，外部可修改私有成员 | 删除引用访问器，添加 acquireEngines() 封装获取 + rmvpe()/game() 只读访问器 |
| 路径控件记忆         | PathSelector/FilePathSelector 各自独立实现最近路径记忆 | 提取 RecentPathStore 共享类，统一存储组名 "PathSelector" |
| SlicerPage 原子写入 | 设计文档称 AudioFileWriter 内部已原子写入，rename 冗余 | 实际 AudioFileWriter 无原子写入，tmp+rename 模式必要；提取 writeSliceAtomically 辅助函数消除重复 |

---