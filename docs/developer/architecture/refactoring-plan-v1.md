# Dataset Tools 大型重构方案 v1

> 版本 1.0.0 | 2026-06-01
>
> 本文档是 dataset-tools 项目的综合重构方案。目标：抛弃一切技术与架构债，建立统一的底层接口体系，同时保证现有功能完全不变、行为一致、数据安全稳定。
>
> 本方案严格遵循 [human-decisions.md](../../human-decisions.md) 的设计准则体系（ARCH/CONCUR/ROBUST/INFRA/VIEW），流程适应 [pipeline.md](data-flow/pipeline.md) 标注流水线文档。

---

## 1 项目现状

### 1.1 技术栈

| 层级 | 技术 | 版本要求 |
|------|------|----------|
| 语言 | C++20 | MSVC 2022 / GCC / Clang |
| GUI | Qt 6 | 6.8+, Core/Gui/Widgets/Svg/Network |
| 构建 | CMake | 3.21+ |
| 包管理 | vcpkg | nlohmann_json, QWindowKit |
| 推理 | ONNX Runtime | — |

### 1.2 六层架构

```
Layer 5  dstools-infer-common   OnnxEnv, OnnxModelBase, IInferenceEngine
Layer 4  dsfw-widgets           通用 UI 组件 (SHARED DLL)
Layer 3  dsfw-ui-core           AppShell, IconNavBar, Theme, FramelessHelper, IPageActions
Layer 2  dstools-audio          AudioDecoder (FFmpeg), AudioPlayback (SDL2)
Layer 2.5 dsfw-signal           F0Curve, CurveTools (静态库)
Layer 1  dsfw-core              AppSettings, ServiceLocator, AsyncTask, JsonHelper
Layer 0.5 dsfw-base             PathUtils, JsonHelper (Qt-free 静态库)
Layer 0  dsfw-types             Result<T>, ExecutionProvider, TimePos (header-only)
```

### 1.3 应用清单

| 应用 | 类型 | 说明 |
|------|------|------|
| ds-labeler | GUI | DsLabeler 主应用（工程管理、标注流水线） |
| label-suite | GUI | LabelSuite（独立编辑器集合） |
| cli | CLI | 命令行工具 |
| hfa-cli | CLI | Hubert Forced Aligner 命令行 |
| test-shell | CLI | 推理引擎测试外壳 |
| widget-gallery | GUI | UI 控件展示 |

### 1.4 已完备的基础设施

项目中已正确实现并稳定运行的基础设施，重构中不应修改：

| 基础设施 | 状态 | 位置 |
|----------|------|------|
| PipelineContext + dirty 传播 | 已实现 | dsfw-core |
| PipelineRunner 流水线执行 | 已实现 | dsfw-core |
| TaskProcessorRegistry | 已实现 | dsfw-core |
| FormatAdapterRegistry | 已实现 | dsfw-core |
| EditorPageBase 生命周期 | 已实现 | apps/shared/data-sources |
| AudioVisualizerContainer 统一视图 | 已实现 | apps/shared/audio-visualizer |
| ViewportController 统一缩放 | 已实现 | apps/shared/chart-framework |
| BoundaryDragController 集中拖拽 | 已实现 | apps/shared/chart-framework |
| InferBridge 引擎桥接 | 已实现 | libs/infer-bridge |
| DsTextDocBridge 三向转换 | 已实现 | apps/shared/bridges |
| ServiceLocator（全局服务） | 已实现 | dsfw-core |
| SettingsKey<T> 强类型 | 已实现 | dsfw-core |
| AtomicFileWriter 原子写入 | 已实现 | dsfw-core |

---

## 2 重构原则

### 2.1 必须遵守的核心约束

以下规则在任何重构方案中不可违反，摘自 [human-decisions.md](../../human-decisions.md)：

| 编号 | 约束 | 重构中的影响 |
|------|------|-------------|
| ARCH-01 | 相同行为只存在一处 | 所有重复代码必须下沉到共享层 |
| ARCH-06 | 依赖倒置，构造注入优先 | 新增模块必须面向接口，通过构造函数注入依赖 |
| ARCH-07 | 开闭原则 | 新增功能通过新增类，不修改稳定核心 |
| ARCH-08 | 适配器隔离文件格式 | 任何文件 I/O 必须通过 IFormatAdapter |
| CONCUR-01 | >50ms 操作异步 | 所有推理、文件读写必须异步 |
| CONCUR-03 | 共享状态锁保护 | 重构后的所有共享数据结构考虑线程安全 |
| ROBUST-02 | Result<T> 传播错误 | 禁止在业务代码中 throw |
| INFRA-01 | ServiceLocator 仅全局服务 | 页面资源绝不注册到 ServiceLocator |
| INFRA-05 | PathUtils 统一路径库 | 禁止 path.string()，统一 PathUtils::toUtf8() |
| INFRA-06 | RAII 资源管理 | 禁止裸 new/delete，禁用手动 lock/unlock |
| INFRA-08 | SettingsKey<T> 集中定义 | 新配置键必须在 Keys.h 中定义 |
| INFRA-09 | 配置存用户目录 | 不存 .dsproj |
| INFRA-13 | PIMPL 隔离第三方头文件 | 公共接口包含第三方头文件时必须 PIMPL |
| INFRA-14 | 接口版本化 | 所有接口声明 kInterfaceVersion |
| INFRA-16 | 不可变配置快照 | 流水线执行期间配置不可变 |

### 2.2 补充设计准则

以下准则来自 C++ 核心指南 (C++ Core Guidelines)、Qt 最佳实践、KDE Frameworks、LLVM 编码标准等优秀 C++/Qt 项目的经验总结：

#### CCG-01：类型安全与强类型

**原则**：使用强类型封装原始值，避免"原始类型偏执"(Primitive Obsession)。时间用 `TimePos` 而非 `int64_t`，频率用 `Frequency`（Hz）而非 `double`，避免隐式转换导致的语义错误。

**来源**：C++ Core Guidelines (I.4: Make interfaces precisely and strongly typed)、Qt 6 的 QLatin1StringView 实践。

**实施**：
- 引入 `Seconds`、`Milliseconds`、`Microseconds` 强类型 duration，使用 `std::chrono::duration` 或自定义字面量
- 引入 `SampleRate` 强类型，替换裸 `int`
- 禁止非显式构造函数，使用 `explicit` 关键字

#### CCG-02：编译期检查优先于运行时检查

**原则**：能用 `static_assert`、`consteval`、模板约束解决的检查，不推迟到运行时。利用 C++20 concepts 约束模板参数。

**来源**：C++ Core Guidelines (P.4: Ideally, a program should be statically type safe)、LLVM Coding Standards。

**实施**：
- `SettingsKey<T>` 使用 `static_assert` 确保 T 为支持的类型
- 路径拼接函数使用 `consteval` 检查格式合法性（如未来 C++23/26）
- 引擎类型特性使用 concepts 约束

#### CCG-03：值语义优先于引用语义

**原则**：数据结构优先设计为值类型（可复制、可移动），而非强制使用指针/引用传递。与 Qt 的隐式共享 (Implicit Sharing) 和 COW (Copy-on-Write) 策略一致。

**来源**：C++ Core Guidelines (C.10: Prefer concrete types over class hierarchies)、Qt Implicit Sharing 文档。

**实施**：
- `DsTextDocument`、`DSFile` 等内部模型提供值语义（复制构造、移动构造）
- `PipelineContext` 提供快照复制能力（配合 INFRA-16）
- 大对象（如音频数据）使用 `QSharedData` + COW 模式

#### CCG-04：不可变数据优先

**原则**：默认使用 `const`，函数参数优先 `const &`，成员变量仅在必要时设为可变。不可变对象天然线程安全，简化并发编程。

**来源**：C++ Core Guidelines (Con.1: By default, make objects immutable)、函数式编程原则。

**实施**：
- 所有 getter 方法标记 `const`
- 配置快照类所有成员为 `const`
- 数据传输对象（DTO）使用不可变设计

#### CCG-05：错误处理分层明确

**原则**：区分四类错误——(1) 编程错误 (assert/contract)、(2) 可恢复错误 (Result<T>)、(3) 不可恢复错误 (crash)、(4) 外部异常 (try-catch 边界)。每一层明确使用哪种错误处理策略。

**来源**：C++ Core Guidelines (E: Error handling)、LLVM Error Handling Guide。

**实施**：
- `assert` 用于内部不变量检查（仅在 Debug 构建启用）
- `Result<T>` 用于可恢复的业务错误
- `std::terminate` 用于不可恢复的致命错误
- `try-catch` 仅用于第三方库边界，包裹后转 Result<T>

#### CCG-06：资源获取即初始化 (RAII) 全覆盖

**原则**：所有资源——不仅仅是内存——都应在构造函数中获取、析构函数中释放。包括文件句柄、锁、推理引擎句柄、GPU 内存、网络连接、定时器。

**来源**：C++ Core Guidelines (R: Resource management)、Qt 的父子对象模型。

**实施**：
- 自定义 RAII 包装类用于 ONNX Runtime 的 Ort::Session（如已存在则检查完整性）
- 锁包装：`std::lock_guard`、`std::scoped_lock`、`std::shared_lock`
- GPU 内存：包装 ONNX 的 `Ort::MemoryInfo` + `Ort::Value` 为一个 RAII 类
- 文件句柄：`std::ifstream`/`std::ofstream`（已有 RAII），不做额外包装

#### CCG-07：接口隔离原则 (ISP)

**原则**：接口应小而专注。一个类不应被迫实现它不使用的方法。大接口应拆分为多个小接口。

**来源**：SOLID 原则 (Interface Segregation Principle)、Qt 的模块化设计。

**实施**：
- `IEditorDataSource` 拆分为 `ISliceLoader`、`ISliceSaver`、`IAudioPathProvider` 三个小接口（如 EditorPage 只需要读取切片不需要保存时）
- `ITaskProcessor` 保持当前简洁设计（仅 `process`、`validate`、`name`、`processorId`）
- 新增接口前必须满足 INFRA-02（至少两个预期实现）

#### CCG-08：Noexcept 规范

**原则**：所有不会抛出异常的函数标记 `noexcept`，特别是析构函数、移动构造函数、swap 函数。这使编译器能生成更优的代码，也是标准库容器高效运作的前提。

**来源**：C++ Core Guidelines (F.16: Use noexcept when possible)、ISO C++ 建议。

**实施**：
- 所有析构函数标记 `noexcept`（除非包含可能抛异常的操作）
- 所有移动构造函数和移动赋值运算符标记 `noexcept`
- getter/简单查询方法标记 `noexcept`
- 接口的虚析构函数标记 `noexcept`

#### CCG-09：单元测试作为契约

**原则**：每个公共 API 至少有一个单元测试。测试不仅验证正确性，也是 API 的使用文档。遵循 given-when-then 结构。

**来源**：C++ Core Guidelines (T: Testing)、Google Test 最佳实践。

**实施**：
- 新增的每个公共接口类必须有对应的单元测试
- 使用 Qt Test 框架（已有 test-design.md 定义三层测试体系）
- 测试使用自包含的 fixture 数据（INFRA-17）
- CI 中运行全量单元测试

#### CCG-10：日志分层与可观测性

**原则**：日志分为 ERROR、WARN、INFO、DEBUG、TRACE 五层，生产环境默认 INFO 级别。关键业务操作（推理、导出、文件 I/O 失败）记录 INFO+ 日志，性能热点记录 DEBUG 日志。日志消息包含上下文（sliceId、projectName），便于问题定位。

**来源**：Google Logging Guidelines、KDE Frameworks 日志框架。

**实施**：
- 统一使用 dsfw::Log（如已存在）或引入结构化日志
- 日志消息包含关键上下文：`"[{page}] [{sliceId}] message"`
- 错误日志包含完整错误链：从根因到上层调用
- 避免在热路径（每帧渲染）中记录 INFO+ 日志

---

## 3 技术债清单

### 3.1 架构债

| 编号 | 问题 | 严重度 | 影响范围 |
|------|------|--------|----------|
| AD-01 | DSFile 与 DsTextDocument 双模型无自动桥接 | 中 | PitchLabelerPage 中手动做三向映射，分散多处易出错 |
| AD-02 | DsDocument 与 DsTextDocument 概念重叠 | 中 | DsDocument 存储 .ds 工程数据，DsTextDocument 存储切片数据，两者间的转换逻辑分散 |
| AD-03 | label-suite 和 ds-labeler 的页面工厂逻辑耦合 | 中 | PageFactory 需要了解两个应用的所有页面类型 |
| AD-04 | hfa-cli / test-shell 作为独立 app 与主 CLI 的功能边界模糊 | 低 | hfa-cli 本质上只是 CLI 的 HFA 子命令 |

### 3.2 基础设施债

| 编号 | 问题 | 严重度 | 影响范围 |
|------|------|--------|----------|
| ID-01 | 配置键定义分散在 4 个文件中 | 中 | AppSettingKeys.h, SharedSettingsKeys.h, PitchLabelerKeys.h, CommonKeys.h |
| ID-02 | 文件路径选择无统一控件 | 高 | PathSelector 存在但与各页面文件打开对话框行为不一致 |
| ID-03 | 编码转换存在重复或潜在不一致 | 中 | Encoding.h 与 PathUtils.h 的职责边界模糊 |
| ID-04 | DsTextTypes.h 版本号硬编码为 "3.0.0" | 低 | 应使用 constexpr 集中定义 |
| ID-05 | magic-numbers-audit.md 中 21 个魔法数字未迁移到配置系统 | 中 | 频谱图/功率图/波形图/钢琴卷帘渲染参数硬编码 |

### 3.3 代码质量债

| 编号 | 问题 | 严重度 | 影响范围 |
|------|------|--------|----------|
| CD-01 | IBoundaryModel 缺少 kInterfaceVersion | 低 | 接口版本管理不一致 |
| CD-02 | IEditorDataSource 缺少 kInterfaceVersion | 低 | 同上 |
| CD-03 | CCG-08：部分移动构造函数未标记 noexcept | 低 | 性能优化机会 |
| CD-04 | 测试覆盖率不足 | 中 | 核心接口缺少单元测试 |

### 3.4 已修复的历史债（供参考，勿重复处理）

以下已在之前版本中修复，记录于此以避免重复工作：

- IFileIOProvider / ISettingsBackend / ISlicerService 冗余接口（已删除）
- BoundaryBindingManager（已由 BoundaryDragController 取代）
- PPS/pixelsPerSecond（已由 resolution 取代）
- kResolutionTable 固定列表（已由连续范围 + 对数步进表取代）
- .dsproj 中 defaults 字段（已迁移到 AppSettings）
- processEvents() 调用（已全部移除）
- path.string() 调用（已全部替换为 PathUtils）
- ServiceLocator 误用（已限定为全局服务）
- ISettingsBackend 抽象层（已删除，直接使用具体类）

---

## 4 重构阶段规划

### 阶段 0：统一底层接口 (Platform Foundation)

**目标**：建立项目统一底层，所有路径操作、编码转换、文件选择、平台抽象收敛到固定模块。

#### 0.1 路径与编码统一模块

**当前状态**：`dsfw::PathUtils`（位于 `src/framework/base/`）已提供核心路径操作。`Encoding.h` 提供编码转换。两者功能有重叠。

**重构内容**：

1. **合并 Encoding.h 到 PathUtils**：将 `Encoding.h` 中的编码转换函数整合到 `PathUtils` 中，明确 PathUtils 是唯一的"路径 + 编码"统一模块。详见 [§5.1](#51-路径与编码统一模块设计)。

2. **引入编码检测**：`PathUtils::detectEncoding(const std::filesystem::path &)` 返回路径的实际编码，用于处理混合编码场景。

3. **全量审查**：确保项目中所有模块通过 PathUtils 处理路径和编码，不存在绕过的路径操作。

4. **添加 PathUtils 单元测试**：覆盖 Windows ANSI/UTF-8、macOS NFD/NFC、Linux UTF-8 路径编码场景。使用 CCG-09 的 given-when-then 结构。

#### 0.2 文件路径选择统一控件

**当前状态**：各页面各自使用 `QFileDialog::getOpenFileName()` 或 `QFileDialog::getExistingDirectory()`，行为不一致（初始路径、过滤器、标题等）。

**重构内容**：

1. **设计 FilePathSelector 统一控件**：封装 `QFileDialog`，提供统一行为。详见 [§5.2](#52-文件路径选择统一控件设计)。

2. **迁移所有页面**：将所有页面的 `QFileDialog` 调用替换为 `FilePathSelector`。

3. **支持最近路径记忆**：自动记忆每种选择类型的最近路径，存储到 AppSettings。

#### 0.3 配置键集中管理

**当前状态**：`AppSettingKeys.h`、`SharedSettingsKeys.h`、`PitchLabelerKeys.h`、`CommonKeys.h` 四个文件。

**重构内容**：

1. **合并为单一 Keys.h**：将所有配置键定义合并到 `src/apps/shared/settings/Keys.h`。

2. **分类组织**：使用嵌套 namespace 按领域分类（如 `Keys::Audio`、`Keys::View`、`Keys::Model`）。

3. **SSOT 原则**：每个配置键只在 Keys.h 中定义一次。

---

### 阶段 1：架构清理 (Architecture Cleanup)

**目标**：消除架构债，统一数据模型，简化模块关系。

#### 1.1 统一内部文档模型 (AD-01, AD-02)

**当前状态**：`DSFile`（PitchEditor 专用）和 `DsTextDocument`（通用切片标注）是两套并行的数据模型。`DsTextDocBridge` 提供了转换桥接，但转换逻辑分散。

**重构内容**：

1. **扩展 DsTextDocument 为唯一数据源**：将 `DSFile` 的功能映射到 `DsTextDocument` 的层结构中，使 PitchEditor 也基于 `DsTextDocument` 工作。`DSFile` 保留为 DsTextDocument 的视图/适配器，而非独立模型。

2. **强化 DsTextDocBridge**：确保所有 DSFile <-> DsTextDocument <-> TextGridDocument 的转换通过 `DsTextDocBridge` 统一完成，消除各处分散的手动映射代码。

3. **版本管理集中化**：`DsTextDocument::version` 从硬编码 "3.0.0" 改为 `constexpr` 集中定义在 `DsTextTypes.h` 中。

#### 1.2 模块边界整理 (AD-03, AD-04)

1. **hfa-cli 合并到 CLI**：将 `hfa-cli` 的功能作为 CLI 的子命令 `dataset-tools hfa`，消除独立应用。

2. **test-shell 合并到 CLI**：将 `test-shell` 的功能作为 CLI 的子命令 `dataset-tools test`。

3. **PageFactory 解耦**：引入 `IPageDescriptor` 接口，各应用独立注册页面，PageFactory 仅做通用调度。

#### 1.3 接口审查与补充

1. **为 IBoundaryModel 添加 kInterfaceVersion**（CD-01）
2. **为 IEditorDataSource 添加 kInterfaceVersion**（CD-02）
3. **审查所有接口是否满足 INFRA-02**（接口必要性）
4. **为公共接口的移动构造函数/赋值运算符添加 noexcept**（CCG-08）

---

### 阶段 2：技术债清偿 (Technical Debt Resolution)

**目标**：消除所有代码质量债和基础设施债。

#### 2.1 魔法数字配置化 (ID-05)

**当前状态**：`magic-numbers-audit.md` 识别了 23 个硬编码参数，2 个已迁移。

**重构内容**：

1. **建立 ChartConfig 体系**：`ChartConfig` 结构体，包含所有图表渲染参数（频谱图颜色映射、FFT 窗大小、波形图振幅范围等）。
2. **在 Keys.h 中定义 ChartConfig 的 SettingsKey**。
3. **AppSettings 中读写 ChartConfig**。
4. **迁移现有硬编码参数**，按优先级：
   - 高频：频谱图渲染 (5 个)、功率图渲染 (3 个)、音高提取 (5 个)
   - 中频：波形图渲染 (3 个)、钢琴卷帘编辑器 (10 个)
   - 低频：通用常量 (3 个)

#### 2.2 测试体系补全 (CD-04)

1. **核心接口单元测试**：`IFormatAdapter`、`ITaskProcessor`、`PipelineRunner` 的 mock 测试
2. **PathUtils 编码测试**：Win/Mac/Linux 路径编码场景
3. **DsTextDocument 序列化测试**：往返测试 (round-trip)
4. **PipelineContext dirty 传播测试**：验证 DAG 传播算法
5. **Result<T> 传播链测试**：验证错误不被吞掉

---

### 阶段 3：稳定性与安全性加固 (Stability & Security)

**目标**：确保工程数据在实际使用中的稳定性和安全性，防止数据丢失和损坏。

#### 3.1 数据完整性

1. **原子写入全覆盖**：所有文件写入（.dsproj、.dstext、PipelineContext JSON）使用 `AtomicFileWriter`（写入临时文件 + 原子重命名）。审查现有代码，确保无遗漏。

2. **写入前校验**：`DsTextDocument::save()` 写入前进行数据完整性校验（JSON 结构完整性、必需字段存在等）。

3. **自动备份**：工程文件（.dsproj）在写入前自动备份到 `dstemp/backups/`，保留最近 5 个版本。

#### 3.2 数据隔离

1. **临时文件隔离**：所有中间文件写入 `dstemp/`，不污染工程目录。
2. **模型输出隔离**：推理引擎的输出文件写入独立子目录（如 `dstemp/alignment/`），不覆盖用户标注文件。
3. **多实例保护**：`SingleInstanceGuard` 确保同一工程不被多个实例同时打开。

#### 3.3 错误恢复

1. **工程文件损坏检测**：`DsProject::load()` 在解析失败时尝试从备份恢复。
2. **切片数据损坏检测**：`DsTextDocument::load()` 解析失败时报告具体错误位置（行号、字段名），帮助用户手动修复。
3. **部分数据加载**：工程中部分切片加载失败时，允许加载成功的切片继续工作，在切片列表中标记失败项（灰色 + 错误 tooltip）。

#### 3.4 安全审计

1. **路径遍历防护**：确保所有文件路径在工程目录内，防止 `../` 逃逸攻击。
2. **JSON 注入防护**：确保导出文件名不包含特殊字符导致 JSON 注入。
3. **敏感信息保护**：配置文件中不存储绝对路径中的用户名信息（使用相对路径或环境变量）。

---

### 阶段 4：代码质量提升 (Code Quality)

**目标**：全面应用补充准则，提升整体代码质量。

#### 4.1 Noexcept 规范化 (CCG-08)

1. 审查所有析构函数，标记 `noexcept`
2. 审查所有移动构造函数/赋值运算符，标记 `noexcept`
3. 审查所有 getter/简单查询方法，标记 `noexcept`

#### 4.2 强类型引入 (CCG-01)

1. 引入 `Seconds`、`Milliseconds`、`Microseconds` 类型别名或 wrapper
2. 引入 `SampleRate` 强类型
3. 为 `explicit` 的单参数构造函数标记 `explicit`

#### 4.3 Const 正确性 (CCG-04)

1. 审查所有成员函数，getter 标记 `const`
2. 局部变量优先标记 `const`
3. 函数参数优先 `const &`

#### 4.4 日志规范化 (CCG-10)

1. 统一日志格式：`"[{module}] [{context}] message"`
2. 分级使用日志：ERROR（用户可见错误）、WARN（非致命异常）、INFO（关键业务操作）、DEBUG（性能热点）、TRACE（函数调用跟踪）
3. 移除 `qDebug()` 调用，统一为 `dsfw::Log`（或项目统一的日志宏）

---

## 5 统一底层接口设计

### 5.1 路径与编码统一模块设计

**目标**：`dsfw::PathUtils` 成为项目中唯一的路径与编码处理入口。

#### 5.1.1 模块位置

`src/framework/base/include/dsfw/PathUtils.h`

#### 5.1.2 接口定义

```cpp
#pragma once

#include <filesystem>
#include <string>
#include <fstream>
#include <optional>
#include <QString>

namespace dsfw {

class PathUtils {
public:
    PathUtils() = delete;

    static std::filesystem::path toStdPath(const QString &path) noexcept;
    static QString fromStdPath(const std::filesystem::path &path) noexcept;

    static std::string toUtf8(const std::filesystem::path &path);
    static std::wstring toWide(const std::filesystem::path &path);
    static QString toQString(const std::filesystem::path &path);

    static std::filesystem::path normalize(const std::filesystem::path &path);
    static std::filesystem::path join(const std::filesystem::path &base,
                                      const std::filesystem::path &child);

    static std::unique_ptr<std::ifstream> openFileForRead(const std::filesystem::path &path);
    static std::unique_ptr<std::ofstream> openFileForWrite(const std::filesystem::path &path);

    static bool isSubPath(const std::filesystem::path &parent,
                          const std::filesystem::path &child) noexcept;
    static std::filesystem::path relativeTo(const std::filesystem::path &path,
                                             const std::filesystem::path &base);
    static std::optional<std::filesystem::path> canonicalOrNull(
        const std::filesystem::path &path) noexcept;

    enum class Encoding { Utf8, Ansi, Unknown };
    static Encoding detectEncoding(const std::filesystem::path &path);

    static QString toNativeSeparators(const QString &path) noexcept;
    static QString toPosixSeparators(const QString &path) noexcept;
};

} // namespace dsfw
```

#### 5.1.3 设计说明

- **单例禁止**：`PathUtils` 是无状态的静态方法集合，使用 `= delete` 禁止构造。
- **noexcept**：不会失败的操作（`toStdPath`、`normalize`、`join`）标记 `noexcept`。
- **跨平台**：`openFileForRead`/`openFileForWrite` 内部使用 `_wfopen`（Windows）或 `fopen`（Unix），解决 Windows 中文路径编码问题。
- **整合 Encoding.h**：将 `Encoding` 枚举和相关方法整合到 `PathUtils` 中。

#### 5.1.4 与 Encoding.h 的整合

`Encoding.h` 当前提供：
- `fromUtf8()` / `toUtf8()` — QString <-> std::string 转换
- `fromWide()` / `toWide()` — QString <-> std::wstring 转换
- `fromLocal8Bit()` / `toLocal8Bit()` — QString <-> 本地编码转换

整合策略：
- QString <-> std::string/wstring 的转换合并到 PathUtils
- 本地编码相关功能保留在 Encoding.h 中（独立关注点）
- PathUtils 内部使用 Encoding 的函数

#### 5.1.5 单元测试

```cpp
// 测试用例示例（使用 CCG-09 given-when-then 结构）
void testToStdPathChinese() {
    // Given
    QString chinesePath = QStringLiteral("C:/测试目录/文件.txt");

    // When
    auto path = PathUtils::toStdPath(chinesePath);

    // Then
    QCOMPARE(PathUtils::toUtf8(path), "C:/测试目录/文件.txt");
}

void testNormalizeNFC() {
    // Given: macOS NFD 路径
    QString nfdPath = QString::fromUtf8("Cafe\u0301.txt");  // e + combining acute

    // When
    auto normalized = PathUtils::normalize(PathUtils::toStdPath(nfdPath));

    // Then: NFC 规范化
    QCOMPARE(PathUtils::toUtf8(normalized), QString::fromUtf8("Caf\u00e9.txt").toStdString());
}
```

### 5.2 文件路径选择统一控件设计

**目标**：所有页面的文件/目录选择使用统一的 `FilePathSelector`，行为一致。

#### 5.2.1 控件位置

`src/framework/widgets/include/dsfw/widgets/FilePathSelector.h`

#### 5.2.2 接口定义

```cpp
#pragma once

#include <QWidget>
#include <QString>
#include <QStringList>
#include <QFileDialog>

namespace dsfw::widgets {

class FilePathSelector : public QObject {
    Q_OBJECT
public:
    enum class Mode {
        OpenFile,
        OpenFiles,
        SaveFile,
        OpenDirectory
    };

    struct Config {
        Mode mode = Mode::OpenFile;
        QString title;
        QStringList nameFilters;
        QString defaultSuffix;
        QString settingsKey;
        QString initialPath;
    };

    explicit FilePathSelector(const Config &config, QWidget *parent = nullptr);

    QString selectedPath() const;
    QStringList selectedPaths() const;

    QString exec();

signals:
    void pathSelected(const QString &path);
    void pathsSelected(const QStringList &paths);

private:
    Config m_config;
    QString m_selectedPath;
    QStringList m_selectedPaths;

    QString loadRecentPath() const;
    void saveRecentPath(const QString &path);
};

} // namespace dsfw::widgets
```

#### 5.2.3 设计说明

- **统一入口**：所有文件/目录选择通过 `FilePathSelector` 进行，禁止直接使用 `QFileDialog`。
- **最近路径记忆**：每种选择类型（通过 `settingsKey` 区分）自动记忆最近使用的路径到 `AppSettings`。
- **配置驱动**：`Config` 结构体提供所有选择参数，保持不可变传递（CCG-04）。
- **可测试**：信号通知选择结果，便于单元测试中 mock。

#### 5.2.4 使用示例

```cpp
// 替换前
QString path = QFileDialog::getOpenFileName(this, "选择音频文件", lastPath, "*.wav *.flac");

// 替换后
FilePathSelector::Config cfg;
cfg.mode = FilePathSelector::Mode::OpenFile;
cfg.title = "选择音频文件";
cfg.nameFilters = {"音频文件 (*.wav *.flac)"};
cfg.settingsKey = "AudioFileSelector/lastPath";

auto *selector = new FilePathSelector(cfg, this);
QString path = selector->exec();
```

### 5.3 消息提示统一控件设计

**目标**：统一所有 Toast/Notification/错误弹窗的行为和样式。

#### 5.3.1 控件位置

`src/framework/widgets/include/dsfw/widgets/MessageHelper.h`

#### 5.3.2 接口定义

```cpp
#pragma once

#include <QWidget>
#include <QString>

namespace dsfw::widgets {

class MessageHelper {
public:
    MessageHelper() = delete;

    enum class Level { Info, Warning, Error };

    static void toast(QWidget *parent, const QString &message,
                      Level level = Level::Info,
                      int durationMs = 3000);

    static bool confirm(QWidget *parent, const QString &title,
                        const QString &message);

    static void error(QWidget *parent, const QString &title,
                      const QString &message);
};

} // namespace dsfw::widgets
```

#### 5.3.3 设计说明

- **统一外观**：所有 Toast 使用相同的动画/位置/时长。
- **层级控制**：Info（灰色）、Warning（橙色）、Error（红色），视觉一致性。
- **禁止 QMessageBox 裸用**：所有弹窗/通知通过 `MessageHelper` 进行。

---

## 6 迁移策略

### 6.1 核心原则

1. **逐阶段迁移**：按阶段 0 -> 1 -> 2 -> 3 -> 4 顺序执行，每阶段完成后进行回归测试。
2. **增量提交**：每完成一个子任务独立提交（不推送），便于回滚和审查。
3. **行为不变**：每个阶段结束时，所有现有功能的行为必须与重构前完全一致。
4. **编译即运行**：每次修改后立即编译，确保无编译错误后再继续。

### 6.2 风险控制

| 风险 | 概率 | 影响 | 缓解措施 |
|------|------|------|----------|
| 路径编码迁移导致文件读写出错 | 中 | 高 | PathUtils 迁移前先写单元测试覆盖所有编码场景 |
| DSFile -> DsTextDocument 迁移丢失数据 | 中 | 高 | 先实现 DsTextDocBridge 的完整 round-trip 测试 |
| QFileDialog 替换导致选择行为变化 | 低 | 中 | FilePathSelector 内部仍用 QFileDialog，仅统一参数 |
| 接口版本化导致编译错误 | 低 | 低 | 引用 kInterfaceVersion 的代码逐一更新 |

### 6.3 验证清单

每个阶段完成后，执行以下验证：

- [ ] `cmake --build --preset release` 编译通过，无 warning
- [ ] `ctest` 全量测试通过
- [ ] DsLabeler 启动 -> 新建工程 -> 导入音频 -> 标注全流程
- [ ] LabelSuite 启动 -> 打开文件 -> 编辑器全流程
- [ ] CLI 基本命令可用
- [ ] 中文路径、日文路径读写正常
- [ ] 跨平台：Windows 编译运行正常

---

## 7 文档更新计划

### 7.1 重构期间需要更新的文档

| 文档 | 更新内容 | 阶段 |
|------|---------|------|
| human-decisions.md | 补充 CCG-01~10 到速查表 | 阶段 0 |
| overview.md | 更新架构图（统一路径模块、FilePathSelector） | 阶段 0 |
| conventions.md | 补充 noexcept/强类型/const 正确性规范 | 阶段 4 |
| component-design.md | 更新 DSFile/DsTextDocument 关系描述 | 阶段 1 |
| pipeline.md | 如有 pipeline 变更则更新（预期无变更） | 阶段 1 |
| magic-numbers-audit.md | 标记已迁移的魔法数字为"已完成" | 阶段 2 |
| test-design.md | 补充 CCG-09 (given-when-then) 结构规范 | 阶段 2 |

### 7.2 重构完成后新增的文档

| 文档 | 说明 |
|------|------|
| docs/developer/architecture/platform/FilePathSelector.md | 文件路径选择控件设计文档 |
| docs/developer/architecture/platform/PathUtils.md | 路径编码模块设计文档 |
| docs/developer/architecture/platform/MessageHelper.md | 消息提示控件设计文档 |

---

## 8 关联文档

- [human-decisions.md](../../human-decisions.md) — 设计准则与决策
- [overview.md](overview.md) — 框架六层架构
- [pipeline.md](data-flow/pipeline.md) — 标注流水线定义
- [ds-format.md](data-flow/ds-format.md) — 工程格式规范
- [dirty-mechanism.md](framework/dirty-mechanism.md) — 标脏机制设计
- [component-design.md](data-flow/component-design.md) — 核心组件设计
- [unified-app-design.md](framework/unified-app-design.md) — 统一应用设计
- [conventions.md](../../guides/conventions.md) — 编码规范
- [test-design.md](../../testing/test-design.md) — 测试设计
- [magic-numbers-audit.md](../../magic-numbers-audit.md) — 魔法数字审计