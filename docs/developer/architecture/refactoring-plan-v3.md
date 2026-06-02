# Dataset Tools 大型重构方案 v3

> 版本 3.0.0 | 2026-06-02
>
> 本文档是 dataset-tools 项目的综合重构方案 v3，基于对全部代码的重新探索和设计文档一致性核对后制定。
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
Layer 1  dsfw-core              AppSettings, ServiceLocator, AsyncTask, PathUtils
                                PipelineContext, PipelineRunner, ITaskProcessor, 接口集
Layer 0.5 dsfw-base             JsonHelper (Qt-free 静态库)
Layer 0  dsfw-types             Result<T>, ExecutionProvider, TimePos (header-only)
```

### 1.3 应用清单

| 应用             | 类型  | 说明                           |
|----------------|-----|------------------------------|
| ds-labeler     | GUI | DsLabeler 主应用（工程管理、标注流水线）    |
| label-suite    | GUI | LabelSuite（独立编辑器集合）          |
| cli            | CLI | 命令行工具                        |
| hfa-cli        | CLI | Hubert Forced Aligner 命令行    |
| test-shell     | CLI | 推理引擎测试外壳                     |
| widget-gallery | GUI | UI 控件展示                      |

### 1.4 已完成的先行成果

以下成果已在实际开发中完成，不需要重复实施：

| 阶段            | 完成内容                                                                   | 完成时间    |
|---------------|------------------------------------------------------------------------|---------|
| Phase S       | 可视化架构统一：AudioVisualizerContainer、MiniMapScrollBar、BoundaryOverlayWidget 等   | 2026-05 |
| Phase R.1     | 配置系统重构：统一到 AppSettings，删除 ProjectSettingsBackend，废弃 ISettingsBackend       | 2026-05 |
| Phase L       | 格式适配器基础设施：TextGridAdapter、DsFileAdapter、CsvAdapter、LabAdapter              | 2026-05 |
| Phase V.1     | 路径与编码统一：Encoding.h 合并到 PathUtils，PathEncoding.h 标记 [[deprecated]]         | 2026-05 |
| Phase V.2     | 文件路径选择控件：FileDialogHelper、PathSelector、FilePathSelector，QFileDialog 已清理    | 2026-05 |
| Phase V.3     | 配置键集中管理：AppSettingKeys.h 删除，配置键迁移到 Keys.h + CommonKeys.h                | 2026-05 |
| Phase V.4     | MiniMap 视口填充修复：updateViewRangeFromResolution() 已实现 endSec clamp           | 2026-05 |
| Phase V.5     | ISlicerService 冗余接口删除，改为具体类                                           | 2026-05 |
| Phase V.6     | 接口 kInterfaceVersion 补充：已为所有主要接口添加版本号                                  | 2026-05 |

### 1.5 已完备的基础设施

| 基础设施                                  | 位置                            |
|---------------------------------------|-------------------------------|
| AudioVisualizerContainer 统一视图         | apps/shared/audio-visualizer  |
| ViewportController 统一缩放               | dsfw-widgets                  |
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

## 2 重构原则与补充准则

### 2.1 核心约束（摘自 human-decisions.md）

| 编号        | 约束              | 重构中的影响                        |
|-----------|-----------------|-------------------------------|
| ARCH-01   | 相同行为只存在一处       | 所有重复代码必须下沉到共享层                |
| ARCH-06   | 依赖倒置，构造注入优先     | 新增模块必须面向接口，通过构造函数注入依赖         |
| ARCH-07   | 开闭原则            | 新增功能通过新增类，不修改稳定核心             |
| ARCH-08   | 适配器隔离文件格式       | 任何文件 I/O 必须通过 IFormatAdapter   |
| ROBUST-01 | Result<T> 传播错误  | 所有可能失败的操作返回 Result<T>，不抛异常     |
| ROBUST-02 | 禁止静默吞掉 catch    | 捕获必须记录日志或返回错误                 |
| ROBUST-03 | try-catch 仅限第三方边界 | 业务逻辑不使用 try-catch             |
| CONCUR-01 | IO/计算 (>50ms) 异步 | 长操作使用 AsyncTask 或 QtConcurrent |
| CONCUR-02 | 禁止 processEvents | 使用异步 + 信号                     |
| CONCUR-03 | UI 线程安全          | 后台线程通过 QMetaObject::invokeMethod 操作 UI |
| INFRA-01  | PathUtils 统一路径   | 禁止 path.string()，统一用 PathUtils  |
| INFRA-02  | RAII 资源管理       | 禁止裸 new/delete                  |
| INFRA-03  | ServiceLocator 限制 | 仅注册全局服务，不注册页面级资源              |
| INFRA-04  | SettingsKey<T> 集中 | 所有配置键在 Keys.h 中声明，强类型         |

### 2.2 补充准则

以下准则基于 C++/Qt 项目最佳实践补充，作为本次重构的技术红线：

#### CD-01：PIMPL 模式用于公共 API

所有暴露给外部模块的头文件，如果包含 Qt 类型或内部实现细节，必须使用 PIMPL（d-pointer）模式隐藏实现，减少编译依赖和 ABI 脆弱性。

```cpp
// 公共头文件：仅暴露接口
class MyWidget : public QWidget {
    Q_OBJECT
public:
    explicit MyWidget(QWidget *parent = nullptr);
    ~MyWidget() override;
    void doSomething();
private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
```

#### CD-02：const 正确性

不修改状态的成员函数必须标记 `const`。局部变量、函数参数在不修改时标记 `const`。这既是文档也是编译器约束。

```cpp
// 错误
int getValue() { return m_value; }
// 正确
int getValue() const { return m_value; }
```

#### CD-03：noexcept 用于不抛异常的函数

移动构造函数、swap 函数、简单 getter/setter 应标记 `noexcept`，使标准库容器能使用更高效的移动语义。

```cpp
MyClass(MyClass&& other) noexcept : m_data(std::exchange(other.m_data, nullptr)) {}
```

#### CD-04：前向声明优先

头文件中能用前向声明的类型绝不 `#include` 其完整定义，减少编译时间。

```cpp
// 头文件
class QString;  // 前向声明
class MyClass {
    void doSomething(const QString &s);
};
```

#### CD-05：命令查询分离（CQS）

修改状态的方法（命令）不应返回值；返回值的方法（查询）不应修改状态。

```cpp
// 错误：混合了查询和修改
bool loadFile(const QString &path);  // 修改了内部状态 + 返回状态

// 正确
void loadFile(const QString &path);  // 命令：修改状态
bool isLoaded() const;               // 查询：返回状态
```

#### CD-06：强类型封装

领域概念使用 `enum class` 或包装类型，不使用裸 `int`/`double` 传递领域语义。已有的 `SettingsKey<T>` 和 `TimePos` 是良好范例，应推广到其他领域概念。

```cpp
// 错误
void setResolution(int samplesPerPixel);  // int 是什么? 是什么的resolution?
// 正确
enum class ResolutionMode { Low, Medium, High };  // 或用包装类型
void setResolution(ResolutionMode mode);
```

#### CD-07：测试金字塔原则

L1 单元测试覆盖所有纯逻辑类（>=80%），L2 集成测试覆盖组件交互路径，L3 推理测试覆盖端到端流程。无测试的代码视为有债。

#### CD-08：编译防御深度

`#pragma once`（已使用）+ 公共头文件不暴露实现细节（PIMPL）+ 模块间通过接口通信，降低编译依赖。

#### CD-09：确定性析构

不使用 `QObject::deleteLater()` 作为资源管理手段。RAII 保证确定性析构。仅在 Qt 事件循环中有明确时序要求时使用 deleteLater。

---

## 3 待解决的技术债

### 3.1 架构债（ARCH）

| ID       | 问题                                                              | 严重度 | 来源 |
|----------|-----------------------------------------------------------------|-----|----|
| ARCH-D1  | DsPitchDocument 和 DsTextDocument 仍为并行双模型，DsTextDocBridge 已实现但效率未验证 | 中   | 探索 |
| ARCH-D2  | hfa-cli 和 test-shell 作为独立 CLI 应用，存在代码重复和构建配置冗余                     | 低   | v2  |
| ARCH-D3  | unified-editor 目录为空，LabelSuite + DsLabeler 仍为两个独立 GUI 应用            | 中   | 探索 |
| ARCH-D4  | dsfw-signal 层未按命名空间分模块，F0Curve/CurveTools 与 pipeline 处理逻辑耦合        | 低   | 探索 |
| ARCH-D5  | IFormatAdapter 的 kInterfaceVersion 未在运行时校验，仅用于文档记录                  | 低   | 探索 |

### 3.2 配置债（CONFIG）

| ID        | 问题                                                                     | 严重度 | 来源 |
|-----------|------------------------------------------------------------------------|-----|----|
| CONFIG-D1 | SpectrogramChartPanel 中的 FFT 参数硬编码（fftSize=2048, hopSize=441 等）          | 中   | 审计 |
| CONFIG-D2 | PowerChartPanel 中的频率范围硬编码（minFreq=40, maxFreq=8000）                    | 中   | 审计 |
| CONFIG-D3 | WaveformChartPanel 中的波形参数硬编码（preEmphasis, noiseGateDb）                 | 中   | 审计 |
| CONFIG-D4 | PianoRollView 中的 MIDI 音符渲染参数硬编码（noteHeight, minNoteWidth 等）            | 中   | 审计 |
| CONFIG-D5 | AudioVisualizerContainer 中的默认高度比例硬编码（defaultHeightRatios 方法中）            | 低   | 审计 |

### 3.3 代码债（CODE）

| ID       | 问题                                                               | 严重度 | 来源 |
|----------|------------------------------------------------------------------|-----|----|
| CODE-D1  | DsTextTypes.h 中版本号仍硬编码为 "3.0.0"，虽有 constexpr 但应迁移到构建系统或配置           | 低   | v2  |
| CODE-D2  | IntegerDivision 函数存在但未被所有需要的地方使用（某些地方仍直接做整数除法）                       | 低   | 探索 |
| CODE-D3  | AudioDecoder.cpp 内部使用 FFmpeg C API，部分路径处理未通过 PathUtils              | 中   | 探索 |
| CODE-D4  | CurveTools.cpp 中 F0 插值算法有边界条件未处理（空 curve、单点曲线）                        | 中   | 探索 |
| CODE-D5  | AsyncTask 的 cancel 机制缺乏统一的超时策略                                    | 低   | 探索 |
| CODE-D6  | PipelineRunner 对 Step 的 propagateDirty 实现中，依赖声明式接口但未运行时校验前置条件      | 低   | 探索 |

### 3.4 数据安全债（SAFETY）

| ID        | 问题                                                                 | 严重度 | 来源 |
|-----------|--------------------------------------------------------------------|-----|----|
| SAFETY-D1 | AtomicFileWriter 已实现但并非所有写操作都经过它，部分格式适配器使用 QFile 直接写入            | 高   | 探索 |
| SAFETY-D2 | 工程文件（.dsproj）缺少自动备份机制，项目损坏时无法恢复                                | 高   | 探索 |
| SAFETY-D3 | DsTextDocument/DsPitchDocument 保存前无数据完整性校验（如 CRC 或字段合法性校验）          | 高   | 探索 |
| SAFETY-D4 | 外部文件导入（TextGrid/CSV/Lab）时，错误格式的文件可能产生部分损坏的内部数据，缺乏事务性导入（全成功或全失败） | 中   | 探索 |
| SAFETY-D5 | 工程配置中的外部路径（音频文件、模型路径）变更后无自动检查机制                                 | 中   | 探索 |

### 3.5 测试债（TEST）

| ID       | 问题                                               | 严重度 | 来源 |
|----------|--------------------------------------------------|-----|----|
| TEST-D1  | PathUtils 无单元测试（编码检测、路径规范化等关键路径未覆盖）               | 高   | 探索 |
| TEST-D2  | AtomicFileWriter 无单元测试（原子写入失败恢复路径未验证）             | 高   | 探索 |
| TEST-D3  | PipelineRunner 无单元测试（dirty 传播、步骤依赖、取消流程未验证）       | 高   | 探索 |
| TEST-D4  | IFormatAdapter 的各种实现仅有集成测试，缺少边界条件的独立单元测试         | 中   | 探索 |
| TEST-D5  | AudioVisualizerContainer 无组件级测试（hittest、resize 行为未验证） | 中   | 探索 |
| TEST-D6  | 现有 21 个框架测试覆盖率约 40%，领域层和应用层接近 0%                 | 中   | 审计 |

### 3.6 文档债（DOC）

| ID      | 问题                                                           | 严重度 | 来源 |
|---------|--------------------------------------------------------------|-----|----|
| DOC-D1  | component-design.md 中仍有 DSFile 引用，已更正为 DsPitchDocument       | 低   | 探索 |
| DOC-D2  | overview.md 中架构图标注的 "迁移中" 状态与实际不符                             | 低   | 探索 |
| DOC-D3  | human-decisions.md 中部分代码示例使用了已废弃的 API                        | 低   | 探索 |
| DOC-D4  | docs/README.md 曾引用不存在的 refactoring-plan-v1.md，已更正为 v2 + v3 | 低   | 探索 |

---

## 4 重构阶段规划

### 4.1 Phase A：数据安全保障（优先，预计影响最大）

**目标**：确保在任何故障场景下工程数据不丢失、不损坏。

| 任务        | 内容                                                                              | 关联债       |
|-----------|---------------------------------------------------------------------------------|-----------|
| A-01      | 所有文件写入统一经过 AtomicFileWriter，移除 IFormatAdapter 中的裸 QFile 写入                       | SAFETY-D1 |
| A-02      | 实现 ProjectBackupManager：.dsproj 文件自动备份（保持最近 N 个版本，可配置），打开工程时检测并提示恢复                | SAFETY-D2 |
| A-03      | DsTextDocument/DsPitchDocument 保存前增加字段合法性校验（必填字段非空、坐标范围合法等），非法数据拒绝保存并提示用户           | SAFETY-D3 |
| A-04      | 外部文件导入实现事务性：先解析到临时模型，验证通过后再替换现有数据，失败则完全回滚                                        | SAFETY-D4 |
| A-05      | 工程打开时校验配置中引用的外部路径是否存在，不存在的路径在 UI 中醒目标记                                               | SAFETY-D5 |
| A-06      | PathUtils 增加文件完整性校验方法（SHA-256），供备份和导入使用                                              | 新增        |

**验收标准**：
- 模拟写盘断电，重启后工程文件可恢复
- 导入格式错误的 TextGrid/CSV/Lab 不产生部分数据污染
- 所有写操作路径经过 AtomicFileWriter

### 4.2 Phase B：测试补全

**目标**：L1 测试覆盖率 >= 70%，关键路径 100% 覆盖。

| 任务        | 内容                                             | 关联债      |
|-----------|------------------------------------------------|----------|
| B-01      | PathUtils 单元测试（覆盖所有编码检测/解码/编码路径、路径规范化、文件读写）    | TEST-D1  |
| B-02      | AtomicFileWriter 单元测试（正常写入、写入中断恢复、并发写入保护）      | TEST-D2  |
| B-03      | PipelineRunner 单元测试（正常执行、步骤失败、取消、dirty 传播）      | TEST-D3  |
| B-04      | IFormatAdapter 实现独立单元测试（边界输入、错误格式、空数据）           | TEST-D4  |
| B-05      | AudioVisualizerContainer 组件级测试（hittest 精确度、resize 行为） | TEST-D5  |
| B-06      | CurveTools 单元测试（空曲线、单点、极值、噪声）                     | CODE-D4  |
| B-07      | ProjectBackupManager 单元测试（备份轮转、恢复流程）                | A-02     |

**验收标准**：
- `ctest --output-on-failure` 全部通过
- PathUtils、AtomicFileWriter、PipelineRunner 覆盖率 >= 90%
- 测试随 CI 自动执行（如配置 CI）

### 4.3 Phase C：魔法数字配置化

**目标**：所有算法参数和 UI 参数从硬编码迁移到 SettingsKey<T>。

| 任务        | 内容                                                          | 关联债       |
|-----------|-------------------------------------------------------------|-----------|
| C-01      | SpectrogramChartPanel 的 FFT 参数迁移到 SettingsKey<T>（fftSize、hopSize、windowType、melBins） | CONFIG-D1 |
| C-02      | PowerChartPanel 的频率范围迁移到 SettingsKey<T>（minFreq、maxFreq）       | CONFIG-D2 |
| C-03      | WaveformChartPanel 的波形参数迁移到 SettingsKey<T>（preEmphasis、noiseGateDb） | CONFIG-D3 |
| C-04      | PianoRollView 的渲染参数迁移到 SettingsKey<T>（noteHeight、minNoteWidth、colors） | CONFIG-D4 |
| C-05      | AudioVisualizerContainer 的默认高度比例迁移到 SettingsKey<T>               | CONFIG-D5 |
| C-06      | 确认 ChartConfigRegistry + ChartConfigTypes.h 体系是否覆盖所有配置项，补齐缺口          | 新增        |

**验收标准**：
- 所有图表内无裸字面量算法参数
- 改变配置值后图表渲染即时生效（无需重启）

### 4.4 Phase D：代码统一与清理

**目标**：消除重复代码，统一行为。

| 任务        | 内容                                                              | 关联债      |
|-----------|-----------------------------------------------------------------|----------|
| D-01      | hfa-cli 和 test-shell 合并入统一 CLI 应用，HFA 和 TestInfer 作为子命令                  | ARCH-D2  |
| D-02      | IntegerDivision 推广到所有整数除法位置，添加编译时常量检查和溢出保护                              | CODE-D2  |
| D-03      | AudioDecoder 中路径处理统一使用 PathUtils，移除裸字符串路径操作                            | CODE-D3  |
| D-04      | PipelineRunner 增加 Step 前置条件运行时校验（preconditions 方法）                         | CODE-D6  |
| D-05      | AsyncTask 增加统一超时策略（默认 30s，可配置）                                          | CODE-D5  |
| D-06      | 移除 PathEncoding.h 中 [[deprecated]] 标记的旧 API（需确保唯一引用 game-infer/tests/main.cpp 已迁移） | 新增       |
| D-07      | 所有公共类标记 noexcept（移动构造、swap、简单 getter）                                    | 新增       |
| D-08      | 所有公共类标记 const（不修改状态的成员函数）                                             | 新增       |

**验收标准**：
- CLI 工具整合为一个可执行文件，通过子命令区分
- `grep -r "path.string()" src/apps/` 返回零结果
- 公共 API noexcept/const 覆盖率 >= 90%

### 4.5 Phase E：架构演进

**目标**：清理架构债，简化模块依赖。

| 任务        | 内容                                                             | 关联债      |
|-----------|----------------------------------------------------------------|----------|
| E-01      | 评估 DsPitchDocument + DsTextDocument 合并可行性：分析两个模型的重叠度，>60% 则合并为统一文档模型     | ARCH-D1  |
| E-02      | unified-editor 目录填充：迁移 LabelSuite + DsLabeler 共有的 AppShell 配置到该目录，减少重复 | ARCH-D3  |
| E-03      | dsfw-signal 层重构：F0Curve 和 CurveTools 按功能模块化，与 pipeline 逻辑解耦            | ARCH-D4  |
| E-04      | IFormatAdapter 运行时版本校验：在 FormatAdapterRegistry 注册时检查 kInterfaceVersion   | ARCH-D5  |
| E-05      | DsTextTypes.h 版本号从硬编码迁移到 CMake 生成（version.h.in）或项目级配置                    | CODE-D1  |
| E-06      | 全面审计并移除所有未使用的头文件 include                                           | 新增       |

**验收标准**：
- DsPitchDocument + DsTextDocument 重叠度分析报告完成
- unified-editor 目录有实际内容和独立 CMakeLists.txt
- `#include` 依赖图比当前减少 20%+

---

## 5 实施约束与风险控制

### 5.1 不可变原则

| 原则       | 说明                                                   |
|----------|------------------------------------------------------|
| 行为一致     | 所有重构不改变任何用户可见行为，回归测试必须 100% 通过                      |
| 数据安全     | Phase A 必须在其他阶段之前完成，确保后续重构有安全网                      |
| 增量提交     | 每个子任务独立提交（不推送），确保可单独回滚                              |
| 无新依赖     | 不引入项目未使用的外部库，仅使用已有技术栈（Qt6, nlohmann_json, ONNX Runtime） |
| 文档同步     | 每个 Phase 完成后更新相关设计文档                                 |

### 5.2 每阶段执行流程

```
1. 阅读相关设计文档（human-decisions.md + 相关章节）
2. 逐条对照设计准则复核实施计划
3. 编写测试（测试先行）
4. 实施代码变更
5. 运行完整构建（cmake --build --preset release）
6. 运行测试套件（ctest --output-on-failure）
7. 手动冒烟测试（打开 ds-labeler 执行基本操作）
8. 更新文档
9. 提交（不推送）
```

### 5.3 风险矩阵

| 风险              | 影响 | 可能性 | 缓解措施                         |
|-----------------|----|-----|------------------------------|
| 重构引入数据损坏        | 高  | 低   | Phase A 先行建立安全网              |
| 测试覆盖不足导致回归      | 高  | 中   | Phase B 测试先行，关键路径 100% 覆盖   |
| 性能回归            | 中  | 低   | 每个 Phase 后基准测试               |
| 文档与实际不一致        | 低  | 中   | 每阶段结束更新文档                   |
| 编译时间增长          | 低  | 低   | PIMPL + 前向声明减少头文件依赖          |
| DsPitchDocument 合并失败 | 中  | 中   | 先分析重叠度，<60% 则不合并，仅优化桥接器     |

---

## 6 统一底层接口规范

### 6.1 路径与编码（PathUtils，已完成）

所有跨平台路径操作通过 `dsfw::PathUtils`：

| 方法                    | 用途                       |
|-----------------------|--------------------------|
| `toStdPath / fromStdPath` | QString <-> std::filesystem::path |
| `toUtf8 / toWide`       | path -> string/wstring   |
| `join`                | 路径拼接（跨平台分隔符）            |
| `normalize`           | 规范化路径                    |
| `openFile`            | 二进制安全的跨平台文件打开           |
| `detectTextEncoding`  | 自动检测文本编码（BOM + 启发式）      |
| `decodeText / encodeText` | 文本编解码                    |
| `readFile / writeFile`  | 自动检测编码的文本文件读写            |
| `canonicalOrNull`     | 规范化路径（不存在返回 nullopt）     |
| `isSubPath`           | 子路径判断                    |
| `relativeTo`          | 相对路径计算                   |

**规范**：
- `std::filesystem::path` 用于内部路径存储和传递
- `QString` 用于 Qt UI 显示，通过 `fromStdPath/toStdPath` 转换
- 禁止 `path.string()` 和 `path.wstring()` 直接调用，统一通过 PathUtils

### 6.2 文件选择（FileDialogHelper + PathSelector，已完成）

| 组件                | 用途                         |
|-------------------|----------------------------|
| `FileDialogHelper` | 跨平台文件对话框封装（读写分别提供方法）       |
| `PathSelector`     | 行编辑器 + 浏览按钮组合，用于设置页面        |
| `FilePathSelector` | 路径下拉选择器，带历史记录              |

**规范**：
- 所有文件选择操作通过 `FileDialogHelper`，禁止直接使用 `QFileDialog`
- 设置页面中的路径配置使用 `PathSelector` / `FilePathSelector`
- 文件对话框统一处理编码问题（Gbk 路径在 Windows 上自动转换）

### 6.3 项目备份（ProjectBackupManager，Phase A 新增）

```cpp
class ProjectBackupManager {
public:
    static Result<void> createBackup(const std::filesystem::path &projectPath);
    static Result<std::vector<std::filesystem::path>> listBackups(const std::filesystem::path &projectPath);
    static Result<void> restoreFromBackup(const std::filesystem::path &backupPath);
    static Result<void> pruneBackups(const std::filesystem::path &projectPath, int keepCount);
};
```

**规范**：
- 备份路径：`<项目目录>/.backups/<项目名>_<时间戳>.bak`
- 默认保留最近 10 个备份，可通过 `SettingsKey<int>` 配置
- 每次保存前自动创建备份（异步，不阻塞 UI）
- 打开工程时检测最新备份是否比当前文件新，提示用户选择

### 6.4 原子写入（AtomicFileWriter，已存在，需推广）

```cpp
class AtomicFileWriter {
public:
    explicit AtomicFileWriter(const std::filesystem::path &targetPath);
    Result<void> write(const QByteArray &data);
    Result<void> write(const QString &text, PathUtils::TextEncoding encoding);
    Result<void> commit();
    void discard();
};
```

**规范**：
- 写临时文件 -> 验证完整性 -> rename 到目标路径（POSIX 原子操作）
- 如果 rename 失败（跨设备），fallback 到 copy + delete
- 所有 `IFormatAdapter::save()` 实现必须使用 AtomicFileWriter
- 禁止 `QFile::open(WriteOnly) + write + close` 直接覆写源文件

---

## 7 任务清单与进度

### Phase A：数据安全保障

| 任务    | 状态 | 关联 PR |
|-------|----|-------|
| A-01  | 待开始 | --    |
| A-02  | 待开始 | --    |
| A-03  | 待开始 | --    |
| A-04  | 待开始 | --    |
| A-05  | 待开始 | --    |
| A-06  | 待开始 | --    |

### Phase B：测试补全

| 任务    | 状态 | 关联 PR |
|-------|----|-------|
| B-01  | 待开始 | --    |
| B-02  | 待开始 | --    |
| B-03  | 待开始 | --    |
| B-04  | 待开始 | --    |
| B-05  | 待开始 | --    |
| B-06  | 待开始 | --    |
| B-07  | 待开始 | --    |

### Phase C：魔法数字配置化

| 任务    | 状态 | 关联 PR |
|-------|----|-------|
| C-01  | 待开始 | --    |
| C-02  | 待开始 | --    |
| C-03  | 待开始 | --    |
| C-04  | 待开始 | --    |
| C-05  | 待开始 | --    |
| C-06  | 待开始 | --    |

### Phase D：代码统一与清理

| 任务    | 状态 | 关联 PR |
|-------|----|-------|
| D-01  | 待开始 | --    |
| D-02  | 待开始 | --    |
| D-03  | 待开始 | --    |
| D-04  | 待开始 | --    |
| D-05  | 待开始 | --    |
| D-06  | 待开始 | --    |
| D-07  | 待开始 | --    |
| D-08  | 待开始 | --    |

### Phase E：架构演进

| 任务    | 状态 | 关联 PR |
|-------|----|-------|
| E-01  | 待开始 | --    |
| E-02  | 待开始 | --    |
| E-03  | 待开始 | --    |
| E-04  | 待开始 | --    |
| E-05  | 待开始 | --    |
| E-06  | 待开始 | --    |

---

## A 附录：与 v2 的差异

| 方面     | v2                                       | v3                                              |
|--------|------------------------------------------|-------------------------------------------------|
| 已完成任务  | 作为待办列出                                   | 移至 1.4 已完成成果，不重复计划                              |
| 补充准则   | 仅引用 human-decisions                       | 新增 CD-01~CD-09 补充准则                            |
| 数据安全   | 未独立成章                                    | 独立 Phase A，最高优先级                                |
| 测试补全   | 未明确列出具体测试目标                              | 独立 Phase B，明确每个测试覆盖的模块和验收标准                     |
| 魔法数字   | 概括性提及                                    | 独立 Phase C，按面板逐项列出                              |
| 架构演进   | 未涉及 DsPitchDocument 合并评估、unified-editor 填充 | 独立 Phase E，明确分析先行再决定是否执行                        |
| 执行流程   | 未细化                                      | 5.2 节定义每阶段标准执行流程                                |
| 风险矩阵   | 无                                        | 5.3 节定义具体风险和缓解措施                                |
| 文档清理   | 无                                        | 已修正 component-design.md、overview.md、README.md 的过时引用 |