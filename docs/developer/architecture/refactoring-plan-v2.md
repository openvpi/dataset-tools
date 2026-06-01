# Dataset Tools 大型重构方案 v2

> 版本 2.0.0 | 2026-06-02
>
> 本文档是 dataset-tools 项目的综合重构方案 v2，在 v1 基础上基于全面代码探索和设计文档一致性核对后制定。
> 目标：抛弃一切技术与架构债，建立统一的底层接口体系，同时保证现有功能完全不变、行为一致、数据安全稳定。
>
> 本方案严格遵循 [human-decisions.md](../../human-decisions.md) 的设计准则体系（ARCH/CONCUR/ROBUST/INFRA/VIEW），
> 流程适应 [data-flow-design.md](data-flow/data-flow-design.md) 完整数据流设计。

---

## 1 项目现状

### 1.1 技术栈

| 层级  | 技术           | 版本要求                               |
|-----|--------------|------------------------------------|
| 语言  | C++20        | MSVC 2022 / GCC / Clang            |
| GUI | Qt 6         | 6.8+, Core/Gui/Widgets/Svg/Network |
| 构建  | CMake        | 3.21+                              |
| 包管理 | vcpkg        | nlohmann_json, QWindowKit          |
| 推理  | ONNX Runtime | --                                 |

### 1.2 六层架构

```
Layer 5  dstools-infer-common   OnnxEnv, OnnxModelBase, IInferenceEngine
Layer 4  dsfw-widgets           通用 UI 组件 (SHARED DLL)
Layer 3  dsfw-ui-core           AppShell, IconNavBar, Theme, FramelessHelper, IPageActions
Layer 2  dstools-audio          AudioDecoder (FFmpeg), AudioPlayback (SDL2)
Layer 2.5 dsfw-signal           F0Curve, CurveTools (静态库)
Layer 1  dsfw-core              AppSettings, ServiceLocator, AsyncTask, PathUtils, Encoding
                                PipelineContext, PipelineRunner, ITaskProcessor, 接口集
Layer 0.5 dsfw-base             JsonHelper (Qt-free 静态库)
Layer 0  dsfw-types             Result<T>, ExecutionProvider, TimePos (header-only)
```

### 1.3 应用清单

| 应用             | 类型  | 说明                        |
|----------------|-----|---------------------------|
| ds-labeler     | GUI | DsLabeler 主应用（工程管理、标注流水线） |
| label-suite    | GUI | LabelSuite（独立编辑器集合）       |
| cli            | CLI | 命令行工具                     |
| hfa-cli        | CLI | Hubert Forced Aligner 命令行 |
| test-shell     | CLI | 推理引擎测试外壳                  |
| widget-gallery | GUI | UI 控件展示                   |

### 1.4 已完成的先行阶段

| 阶段            | 完成内容                                                                                                            | 完成时间    |
|---------------|-----------------------------------------------------------------------------------------------------------------|---------|
| **Phase S**   | 可视化架构统一：AudioVisualizerContainer、MiniMapScrollBar、BoundaryOverlayWidget、ViewportController、TierLabelArea、键鼠交互统一 | 2026-05 |
| **Phase R.1** | 配置系统重构：统一到 AppSettings，删除 ProjectSettingsBackend，废弃 ISettingsBackend                                            | 2026-05 |
| **Phase L**   | 格式适配器基础设施：TextGridAdapter、DsFileAdapter、CsvAdapter、LabAdapter + FormatAdapterRegistry                           | 2026-05 |

### 1.5 已完备的基础设施

| 基础设施                                  | 位置                            |
|---------------------------------------|-------------------------------|
| AudioVisualizerContainer 统一视图         | apps/shared/audio-visualizer  |
| ViewportController 统一缩放               | src/framework/widgets/        |
| BoundaryDragController 集中拖拽           | apps/shared/chart-framework   |
| MiniMapScrollBar 缩略图                  | apps/shared/audio-visualizer  |
| TierLabelArea 标签层次结构                  | apps/shared/phoneme-editor/ui |
| AppSettings 统一配置 + SettingsKey<T> 强类型 | dsfw-core                     |
| FormatAdapterRegistry 注册机制            | dsfw-core                     |
| PipelineContext + dirty 传播            | dsfw-core                     |
| PipelineRunner 流水线执行                  | dsfw-core                     |
| TaskProcessorRegistry                 | dsfw-core                     |
| EditorPageBase 生命周期                   | apps/shared/data-sources      |
| InferBridge 引擎桥接                      | libs/infer-bridge             |
| DsTextDocBridge 三向转换                  | apps/shared/bridges           |
| ServiceLocator（全局服务）                  | dsfw-core                     |
| AtomicFileWriter 原子写入                 | dsfw-core                     |
| Dirty 机制完整实现                          | dsfw-core + apps              |

---

## 2 重构原则

### 2.1 核心约束（摘自 human-decisions.md）

| 编号        | 约束                   | 重构中的影响                                  |
|-----------|----------------------|-----------------------------------------|
| ARCH-01   | 相同行为只存在一处            | 所有重复代码必须下沉到共享层                          |
| ARCH-06   | 依赖倒置，构造注入优先          | 新增模块必须面向接口，通过构造函数注入依赖                   |
| ARCH-07   | 开闭原则                 | 新增功能通过新增类，不修改稳定核心                       |
| ARCH-08   | 适配器隔离文件格式            | 任何文件 I/O 必须通过 IFormatAdapter            |
| CONCUR-01 | >50ms 操作异步           | 所有推理、文件读写必须异步                           |
| CONCUR-03 | 共享状态锁保护              | 重构后的所有共享数据结构考虑线程安全                      |
| ROBUST-02 | Result<T> 传播错误       | 禁止在业务代码中 throw                          |
| INFRA-01  | ServiceLocator 仅全局服务 | 页面资源绝不注册到 ServiceLocator                |
| INFRA-05  | PathUtils 统一路径库      | 禁止 path.string()，统一 PathUtils::toUtf8() |
| INFRA-06  | RAII 资源管理            | 禁止裸 new/delete，禁用手动 lock/unlock         |
| INFRA-08  | SettingsKey<T> 集中定义  | 新配置键必须在 Keys.h 中定义                      |
| INFRA-09  | 配置存用户目录              | 不存 .dsproj                              |
| INFRA-13  | PIMPL 隔离第三方头文件       | 公共接口包含第三方头文件时必须 PIMPL                   |
| INFRA-14  | 接口版本化                | 所有接口声明 kInterfaceVersion                |
| INFRA-16  | 不可变配置快照              | 流水线执行期间配置不可变                            |

### 2.2 补充设计准则（CCG-01~10）

| 准则              | 原则                          | 关键实施项                                       |
|-----------------|-----------------------------|---------------------------------------------|
| CCG-01 类型安全     | 强类型封装原始值                    | `Seconds`、`SampleRate` 强类型，`explicit` 构造函数  |
| CCG-02 编译期检查    | 优先 static_assert/consteval  | `SettingsKey<T>` 类型约束                       |
| CCG-03 值语义      | 值类型优先于指针传递                  | 内部模型值语义，`QSharedData` + COW                 |
| CCG-04 不可变优先    | `const` 默认                  | getter 标记 `const`，配置快照不可变                   |
| CCG-05 错误分层     | 四类错误明确区分                    | assert/Result/terminate/try-catch 各司其职      |
| CCG-06 RAII 全覆盖 | 资源绑定到作用域                    | lock_guard/scoped_lock，ONNX Session RAII 包装 |
| CCG-07 接口隔离     | 小而专注的接口                     | ISP 检查                                      |
| CCG-08 noexcept | 不会抛异常的函数标记 noexcept         | 析构/移动/getter 标记 noexcept                    |
| CCG-09 测试契约     | 公共 API 必有测试                 | given-when-then 结构                          |
| CCG-10 日志分层     | ERROR/WARN/INFO/DEBUG/TRACE | 上下文信息包含在日志消息中                               |

---

## 3 技术债清单

### 3.1 架构债

| 编号    | 问题                                           | 严重度 | 影响范围                                                |
|-------|----------------------------------------------|-----|-----------------------------------------------------|
| AD-01 | DSFile 与 DsTextDocument 双模型无自动桥接             | 中   | PitchLabelerPage 中手动做三向映射，分散多处易出错                   |
| AD-02 | DsDocument 与 DsTextDocument 概念重叠             | 中   | DsDocument 存储 .ds 工程数据，DsTextDocument 存储切片数据，转换逻辑分散 |
| AD-03 | label-suite 和 ds-labeler 的页面工厂逻辑耦合           | 中   | PageFactory 需要了解两个应用的所有页面类型                         |
| AD-04 | hfa-cli / test-shell 作为独立 app 与主 CLI 的功能边界模糊 | 低   | hfa-cli 本质上只是 CLI 的 HFA 子命令                         |

### 3.2 基础设施债

| 编号    | 问题                            | 严重度 | 影响范围                                                            |
|-------|-------------------------------|-----|-----------------------------------------------------------------|
| ID-01 | 配置键定义分散在 3 个文件中               | 中   | AppSettingKeys.h（遗留）、Keys.h（SettingsKey<T>）、CommonKeys.h（跨应用共享） |
| ID-02 | 文件路径选择无统一控件                   | 高   | PathSelector 存在但与各页面文件打开对话框行为不一致                                |
| ID-03 | 编码转换存在重复或潜在不一致                | 中   | Encoding.h 与 PathUtils.h 的职责边界模糊，PathUtils.cpp 内部引用 Encoding.h  |
| ID-04 | DsTextTypes.h 版本号硬编码为 "3.0.0" | 低   | 应使用 constexpr 集中定义                                              |
| ID-05 | 21 个魔法数字未迁移到配置系统              | 中   | 频谱图/功率图/波形图/钢琴卷帘渲染参数硬编码                                         |

### 3.3 代码质量债

| 编号    | 问题                   | 严重度 | 影响范围           |
|-------|----------------------|-----|----------------|
| CD-01 | 部分移动构造函数未标记 noexcept | 低   | 性能优化机会（CCG-08） |
| CD-02 | 测试覆盖率不足              | 中   | 核心接口缺少单元测试     |

### 3.4 已修复的历史债（供参考）

- IFileIOProvider / ISettingsBackend / ISlicerService 冗余接口（已删除）
- BoundaryBindingManager（已由 BoundaryDragController 取代）
- PPS/pixelsPerSecond（已由 resolution 取代）
- kResolutionTable 固定列表（已由连续范围 + 对数步进表取代）
- .dsproj 中 defaults 字段（已迁移到 AppSettings）
- processEvents() 调用（已全部移除）
- path.string() 调用（已全部替换为 PathUtils）
- ServiceLocator 误用（已限定为全局服务）
- IBoundaryModel / IEditorDataSource 缺少 kInterfaceVersion（已补充）

---

## 4 重构阶段规划

### 阶段 0：统一底层接口 (Platform Foundation)

**目标**：建立项目统一底层，所有路径操作、编码转换、文件选择、平台抽象收敛到固定模块。

#### 0.1 路径与编码统一模块

**当前状态**：`dsfw::PathUtils`（`src/framework/core/`）已提供核心路径操作和编码检测。`Encoding.h`
（同目录）提供编码转换函数。两者功能有重叠——PathUtils 已含 `detectEncoding` + `Encoding` 枚举，且 `PathUtils.cpp` 内部引用
`Encoding.h`。

**重构内容**：

1. **合并 Encoding.h 到 PathUtils**：将 `Encoding.h` 中的 `readFile`/`writeFile`/`decode`/`encode` 函数整合到 `PathUtils`
   中。
2. **移除独立的 Encoding.h**：删除 `Encoding.h` 和 `Encoding.cpp`，更新所有引用到 `PathUtils`。
3. **全量审查**：确保所有模块通过 PathUtils 处理路径和编码。
4. **添加 PathUtils 单元测试**：覆盖 Windows ANSI/UTF-8、macOS NFD/NFC、Linux UTF-8 路径编码场景。

**涉及文件**：

- `src/framework/core/include/dsfw/PathUtils.h` — 扩展接口
- `src/framework/core/src/PathUtils.cpp` — 合并实现
- `src/framework/core/include/dsfw/Encoding.h` — 删除
- `src/framework/core/src/Encoding.cpp` — 删除
- 所有引用 `Encoding.h` 的源文件 — 更新引用

#### 0.2 文件路径选择统一控件

**当前状态**：各页面各自使用 `QFileDialog`，行为不一致。`PathSelector` 控件已存在但未全面使用。

**重构内容**：

1. **增强 PathSelector 控件**：统一初始路径、过滤器、标题等行为。
2. **迁移所有页面**：将所有页面的 `QFileDialog` 调用替换为 `PathSelector`。
3. **支持最近路径记忆**：自动记忆每种选择类型的最近路径。

**涉及文件**：

- `src/framework/widgets/` — 增强 PathSelector
- 所有页面源文件 — 替换 QFileDialog 调用

#### 0.3 配置键集中管理

**当前状态**：配置键分散在 `AppSettingKeys.h`（遗留 `const char*`）、`Keys.h`（`SettingsKey<T>` 集中定义）、`CommonKeys.h`
（跨应用共享）。

**重构内容**：

1. **合并 AppSettingKeys.h 到 Keys.h**：迁移遗留 `const char*` 风格键到 `SettingsKey<T>` 类型。
2. **保持嵌套 namespace 分类**：已有的 `dstools::settings::pitch` 等 namespace 保持不变。
3. **SSOT 原则**：每个配置键只定义一次。

**涉及文件**：

- `src/apps/shared/settings/AppSettingKeys.h` — 删除
- `src/apps/shared/settings/Keys.h` — 合并
- `src/framework/core/include/dsfw/CommonKeys.h` — 保持不变

#### 0.4 修复顶部缩略图视口填充问题

**问题描述**：缩放到最小时，顶部 MiniMapScrollBar 的视口范围未填满整个视口。

**根因分析**：`AudioVisualizerContainer::updateViewRangeFromResolution()` 在计算 `visibleDuration` 时，
`ViewportController::resolution()` 返回的 resolution 值被四舍五入到最近十位，导致 `visibleDuration` 略小于实际可显示时长，视口不能完全覆盖。

**修复方案**：在 `updateViewRangeFromResolution()` 中，当 `endSec > totalDur` 时确保 `startSec` 从 0 开始，且 `endSec` 精确等于
`totalDur`。

**涉及文件**：

- `src/apps/shared/audio-visualizer/AudioVisualizerContainer.cpp` — 修复 `updateViewRangeFromResolution()`

---

### 阶段 1：架构清理 (Architecture Cleanup)

#### 1.1 统一内部文档模型 (AD-01, AD-02)

**当前状态**：`DSFile`（PitchEditor 专用）和 `DsTextDocument`（通用切片标注）是两套并行的数据模型。PitchLabelerPage 中手动做
DSFile <-> DsTextDocument <-> TextGridDocument 三向映射。

**重构内容**：

1. **扩展 DsTextDocument 为唯一数据源**：将 `DSFile` 的功能映射到 `DsTextDocument` 的层结构中。
2. **强化 DsTextDocBridge**：确保所有格式转换通过 `DsTextDocBridge` 统一完成。
3. **PitchLabel 集成到 Phoneme 视图架构**：PianoRollChartPanel 通过 `AudioVisualizerContainer` 的标准图表注册机制集成，而非独立管理。
4. **版本管理集中化**：`DsTextDocument::version` 改为 `constexpr` 集中定义。

**涉及文件**：

- `src/domain/` — DsTextDocument 扩展
- `src/apps/shared/bridges/DsTextDocBridge.*` — 强化转换
- `src/apps/shared/pitch-editor/` — 重构 PitchEditor 基于 DsTextDocument
- `src/apps/shared/phoneme-editor/PhonemeEditor.cpp` — 更新 PianoRollChartPanel 集成方式

#### 1.2 模块边界整理 (AD-03, AD-04)

1. **hfa-cli 合并到 CLI**：将 `hfa-cli` 作为 CLI 的子命令 `dataset-tools hfa`。
2. **test-shell 合并到 CLI**：将 `test-shell` 作为 CLI 的子命令 `dataset-tools test`。
3. **PageFactory 解耦**：引入 `IPageDescriptor` 接口，各应用独立注册页面。

**涉及文件**：

- `src/apps/cli/` — 扩展子命令
- `src/apps/hfa-cli/` — 删除
- `src/apps/test-shell/` — 删除
- `src/apps/shared/data-sources/PageFactory.*` — 解耦

#### 1.3 接口审查与补充

1. 审查所有接口是否满足 INFRA-02（接口必要性）
2. 为公共接口的移动构造函数/赋值运算符添加 noexcept（CCG-08）

---

### 阶段 2：技术债清偿 (Technical Debt Resolution)

#### 2.1 魔法数字配置化 (ID-05)

**当前状态**：`magic-numbers-audit.md` 识别了 23 个硬编码参数，2 个已迁移（音高提取），21 个待迁移。

**重构内容**：

1. **建立 ChartConfig 体系**：`ChartConfig` 结构体，包含所有图表渲染参数。
2. **在 Keys.h 中定义 ChartConfig 的 SettingsKey**。
3. **AppSettings 中读写 ChartConfig**。
4. **迁移现有硬编码参数**，按优先级：
    - 高频：频谱图渲染 (5 个)、功率图渲染 (3 个)
    - 中频：波形图渲染 (3 个)、钢琴卷帘编辑器 (10 个)
    - 低频：通用常量 (3 个)

**涉及文件**：

- `src/apps/shared/settings/Keys.h` — 新增 ChartConfig 键
- `src/apps/shared/audio-visualizer/SpectrogramChartPanel.*` — 使用配置
- `src/apps/shared/audio-visualizer/PowerChartPanel.*` — 使用配置
- `src/apps/shared/audio-visualizer/WaveformChartPanel.*` — 使用配置
- `src/apps/shared/pitch-editor/ui/PianoRollRenderer.*` — 使用配置

#### 2.2 测试体系补全 (CD-02)

1. **核心接口单元测试**：`IFormatAdapter`、`ITaskProcessor`、`PipelineRunner` 的 mock 测试
2. **PathUtils 编码测试**：Win/Mac/Linux 路径编码场景
3. **DsTextDocument 序列化测试**：往返测试 (round-trip)
4. **PipelineContext dirty 传播测试**：验证 DAG 传播算法
5. **Result<T> 传播链测试**：验证错误不被吞掉

---

### 阶段 3：稳定性与安全性加固 (Stability & Security)

#### 3.1 数据完整性

1. **原子写入全覆盖**：所有文件写入使用 `AtomicFileWriter`。
2. **写入前校验**：`DsTextDocument::save()` 写入前进行数据完整性校验。
3. **自动备份**：工程文件在写入前自动备份到 `dstemp/backups/`，保留最近 5 个版本。

#### 3.2 数据隔离

1. **临时文件隔离**：所有中间文件写入 `dstemp/`。
2. **模型输出隔离**：推理引擎输出写入独立子目录。
3. **多实例保护**：`SingleInstanceGuard` 确保同一工程不被多个实例同时打开。

#### 3.3 错误恢复

1. **工程文件损坏检测**：解析失败时尝试从备份恢复。
2. **切片数据损坏检测**：解析失败时报告具体错误位置。
3. **部分数据加载**：部分切片加载失败时允许成功的切片继续工作。

#### 3.4 安全审计

1. **路径遍历防护**：防止 `../` 逃逸攻击。
2. **JSON 注入防护**：确保导出文件名不包含特殊字符。
3. **敏感信息保护**：配置文件中不存储绝对路径中的用户名信息。

---

### 阶段 4：代码质量提升 (Code Quality)

#### 4.1 Noexcept 规范化 (CCG-08)

1. 审查所有析构函数，标记 `noexcept`
2. 审查所有移动构造函数/赋值运算符，标记 `noexcept`
3. 审查所有 getter/简单查询方法，标记 `noexcept`

#### 4.2 强类型引入 (CCG-01)

1. 引入 `Seconds`、`Milliseconds`、`Microseconds` 类型别名或 wrapper
2. 引入 `SampleRate` 强类型
3. 为单参数构造函数标记 `explicit`

#### 4.3 Const 正确性 (CCG-04)

1. 审查所有成员函数，getter 标记 `const`
2. 局部变量优先标记 `const`
3. 函数参数优先 `const &`

#### 4.4 日志规范化 (CCG-10)

1. 统一日志格式：`"[{module}] [{context}] message"`
2. 分级使用日志：ERROR/WARN/INFO/DEBUG/TRACE
3. 移除 `qDebug()` 调用，统一为 `dsfw::Log`

---

## 5 阶段依赖关系

```
阶段 0 (Platform Foundation)
  ├── 0.1 路径与编码统一 ────────── 无依赖
  ├── 0.2 文件路径选择统一控件 ──── 无依赖
  ├── 0.3 配置键集中管理 ────────── 无依赖
  └── 0.4 修复视口填充问题 ──────── 无依赖
        │
        ▼
阶段 1 (Architecture Cleanup)
  ├── 1.1 统一内部文档模型 ──────── 依赖 0.3（配置键集中）
  ├── 1.2 模块边界整理 ──────────── 依赖 1.1（文档模型统一后 CLI 才能正确迁移）
  └── 1.3 接口审查与补充 ────────── 无依赖
        │
        ▼
阶段 2 (Technical Debt Resolution)
  ├── 2.1 魔法数字配置化 ────────── 依赖 0.3（配置键集中）
  └── 2.2 测试体系补全 ──────────── 依赖 0.1, 1.1（PathUtils 和 DsTextDocument 稳定后）
        │
        ▼
阶段 3 (Stability & Security)
  └── 3.1~3.4 ──────────────────── 依赖 1.1（文档模型统一后）
        │
        ▼
阶段 4 (Code Quality)
  └── 4.1~4.4 ──────────────────── 依赖所有前置阶段完成
```

---

## 6 迁移策略

### 6.1 核心原则

1. **先行阶段已完成**：Phase S（可视化架构）、Phase R.1（配置系统）、Phase L（格式适配器）已在本计划之前完成。本计划任何阶段不得重复已完成的工作。
2. **逐阶段迁移**：按阶段 0 -> 1 -> 2 -> 3 -> 4 顺序执行，每阶段完成后进行回归测试。
3. **增量提交**：每完成一个子任务独立提交（不推送），便于回滚和审查。
4. **行为不变**：每个阶段结束时，所有现有功能的行为必须与重构前完全一致。
5. **编译即运行**：每次修改后立即编译，确保无编译错误后再继续。

### 6.2 风险控制

| 风险                              | 概率 | 影响 | 缓解措施                                  |
|---------------------------------|----|----|---------------------------------------|
| 路径编码迁移导致文件读写出错                  | 中  | 高  | PathUtils 迁移前先写单元测试覆盖所有编码场景           |
| DSFile -> DsTextDocument 迁移丢失数据 | 中  | 高  | 先实现 DsTextDocBridge 的完整 round-trip 测试 |
| QFileDialog 替换导致选择行为变化          | 低  | 中  | PathSelector 内部仍用 QFileDialog，仅统一参数   |
| 接口版本化导致编译错误                     | 低  | 低  | 引用 kInterfaceVersion 的代码逐一更新          |
| 重复实现 Phase S/R.1/L 已做的工作        | 中  | 中  | 实施前查阅 §1.4 和 §1.5 确认已完成的基础设施列表        |

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

| 文档                     | 更新内容                             | 阶段   |
|------------------------|----------------------------------|------|
| refactoring-plan-v2.md | 标记完成项，更新进度                       | 持续   |
| human-decisions.md     | 补充 CCG-01~10 到速查表                | 阶段 0 |
| overview.md            | 更新架构图（统一路径模块、FilePathSelector）   | 阶段 0 |
| conventions.md         | 补充 noexcept/强类型/const 正确性规范      | 阶段 4 |
| component-design.md    | 更新 DSFile/DsTextDocument 关系描述    | 阶段 1 |
| data-flow-design.md    | 合并后新文档，保持与代码同步                   | 持续   |
| magic-numbers-audit.md | 标记已迁移的魔法数字为"已完成"                 | 阶段 2 |
| test-design.md         | 补充 CCG-09 (given-when-then) 结构规范 | 阶段 2 |

### 7.2 重构完成后新增的文档

| 文档                                                       | 说明           |
|----------------------------------------------------------|--------------|
| docs/developer/architecture/platform/FilePathSelector.md | 文件路径选择控件设计文档 |
| docs/developer/architecture/platform/PathUtils.md        | 路径编码模块设计文档   |

---

## 8 关联文档

- [human-decisions.md](../../human-decisions.md) — 设计准则与决策
- [overview.md](overview.md) — 框架六层架构
- [data-flow-design.md](data-flow/data-flow-design.md) — 完整数据流设计
- [ds-format.md](data-flow/ds-format.md) — 工程格式规范
- [component-design.md](data-flow/component-design.md) — 核心组件设计
- [unified-app-design.md](framework/unified-app-design.md) — 统一应用设计
- [conventions.md](../../guides/conventions.md) — 编码规范
- [test-design.md](../testing/test-design.md) — 测试设计
- [magic-numbers-audit.md](../magic-numbers-audit.md) — 魔法数字审计