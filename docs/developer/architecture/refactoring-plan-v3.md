# Dataset Tools 大型重构方案 v3

> 版本 3.2 | 2026-06-03
>
> 本文档是 dataset-tools 项目的综合重构方案 v3，基于对全部代码的重新探索和设计文档一致性核对后制定。
> 目标：抛弃一切技术与架构债，建立统一的底层接口体系，同时保证现有功能完全不变、行为一致、数据安全稳定。
>
> **v3.2 更新**: 对所有已完成任务进行全面代码核实，确认实现正确后精简文档，移除冗余核实记录。主要发现：C-06（ChartConfigRegistry 持久化）实际已实现，修正文档状态。先行成果中 3 项为文档级不准确（Phase S BoundaryOverlayWidget 位置、Phase L CsvAdapter.h 目录、Phase V.4 方法归属），功能实现正确。
>
> **v3.1 更新**: Phase A+B 完成核实，AtomicFileWriter JSON 验证 bug 修复，Phase D/E 任务清单全面代码核实（D-02/D-06 移除、D-04/D-05/E-04 标记完成），技术债清单清理
>
> **v3.1.1 更新**: 逐项复核完善：修正 CODE-D1、CONFIG-D5、E-02、E-05、6.5 命名空间表
>
> 本方案严格遵循 [human-decisions.md](../../human-decisions.md) 的设计准则体系（ARCH/CONCUR/ROBUST/INFRA/VIEW），
> 流程适应 [data-flow-design.md](data-flow/data-flow-design.md) 完整数据流设计。

---

## 1 项目现状

### 1.1 技术栈

| 层级     | 技术                    | 版本要求                                          |
|--------|-----------------------|-----------------------------------------------|
| 语言     | C++20                 | MSVC 2022 / GCC / Clang                       |
| GUI    | Qt 6                  | 6.8+, Core/Gui/Widgets/Svg/Network/Concurrent |
| 构建     | CMake                 | 3.21+                                         |
| 包管理    | vcpkg                 | nlohmann_json, QWindowKit                     |
| 推理     | ONNX Runtime          | --                                            |
| 音频解码   | FFmpeg                | --                                            |
| 音频播放   | SDL2                  | --                                            |
| 音频文件读写 | SndFile, soxr, mpg123 | --                                            |
| 窗口框架   | QWindowKit            | --                                            |
| CLI 参数 | syscmdline            | --                                            |
| G2P    | cpp-pinyin            | --                                            |

### 1.2 实际依赖层次

实际代码中模块以 CMake target 为单位组织，层次关系如下：

```
App Layer    ds-labeler, label-suite, dstools-cli, test-shell, widget-gallery
                ↓
App Shared   dstools-ui-core    (STATIC, src/ui-core/)
                ↓ 包装 dsfw-ui-core + dsfw-core + dstools-domain
App Libs     dstools-domain     (STATIC, src/domain/)
             DsPitchDocument, DsTextDocument, DsProject
             F0Curve, CurveTools, MouthCurve
             格式适配器 (CsvAdapter, DsDocumentAdapter 等)
                ↓
──────────────── 框架层 ─────────────────
Layer 4  dsfw-widgets           通用 UI 组件 (SHARED DLL)
Layer 3  dsfw-ui-core           AppShell, IconNavBar, Theme, FramelessHelper, IPageActions (STATIC)
Layer 2  dstools-audio          AudioDecoder (FFmpeg), AudioPlayback (SDL2) (STATIC)
Layer 1  dsfw-core              AppSettings, ServiceLocator, AsyncTask, PathUtils
                                PipelineContext, PipelineRunner, ITaskProcessor, 接口集
                                含 infer-common 源文件 (OnnxEnv, OnnxModelBase)
Layer 0.5 dsfw-base             JsonHelper (Qt-free 静态库)
Layer 0  dsfw-types             Result<T>, ExecutionProvider, TimePos (header-only)

此外层：
dsfw-signal           curve_tools, music_math, time_series (dsfw::signal 命名空间, 静态库)
dstools-widgets       INTERFACE header-only 层, 通过 dsfw-widgets 导出宏转发
```

#### 实际依赖关系

```
dsfw-widgets ─PUBLIC──→ dsfw-core ───→ dsfw-base ───→ dsfw-types
    │                       ↑
    ├─PRIVATE→ dsfw-ui-core ┘
    └─PRIVATE→ dstools-audio

dsfw-signal ───→ dsfw-types
dstools-domain → dsfw-core + dsfw-signal + dsfw-types
dstools-ui-core → dsfw-ui-core + dsfw-core + dstools-domain
infer-bridge   → dsfw-core + dstools-domain + hubert-infer + rmvpe-infer + game-infer
```

**注意**：`dstools-infer-common`（OnnxEnv, OnnxModelBase, IInferenceEngine）并非独立 target，其源文件被 `dsfw-core` 通过
`target_sources()` 拉入构建。详见 `framework/core/CMakeLists.txt`。

### 1.3 应用清单

| 应用             | 类型  | CMake Target  | 说明                        |
|----------------|-----|---------------|---------------------------|
| ds-labeler     | GUI | DsLabeler     | DsLabeler 主应用（工程管理、标注流水线） |
| label-suite    | GUI | LabelSuite    | LabelSuite（独立编辑器集合）       |
| dstools-cli    | CLI | dstools-cli   | 统一命令行工具（含 HFA、切片、对齐等子命令）  |
| test-shell     | GUI | TestShell     | 推理引擎测试外壳（WIN32 GUI）       |
| widget-gallery | GUI | WidgetGallery | UI 控件展示                   |

**注意**：hfa-cli 不独立存在，HFA 功能通过 `dstools-cli` 子命令 + `libs/hubert-fa/` + `infer-bridge` 提供。

### 1.4 已完成的先行成果

以下成果已在实际开发中完成，不需要重复实施：

| 阶段        | 完成内容                                                                                            |
|-----------|-------------------------------------------------------------------------------------------------|
| Phase S   | 可视化架构统一：AudioVisualizerContainer、MiniMapScrollBar、BoundaryOverlayWidget                 |
| Phase R.1 | 配置系统重构：统一到 AppSettings，删除 ProjectSettingsBackend，废弃 ISettingsBackend                |
| Phase L   | 格式适配器基础设施：TextGridAdapter、DsFileAdapter、CsvAdapter、LabAdapter                          |
| Phase V.1 | 路径与编码统一：Encoding.h 合并到 PathUtils，PathEncoding.h 已删除                                  |
| Phase V.2 | 文件路径选择控件：FileDialogHelper、PathSelector、FilePathSelector，QFileDialog 已清理              |
| Phase V.3 | 配置键集中管理：AppSettingKeys.h 删除，配置键迁移到 Keys.h + CommonKeys.h                             |
| Phase V.4 | MiniMap 视口填充修复：updateViewRangeFromResolution() 已实现 endSec clamp                          |
| Phase V.5 | ISlicerService 冗余接口删除，改为具体类                                                              |
| Phase V.6 | 接口 kInterfaceVersion 补充：已为所有主要接口添加版本号                                                 |
| Phase A   | 数据安全保障（6项）：AtomicFileWriter 统一、ProjectBackupManager、数据校验、外部路径校验、CRC32 完整性       |
| Phase B   | 测试补全（7项）：PathUtils、AtomicFileWriter、PipelineRunner、FormatAdapters、AudioVisualizerContainer、CurveTools、ProjectBackupManager |
| Phase D   | D-04 PipelineRunner 前置条件校验、D-05 AsyncTask 超时机制                                           |
| Phase E   | E-04 IFormatAdapter 运行时版本校验                                                                 |
| Phase C   | C-01~C-06 ChartConfigRegistry 配置化 + 持久化                                                     |

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

| 编号        | 约束                | 重构中的影响                                 |
|-----------|-------------------|----------------------------------------|
| ARCH-01   | 相同行为只存在一处         | 所有重复代码必须下沉到共享层                         |
| ARCH-06   | 依赖倒置，构造注入优先       | 新增模块必须面向接口，通过构造函数注入依赖                  |
| ARCH-07   | 开闭原则              | 新增功能通过新增类，不修改稳定核心                      |
| ARCH-08   | 适配器隔离文件格式         | 任何文件 I/O 必须通过 IFormatAdapter           |
| ROBUST-01 | Result<T> 传播错误    | 所有可能失败的操作返回 Result<T>，不抛异常             |
| ROBUST-02 | 禁止静默吞掉 catch      | 捕获必须记录日志或返回错误                          |
| ROBUST-03 | try-catch 仅限第三方边界 | 业务逻辑不使用 try-catch                      |
| CONCUR-01 | IO/计算 (>50ms) 异步  | 长操作使用 AsyncTask 或 QtConcurrent         |
| CONCUR-02 | 禁止 processEvents  | 使用异步 + 信号                              |
| CONCUR-03 | UI 线程安全           | 后台线程通过 QMetaObject::invokeMethod 操作 UI |
| INFRA-01  | PathUtils 统一路径    | 禁止 path.string()，统一用 PathUtils         |
| INFRA-02  | RAII 资源管理         | 禁止裸 new/delete                         |
| INFRA-03  | ServiceLocator 限制 | 仅注册全局服务，不注册页面级资源                       |
| INFRA-04  | SettingsKey<T> 集中 | 所有配置键在 Keys.h 中声明，强类型                  |

### 2.2 补充准则

以下准则基于 C++/Qt 项目最佳实践补充，作为本次重构的技术红线：

#### CD-01：PIMPL 模式（推荐用于新公共 API）

新编写的公共头文件（特别是跨模块暴露的 UI 组件）推荐使用 PIMPL（d-pointer）模式隐藏 Qt 实现细节，减少编译依赖和 ABI 脆弱性。*
*已有代码不做追溯性改造**。

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

#### CD-05：命令查询分离（CQS，推荐性）

修改状态的方法（命令）不应返回值；返回值的方法（查询）不应修改状态。**但允许返回 `Result<T>` 传播错误**——CQS 不适用于错误码场景。

```cpp
// 推荐
void loadFile(const QString &path);  // 命令：修改状态
bool isLoaded() const;               // 查询：返回状态

// 允许：需要传播错误时，Result<T> 作为异常替代
Result<void> loadFile(const QString &path);
Result<int> computeValue() const;    // 纯计算查询可以返回 Result
```

#### CD-06：强类型封装

领域概念使用 `enum class` 或包装类型，不使用裸 `int`/`double` 传递领域语义。已有的 `SettingsKey<T>` 和 `TimePos`
是良好范例，应推广到其他领域概念。

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

| ID      | 问题                                                                 | 严重度 | 来源 |
|---------|--------------------------------------------------------------------|-----|----|
| ARCH-D1 | DsPitchDocument 和 DsTextDocument 仍为并行双模型，DsTextDocBridge 已实现但效率未验证 | 中   | 探索 |
| ARCH-D2 | test-shell 与 ds-labeler/label-suite 共享大量 UI 组件但仍为独立 target，构建配置可精简 | 低   | v2 |
| ARCH-D3 | unified-editor 目录不存在，LabelSuite + DsLabeler 仍为两个独立 GUI 应用           | 中   | 探索 |
| ARCH-D4 | dstools-domain 中 F0Curve/CurveTools 与 pipeline 处理逻辑耦合，应梳理职责边界      | 低   | 探索 |
| ARCH-D5 | ~~IFormatAdapter 的 kInterfaceVersion 未在运行时校验~~ → 已实现 (ServiceLocator/TaskProcessorRegistry 注册时检查) | ~~低~~ | 探索 |
| ARCH-D6 | dsfw-core 直接编译 infer-common 源文件（OnnxEnv, OnnxModelBase），违反分层原则     | 低   | 审计 |
| ARCH-D7 | dsfw-ui-core PRIVATE 包含 widgets 头文件路径，存在循环包含风险                     | 低   | 审计 |

### 3.2 配置债（CONFIG）

> **大幅改善**: CONFIG-D1~D3 已在代码中通过 ChartConfigRegistry 解决；CONFIG-D4 通过 SettingsKey 解决；
> ChartConfigRegistry setValue() 持久化（C-06）已实现。

| ID        | 问题                                                              | 严重度 | 来源 |
|-----------|-----------------------------------------------------------------|-----|----|
| CONFIG-D5 | AudioVisualizerContainer::addChart() 的 heightWeight/stretchFactor 在多个编辑器中分散硬编码（如口型 0.5、频谱 5.0、钢琴卷帘 3.0），缺少集中默认配置 | 低 | 审计 |

### 3.3 代码债（CODE）

> **部分解决**: CODE-D5(AsyncTask 超时), CODE-D6(PipelineRunner 前置条件) 已实现。

| ID      | 问题                                                                             | 严重度 | 来源 |
|---------|--------------------------------------------------------------------------------|-----|----|
| CODE-D1 | DsTextTypes.h 中版本号使用 CMake 生成的 Version.h（DSTOOLS_DOMAIN_VERSION），但 CMake 变量未定义导致版本号为空 | 低 | v2 |
| CODE-D3 | dstools-audio 中 AudioDecoder.cpp 使用 FFmpeg C API，部分路径处理未通过 PathUtils           | 中   | 探索 |
| CODE-D4 | dstools-domain 中 CurveTools.cpp/F0Curve.cpp 的 F0 插值算法有边界条件未处理（空 curve、单点曲线）    | 中   | 探索 |
| CODE-D5 | ~~AsyncTask 的 cancel 机制缺乏统一的超时策略~~ → 已实现 (kDefaultTimeout=30s)                  | ~~低~~ | 探索 |
| CODE-D6 | ~~PipelineRunner 对 Step 的 propagateDirty 实现中，依赖声明式接口但未运行时校验前置条件~~ → 已实现       | ~~低~~ | 探索 |

### 3.4 数据安全债（SAFETY）

> **已解决**: SAFETY-D1~D3, D5 已在 Phase A 中完成：
> - D1: AtomicFileWriter 统一所有写路径（A-01）
> - D2: ProjectBackupManager 自动备份（A-02）
> - D3: DsTextDocument::validate() + DsProject::validateSliceConsistency() + PipelineContext::checkPreconditions()（A-03）
> - D5: DsProject::validateExternalPaths()（A-05）

| ID        | 问题                                                              | 严重度 | 来源 |
|-----------|-----------------------------------------------------------------|-----|----|
| SAFETY-D4 | 外部文件导入（TextGrid/CSV/Lab）时，错误格式的文件可能产生部分损坏的内部数据，缺乏事务性导入（全成功或全失败） | 中   | 探索 |

### 3.5 测试债（TEST）

> **已解决**: TEST-D1~D5 已在 Phase B 中完成：
> - D1: TestPathUtils.cpp（B-01）
> - D2: TestAtomicFileWriterSafety.cpp（B-02）
> - D3: TestPipelineRunner.cpp（B-03）
> - D4: TestFormatAdapters.cpp（B-04）
> - D5: TestAudioVisualizerContainer.cpp（B-05）

| ID      | 问题                                                    | 严重度 | 来源 |
|---------|-------------------------------------------------------|-----|----|
| TEST-D6 | 现有测试覆盖率约 55%（Phase B 新增 7 套测试后），领域层和应用层仍偏低              | 中   | 审计 |

### 3.6 文档债（DOC）

| ID     | 问题                                                         | 严重度 | 来源 |
|--------|------------------------------------------------------------|-----|----|
| DOC-D1 | component-design.md 中仍有 DSFile 引用，已更正为 DsPitchDocument     | 低   | 探索 |
| DOC-D2 | overview.md 中架构图标注的 "迁移中" 状态与实际不符                          | 低   | 探索 |
| DOC-D3 | human-decisions.md 中部分代码示例使用了已废弃的 API                      | 低   | 探索 |
| DOC-D4 | docs/README.md 曾引用不存在的 refactoring-plan-v1.md，已更正为 v2 + v3 | 低   | 探索 |

---

## 4 重构阶段规划

### 4.1 Phase A：数据安全保障（优先，预计影响最大）

**目标**：确保在任何故障场景下工程数据不丢失、不损坏。

| 任务   | 内容                                                                        | 关联债       | 状态   |
|------|---------------------------------------------------------------------------|-----------|------|
| A-01 | 所有文件写入统一经过 AtomicFileWriter，移除 IFormatAdapter 中的裸 QFile 写入                | SAFETY-D1 | ✅ 完成 |
| A-02 | 实现 ProjectBackupManager：.dsproj 文件自动备份（保持最近 N 个版本，可配置），打开工程时检测并提示恢复       | SAFETY-D2 | ✅ 完成 |
| A-03 | DsTextDocument/DsPitchDocument 保存前增加字段合法性校验（必填字段非空、坐标范围合法等），非法数据拒绝保存并提示用户 | SAFETY-D3 | ✅ 完成 |
| A-04 | 外部文件导入实现事务性：先解析到临时模型，验证通过后再替换现有数据，失败则完全回滚                                 | SAFETY-D4 | 待开始  |
| A-05 | 工程打开时校验配置中引用的外部路径是否存在，不存在的路径在 UI 中醒目标记                                    | SAFETY-D5 | ✅ 完成 |
| A-06 | PathUtils 增加文件完整性校验方法（CRC32，无新增依赖），供备份和导入使用，避免引入 OpenSSL 等新依赖             | 新增        | ✅ 完成 |

**验收标准**：

- 模拟写盘断电，重启后工程文件可恢复
- 导入格式错误的 TextGrid/CSV/Lab 不产生部分数据污染
- 所有写操作路径经过 AtomicFileWriter

### 4.2 Phase B：测试补全

**目标**：L1 测试覆盖率 >= 70%，关键路径 100% 覆盖。

| 任务   | 内容                                                    | 关联债     |
|------|-------------------------------------------------------|---------|
| B-01 | PathUtils 单元测试（覆盖所有编码检测/解码/编码路径、路径规范化、文件读写）           | TEST-D1 |
| B-02 | AtomicFileWriter 单元测试（正常写入、写入中断恢复、并发写入保护）             | TEST-D2 |
| B-03 | PipelineRunner 单元测试（正常执行、步骤失败、取消、dirty 传播）            | TEST-D3 |
| B-04 | IFormatAdapter 实现独立单元测试（边界输入、错误格式、空数据）                | TEST-D4 |
| B-05 | AudioVisualizerContainer 组件级测试（hittest 精确度、resize 行为） | TEST-D5 |
| B-06 | CurveTools 单元测试（空曲线、单点、极值、噪声）                         | CODE-D4 |
| B-07 | ProjectBackupManager 单元测试（备份轮转、恢复流程）                  | A-02    |

**验收标准**：

- `ctest --output-on-failure` 全部通过
- PathUtils、AtomicFileWriter、PipelineRunner 覆盖率 >= 90%
- 测试随 CI 自动执行（如配置 CI）

### 4.3 Phase C：魔法数字配置化

**目标**：所有算法参数和 UI 参数从硬编码迁移到 SettingsKey<T>。

ChartConfigRegistry + AppSettings 体系已覆盖大部分配置需求：

| 面板 | 已配置化参数 | 路径 |
|------|-----------|------|
| SpectrogramChartPanel | hopSize, windowSize, minDb, maxDb | ChartConfigRegistry |
| PowerChartPanel | windowSize, minDb, maxDb | ChartConfigRegistry |
| WaveformChartPanel | loudnessRef, opacity | ChartConfigRegistry |
| PianoRollView | vScale, showPitchTextOverlay, showPhonemeTexts 等 | SettingsKey (Keys.h) |
| AudioVisualizerContainer | chartVisible, chartOrder, defaultResolution | SettingsKey (Keys.h) |

`windowType`, `melBins`, `minFreq`, `maxFreq`, `preEmphasis`, `noiseGateDb` 在当前代码中不存在，无需迁移。

| 任务   | 内容                                                                                   | 状态   | 说明 |
|------|------|------|------|
| C-01~05 | 原任务已基本完成 | ✅ 已完成 | ChartConfigRegistry + AppSettings 已覆盖 |
| C-06 | ChartConfigRegistry 持久化 | ✅ 已完成 | setValue() → saveConfig() → AppSettings::setRawString 已实现 |
| C-07 | PianoRollView 颜色常量迁移：Colors 命名空间中的颜色常量改为 SettingsKey 或 Theme 系统 | 待开始 | 可选，低优先级 |

**验收标准（更新）**：

- ChartConfigRegistry 配置变更跨会话持久化（C-06 ✅ 已满足）
- 所有图表内无需要用户修改代码才能调节的算法参数（已满足）

### 4.4 Phase D：代码统一与清理

**目标**：消除重复代码，统一行为。

| 任务   | 状态   | 内容                                                                               | 说明 |
|------|------|----------------------------------------------------------------------------------|------|
| D-01 | 待开始  | 评估 test-shell 与 ds-labeler 合并可行性，或精简为统一 CLI 的子命令模式               | ARCH-D2, 低优先级 |
| D-03 | 暂缓   | dstools-audio 中 AudioDecoder.cpp 路径处理统一使用 PathUtils                       | 当前 `path.toUtf8().toStdString()` 在 FFmpeg C API 下功能正确 |
| D-04 | ✅ 完成 | PipelineRunner 增加 Step 前置条件运行时校验（preconditions 方法）                    | PipelineContext::checkPreconditions() 已在 PipelineRunner.cpp:94 调用 |
| D-05 | ✅ 完成 | AsyncTask 增加统一超时策略（默认 30s，可配置）                                        | kDefaultTimeout=30s, hasTimedOut(), setTimeout(), requestCancel() 均已实现 |
| D-07 | 待开始  | 所有公共类标记 noexcept（移动构造、swap、简单 getter）                                  | 新增 |
| D-08 | 待开始  | 所有公共类标记 const（不修改状态的成员函数）                                           | 新增 |

**验收标准**：

- test-shell 与主应用合并或功能被覆盖
- `grep -r "path.string()" src/` 返回零结果（排除第三方代码）
- 公共 API noexcept/const 覆盖率 >= 90%（基于 clang-tidy 扫描）

### 4.5 Phase E：架构演进

**目标**：清理架构债，简化模块依赖。

| 任务   | 状态   | 内容                                                                                    | 关联债       |
|------|------|---------------------------------------------------------------------------------------|-----------|
| E-01 | 待开始  | 评估 DsPitchDocument + DsTextDocument 合并可行性：分析两个模型的重叠度，>60% 则合并为统一文档模型                  | ARCH-D1   |
| E-02 | 待开始  | 创建 unified-editor 目录结构：迁移 LabelSuite + DsLabeler 共有的 AppShell 配置到该目录，减少重复                | ARCH-D3   |
| E-03 | 待开始  | dstools-domain 中 F0Curve/CurveTools 与 pipeline 逻辑解耦，梳理与 dsfw-signal（curve_tools）的职责边界 | ARCH-D4   |
| E-04 | ✅ 完成 | IFormatAdapter 运行时版本校验：在 ServiceLocator/TaskProcessorRegistry 注册时检查 kInterfaceVersion | ARCH-D5   |
| E-05 | 待开始  | 设置 CMake 版本变量（DSTOOLS_DOMAIN_VERSION_MAJOR/MINOR/PATCH），使生成的 Version.h 包含正确版本号 | CODE-D1   |
| E-06 | 待开始  | 全面审计并移除所有未使用的头文件 include                                                              | 新增        |
| E-07 | 待开始  | 将 infer-common（OnnxEnv, OnnxModelBase）拆分为独立 target 或移入 infer 层，消除 dsfw-core 的反向依赖     | ARCH-D6   |
| E-08 | 暂缓   | 消除 dsfw-ui-core 对 widgets 头文件的循环包含风险                                                  | ARCH-D7   |
| E-09 | 已完成  | 修复音素右键播放选择性失败：修复边界区间计算逻辑、统一时间单位精度、添加播放范围校验                                            | 新增        |
| E-10 | 待开始  | 音频解码流式化：将 AudioDecoder::decodeAll() 改为按需流式解码，支持大文件高效 seek                             | 新增        |
| E-11 | 已完成  | 播放管线错误处理：播放范围无效时通过 UI 反馈（状态栏/气泡），记录详细诊断日志                                             | ROBUST-01  |

**验收标准**：

- DsPitchDocument + DsTextDocument 重叠度分析报告完成
- unified-editor 目录已创建，有实际内容和独立 CMakeLists.txt
- `#include` 依赖图比当前减少 20%+
- 所有音素区间右键播放均可正常出声，无选择性失败
- AudioDecoder 支持流式解码，seek 效率提升 50%+

#### 4.5.1 音素右键播放选择性失败：根因分析与解决方案

**问题现象**：在 PhonemeEditor 中右键点击音素区间，部分音素可以正常播放，部分音素始终无声，且失败模式稳定（同一音素每次测试结果一致）。

**涉及文件与调用链**：

```
IntervalTierView::mousePressEvent (右键)
  → hitTestInterval(x) → 计算区间索引
  → emit requestPlayback(startTime, endTime)  [TimePos, 微秒]
    → PhonemeEditor::connectSignals lambda
      → usToSec(startTime), usToSec(endTime)  [double, 秒]
      → PlayWidget::setPlayRange(startSec, endSec)
      → PlayWidget::seek(startSec)
      → PlayWidget::setPlaying(true)
```

**根因 1：IntervalTierView::hitTestInterval 像素精度不足**

[IntervalTierView.cpp](file:///d:/projects/dataset-tools/src/apps/shared/phoneme-editor/ui/IntervalTierView.cpp) 中
`hitTestInterval` 将区间时间映射为像素坐标后判断点击位置。当区间宽度 < 1px 时强制设为 1px：

```cpp
if (x2 - x1 < 1) x2 = x1 + 1;
```

**问题**：当多个极短区间挤在同一像素范围内时，`x2 = x1 + 1` 导致多个区间共享同一像素区间，hitTest
可能返回错误的区间索引，从而导致播放错误的区间范围。

**根因 2：PhonemeEditor::playBoundarySegment 区间计算边界条件错误**

[PhonemeEditor.cpp](file:///d:/projects/dataset-tools/src/apps/shared/phoneme-editor/PhonemeEditor.cpp) 中
`playBoundarySegment` 遍历边界点寻找当前时间所在的区间：

```cpp
for (int b = 0; b < count; ++b) {
    double t = m_document->boundaryTime(activeTier, b);
    if (t <= timeSec) segStart = t;
    if (t > timeSec) { segEnd = t; break; }
}
```

**问题**：

- 当 `timeSec` 恰好等于第一个边界时间时，`segStart` 更新为 `t`，但 `t > timeSec` 为 false，`segEnd` 保持 `totalDuration()`
- 当 `timeSec` 位于最后一个边界之后时，`segEnd` 保持 `totalDuration()`，`segStart` 为最后一个边界 → 这是一个有效区间
- 但当 `timeSec` 位于最后一个边界之前且区间为 0 长度时（`segStart == segEnd`），`PlayWidget` 播放 0 秒 → 无声

**根因 3：usToSec 浮点精度损失**

[TimePos.h](file:///d:/projects/dataset-tools/src/types/include/dstools/TimePos.h) 中：

```cpp
inline double usToSec(TimePos us) {
    return static_cast<double>(us) / 1e6;
}
```

`TimePos` 是 `int64_t`（微秒），转换为 `double` 时有效位数为 53 位（约 15-16 位十进制有效数字）。当 `TimePos` 值较大（如 3600
秒 = 3.6e9 微秒）时，`double` 的精度约为 0.4 微秒级别，仍然足够。但问题是：**在 `PlayWidget::setPlayRange` 中，start 和 end
经过转换后可能因浮点舍入导致 start >= end**（当区间本身极短时）。

**根因 4：PlayWidget 播放范围 clamp 无声**

[PlayWidget.cpp](file:///d:/projects/dataset-tools/src/framework/widgets/src/PlayWidget.cpp) 中：

```cpp
if (m_hasRange && posSec >= m_rangeEnd) {
    m_player->stop();
    reloadButtonStatus();
}
```

当 `m_rangeStart >= m_rangeEnd`（由根因 2/3 导致）时，`posSec = std::clamp(posSec, m_rangeStart, m_rangeEnd)` 将 posSec 设为
`m_rangeEnd`，立刻触发停止条件 → 完全无声。**这是最终表现层根因**。

**根因 5：AudioDecoder 全量解码 + 字节位置 seek 误差**

[AudioDecoder.cpp](file:///d:/projects/dataset-tools/src/framework/audio/src/AudioDecoder.cpp) 中 `decodeAll()`
将整个音频文件解码到内存缓冲区，`setPosition` 通过字节偏移计算位置：

```cpp
qint64 pos = static_cast<qint64>(sec * fmt.averageBytesPerSecond());
m_playback->decoderSetPosition(pos);
```

**问题**：

- `averageBytesPerSecond()` 是平均值，对 VBR（可变比特率）编码的文件不精确
- 对于压缩格式（如 MP3），字节位置与时间位置不是线性关系，`setPosition` 可能定位到错误位置
- 如果 seek 位置超出解码缓冲区，播放器可能输出静音

**架构缺陷总结**：

| 缺陷          | 违反准则      | 说明                                       |
|-------------|-----------|------------------------------------------|
| 播放管道无范围校验   | ROBUST-01 | 无效的 start/end 值静默传递到 PlayWidget，无声失败无反馈  |
| 时间单位不统一     | INFRA-05  | TimePos(μs) ↔ double(秒) 在多层间转换，精度不一致     |
| UI 层直接操作音频层 | ARCH-06   | TierEditWidget 信号直接驱动 PlayWidget，缺少中间校验层 |
| 全量解码模式      | CONCUR-01 | 大文件全部解码到内存，既浪费内存又不能高效 seek               |
| 无错误传播       | ROBUST-02 | 播放失败时无任何日志或用户提示，完全静默                     |

**E-09 修复方案**：

1. 在 `PhonemeEditor::playBoundarySegment` 中添加区间有效性校验：`segStart < segEnd`，无效时返回并记录日志
2. 统一播放管线使用 `TimePos`（微秒），仅在 `PlayWidget` 边界转换为秒，消除中间层精度损失
3. 在 `PlayWidget::setPlayRange` 中添加 `start < end` 断言 + 日志，无效范围直接拒绝
4. 修复 `IntervalTierView::hitTestInterval` 的区间定位逻辑，使用二分查找替代线性扫描，处理重叠区间

**E-10 修复方案**：

1. 将 `AudioDecoder::decodeAll()` 改为流式解码：不解码全部文件，而是在 seek 时按需解码目标区间
2. 引入 `StreamingAudioDecoder` 类，支持 `seekToTime(double sec)` + `readSamples(size_t count)` 按需读取
3. 对于 VBR 格式，构建 seek table（时间→字节偏移映射表）以支持精确定位
4. 保持向后兼容：提供 `decodeAll()` 作为便捷方法，内部委托给流式解码器

**E-11 修复方案**：

1. 在播放管线关键节点添加 `Result<void>` 校验：`setPlayRange` → `seek` → `setPlaying` 每一步检查前置条件
2. 无效播放范围时通过 `QStatusBar` 或 Toast 通知用户："当前区间长度为 0，无法播放"
3. 添加 `dsfw::LogCategory::audio` 日志分类，记录所有播放操作的详细参数（范围、seek 位置、解码状态）
4. 在 `PlayWidget` 中添加 `playbackError` 信号，供上层 UI 订阅并展示错误信息

---

## 5 实施约束与风险控制

### 5.1 不可变原则

| 原则   | 说明                                                      |
|------|---------------------------------------------------------|
| 行为一致 | 所有重构不改变任何用户可见行为，回归测试必须 100% 通过                          |
| 数据安全 | Phase A 必须在其他阶段之前完成，确保后续重构有安全网                          |
| 增量提交 | 每个子任务独立提交（不推送），确保可单独回滚                                  |
| 无新依赖 | 不引入项目未使用的外部库，仅使用已有技术栈（Qt6, nlohmann_json, ONNX Runtime） |
| 文档同步 | 每个 Phase 完成后更新相关设计文档                                    |

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

| 风险                           | 影响 | 可能性 | 缓解措施                                       |
|------------------------------|----|-----|--------------------------------------------|
| 重构引入数据损坏                     | 高  | 低   | Phase A 先行建立安全网                            |
| 测试覆盖不足导致回归                   | 高  | 中   | Phase B 测试先行，关键路径 100% 覆盖                  |
| 性能回归                         | 中  | 低   | 每个 Phase 后基准测试                             |
| 文档与实际不一致                     | 低  | 中   | 每阶段结束更新文档                                  |
| 编译时间增长                       | 低  | 低   | PIMPL + 前向声明减少头文件依赖                        |
| DsPitchDocument 合并失败         | 中  | 中   | 先分析重叠度，<60% 则不合并，仅优化桥接器                    |
| infer-common 耦合在 dsfw-core 中 | 中  | 高   | Phase E-07 拆分，当前已通过 target_sources 拉入不影响接口 |
| dsfw-ui-core 与 widgets 循环包含  | 低  | 中   | Phase E-08 消除，当前仅 PRIVATE include 无运行时问题   |
| 音素播放选择性失败                    | 中  | 高   | Phase E-09~E-11 修复，当前影响所有含短音素区间的音频文件       |

---

## 6 统一底层接口规范

### 6.1 路径与编码（PathUtils，已完成）

所有跨平台路径操作通过 `dsfw::PathUtils`：

| 方法                        | 用途                                |
|---------------------------|-----------------------------------|
| `toStdPath / fromStdPath` | QString <-> std::filesystem::path |
| `toUtf8 / toWide`         | path -> string/wstring            |
| `join`                    | 路径拼接（跨平台分隔符）                      |
| `normalize`               | 规范化路径                             |
| `openFile`                | 二进制安全的跨平台文件打开                     |
| `detectTextEncoding`      | 自动检测文本编码（BOM + 启发式）               |
| `decodeText / encodeText` | 文本编解码                             |
| `readFile / writeFile`    | 自动检测编码的文本文件读写                     |
| `canonicalOrNull`         | 规范化路径（不存在返回 nullopt）              |
| `isSubPath`               | 子路径判断                             |
| `relativeTo`              | 相对路径计算                            |

**规范**：

- `std::filesystem::path` 用于内部路径存储和传递
- `QString` 用于 Qt UI 显示，通过 `fromStdPath/toStdPath` 转换
- 禁止 `path.string()` 和 `path.wstring()` 直接调用，统一通过 PathUtils

### 6.2 文件选择（FileDialogHelper + PathSelector，已完成）

| 组件                 | 用途                   |
|--------------------|----------------------|
| `FileDialogHelper` | 跨平台文件对话框封装（读写分别提供方法） |
| `PathSelector`     | 行编辑器 + 浏览按钮组合，用于设置页面 |
| `FilePathSelector` | 路径下拉选择器，带历史记录        |

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

### 6.5 命名空间现状

当前项目存在 namespace 不统一的问题，整理如下以供后续重构参考：

| 库              | CMake Target                   | namespace                                     | include 前缀                    |
|----------------|--------------------------------|-----------------------------------------------|-------------------------------|
| dstools-types  | `dstools-types::dstools-types` | `dstools`                                     | `<dstools/Result.h>`          |
| dsfw-base      | `dsfw::base`                   | `dstools`                                     | `<dsfw/JsonHelper.h>`         |
| dsfw-signal    | `dsfw::signal`                 | `dsfw::signal`                                | `<dsfw/signal/curve_tools.h>` |
| dsfw-core      | `dsfw::core`                   | **混合** `dstools` + `dsfw`                     | `<dsfw/AppSettings.h>`        |
| dsfw-ui-core   | `dsfw::ui-core`                | `dsfw` + `dsfw::widgets` + `dstools` + `dstools::labeler` | `<dsfw/AppShell.h>`           |
| dstools-audio  | `dstools::audio`               | `dstools::audio`                              | `<dstools/AudioDecoder.h>`    |
| dsfw-widgets   | `dsfw::widgets`                | `dsfw::widgets`                               | `<dsfw/widgets/PlayWidget.h>` |
| dstools-domain | `dstools-domain`               | `dstools`                                     | `<dstools/DsDocument.h>`      |

**问题**：`dsfw-core` 内部同时使用 `dstools::` 和 `dsfw::` 命名空间。例如 `CommonKeys.h` 在 `namespace dsfw::CommonKeys`
中使用 `dstools::SettingsKey<T>`。建议在 Phase E 后统一规划命名空间迁移策略。

---

## 7 任务清单与进度

### Phase A+B：数据安全保障 + 测试补全（已完成）

> **全部完成**（15/15 任务），详见 [1.4 已完成的先行成果](#14-已完成的先行成果)。
> 核实日期 2026-06-03，所有实现经代码审查确认，功能正确。

### Phase C：魔法数字配置化

> **已完成**。ChartConfigRegistry + AppSettings 已覆盖各面板主要参数并持久化。

| 任务   | 状态   | 说明 |
|------|------|------|
| C-01~06 | ✅ 完成 | ChartConfigRegistry + AppSettings 覆盖 Spectrogram/Power/Waveform/PianoRoll/AudioVisualizer；setValue() → saveConfig() → AppSettings 持久化已实现 |
| C-07 | 待开始 | PianoRollView Colors 颜色常量迁移到 SettingsKey 或 Theme（可选，低优先级） |

### Phase D：代码统一与清理

| 任务   | 状态   | 说明 |
|------|------|------|
| D-01 | 待开始  | test-shell 合并评估 |
| D-03 | 暂缓   | AudioDecoder 路径处理 |
| D-04 | ✅ 完成 | PipelineRunner checkPreconditions() |
| D-05 | ✅ 完成 | AsyncTask 超时/取消机制 (kDefaultTimeout=30s) |
| D-07 | 待开始  | 公共类 noexcept 标记 |
| D-08 | 待开始  | 公共类 const 标记 |

### Phase E：架构演进

| 任务   | 状态   | 说明 |
|------|------|------|
| E-01 | 待开始  | DsPitchDocument/DsTextDocument 合并评估 |
| E-02 | 待开始  | 创建 unified-editor 目录 |
| E-03 | 待开始  | F0Curve/CurveTools 解耦 |
| E-04 | ✅ 完成 | IFormatAdapter 版本校验（ServiceLocator/TaskProcessorRegistry 注册时检查） |
| E-05 | 待开始  | 版本号 CMake 生成 |
| E-06 | 待开始  | 头文件审计 |
| E-07 | 待开始  | infer-common 拆分 |
| E-08 | 暂缓   | dsfw-ui-core 循环包含 |
| E-09 | 已完成  | 音素播放修复 |
| E-10 | 待开始  | AudioDecoder 流式化 |
| E-11 | 已完成  | 播放管线错误处理 |

### Phase F：图表渲染架构统一（重大重构）

> 详细方案见 [附录 B](#b-附录图表绘制架构统一重构方案)。

| 任务   | 状态   | 说明 | 风险 |
|------|------|------|------|
| F-01 | 待开始  | ChartPanelBase 基类接口扩展（renderFullData/dataDurationSec） | **低** - 仅新增虚方法 |
| F-02 | 待开始  | WaveformChartPanel 先行迁移 | **中** - 验证可行性 |
| F-03 | 待开始  | PowerChartPanel + MouthCurveChartPanel 迁移 | **中** - 修复 MouthCurve 缩放 bug |
| F-04 | 待开始  | SpectrogramChartPanel 迁移 | **中** - FFT 懒计算兼容 |
| F-05 | 待开始  | 基类 paintEvent() 接管绘制，drawContent() final | **高** - 一次修改所有图表 |
| F-06 | 待开始  | 清理废弃代码（rebuildCache/RegionUpdate/m_pendingRegion 等） | **低** - 纯删除 |

---

## A 附录：与 v2 的差异

| 方面    | v2                                         | v3                                                  |
|-------|--------------------------------------------|-----------------------------------------------------|
| 已完成任务 | 作为待办列出                                     | 移至 1.4 已完成成果，不重复计划                                  |
| 补充准则  | 仅引用 human-decisions                        | 新增 CD-01~CD-09 补充准则                                 |
| 数据安全  | 未独立成章                                      | 独立 Phase A，最高优先级                                    |
| 测试补全  | 未明确列出具体测试目标                                | 独立 Phase B，明确每个测试覆盖的模块和验收标准                         |
| 魔法数字  | 概括性提及                                      | 独立 Phase C，按面板逐项列出                                  |
| 架构演进  | 未涉及 DsPitchDocument 合并评估、unified-editor 创建 | 独立 Phase E，明确分析先行再决定是否执行                            |
| 执行流程  | 未细化                                        | 5.2 节定义每阶段标准执行流程                                    |
| 风险矩阵  | 无                                          | 5.3 节定义具体风险和缓解措施                                    |
| 文档清理  | 无                                          | 已修正 component-design.md、overview.md、README.md 的过时引用 |

---

## B 附录：图表绘制架构统一重构方案

### B.1 问题陈述

当前所有图表虽然继承自同一基类 `ChartPanelBase`
，但各自实现了完全不同的绘制逻辑。口型曲线图（MouthCurveChartPanel）因错误的索引计算公式导致在缩放/平移时曲线消失，暴露出架构层面的根本缺陷：
**每个子类都在重复发明视口映射、缓存管理、坐标变换等基础设施，行为不一致是必然结果**。

这违背了 ARCH-01（相同行为只存在一处）和 ARCH-04（>60% 重叠应合并到基类）的设计准则。

### B.2 现状分析：五种图表绘制模式对比

#### B.2.1 绘制模式盘点

| 图表                    | 类型      | `rebuildCache()`              | `drawContent()`            | 使用 `coord`？ | 数据源                    |
|-----------------------|---------|-------------------------------|----------------------------|-------------|------------------------|
| WaveformChartPanel    | 1D 波形   | ✅ 全量+增量 `m_yMax/m_yMin/m_rms` | 遍历缓存绘制波形                   | ❌ Q_UNUSED  | `m_samples` (音频)       |
| PowerChartPanel       | 1D 功率   | ✅ 全量+增量 `m_powerCache`        | 遍历缓存绘制功率曲线                 | ❌ Q_UNUSED  | `m_samples` (音频)       |
| SpectrogramChartPanel | 2D 频谱   | ✅ 全量+增量 `m_viewImage`         | 绘制 `m_viewImage`           | ❌ Q_UNUSED  | `m_samples` → FFT      |
| MouthCurveChartPanel  | 1D 曲线   | ❌ 空实现                         | 实时计算索引，**公式错误**            | ❌ Q_UNUSED  | `m_curve` (TimeSeries) |
| PianoRollChartPanel   | 2D 钢琴卷帘 | ❌ 委托子控件                       | 委托 `PianoRollView::render` | ✅ 传入 coord  | DsPitchDocument        |

#### B.2.2 核心问题

1. **视口映射逻辑重复**：Waveform、Power、Spectrogram 各自独立计算 `m_converter->startSec/endSec` → 像素映射，同一逻辑在 3
   个文件中重复
2. **`drawContent(coord)` 参数形同虚设**：4 个图表 `Q_UNUSED(coord)`，直接读取 `m_converter` 成员变量，`coord` 参数从未被真正使用
3. **`rebuildCache()` 不强制**：MouthCurve 空实现，PianoRoll 不需要，基类未提供默认实现
4. **两阶段渲染不一致**：Waveform/Power/Spectrogram 采用"缓存→绘制"，MouthCurve 采用"实时计算"
5. **关注点未分离**：视口数学、幅度缩放、数据渲染混杂在子类中，无法复用

#### B.2.3 口型曲线 Bug 的根因

问题位于 [MouthCurveChartPanel.cpp:L91-L93](file:///d:/projects/dataset-tools/src/apps/shared/mouth-curve-chart/MouthCurveChartPanel.cpp#L91-L93)：

```cpp
double t = m_converter->startSec() +
           (m_converter->endSec() - m_converter->startSec()) * col / cols;
int curveIdx = static_cast<int>(t * m_curve.values.size() /
    (m_converter->endSec() - m_converter->startSec()));  // BUG: 分母应为 dataDuration
```

`curveIdx` 分母使用了**视口时长**而非**数据总时长**。当视口从 0~10s 缩放到 2~4s 时，`curveIdx` 全部越界（500~1000），曲线完全消失。

### B.3 统一架构设计

#### B.3.1 核心思想

**子类负责渲染"完整数据图像"，基类负责视口裁剪、变换、缓存管理。**

```
┌─────────────────────────────────────────────────┐
│                 ChartPanelBase                   │
│  ┌───────────────────────────────────────────┐  │
│  │  paintEvent()                             │  │
│  │  1. 检查 m_fullDataDirty → 调用 renderFullData() │
│  │  2. 计算 srcRect = viewport 在完整图上的区域  │  │
│  │  3. 应用垂直缩放 transform                   │  │
│  │  4. painter.drawImage(destRect, image, srcRect) │
│  │  5. 绘制边界叠加层、Y 轴                      │  │
│  └───────────────────────────────────────────┘  │
│                                                 │
│  管理的成员:                                     │
│  - QImage m_fullDataImage     (缓存)            │
│  - bool m_fullDataDirty       (脏标记)           │
│  - m_converter, m_amplitudeScale               │
│                                                 │
│  纯虚接口 (子类实现):                            │
│  - renderFullData(QImage&)    渲染完整数据到图像   │
│  - dataDurationSec() const    数据总时长(秒)     │
│                                                 │
│  可选虚接口 (子类可覆盖):                         │
│  - fullDataImageWidth()  const  默认: width()  │
│  - fullDataImageHeight() const  默认: height() │
└─────────────────────────────────────────────────┘
         ↑                    ↑
    ┌────┴────┐          ┌────┴────┐
    │Waveform │          │MouthCurve│  ...
    │renderFullData()    │renderFullData()
    │按列计算min/max     │按列查曲线值
    │dataDurationSec()   │dataDurationSec()
    └─────────┘          └─────────┘
```

#### B.3.2 关键设计决策

**决策 1：视口变化不触发缓存重建**

当前架构中 `onViewportUpdate()` 设置 `m_cacheDirty = true`，导致每次缩放/平移都重建缓存。新架构中，完整图像已涵盖所有数据，视口变化仅改变
`srcRect` 的裁剪区域，无需重建。这消除了视口操作的性能开销。

**决策 2：`drawContent()` 从虚函数变为基类 final 实现**

所有子类不再需要 `drawContent()` 覆盖。基类统一实现：从 `m_fullDataImage` 中裁剪视口区域，绘制到 `chartContentRect()`
。子类只需实现 `renderFullData()`。

**决策 3：移除 `RegionUpdate` 和增量重建**

由于完整图像一次性渲染，不再需要 `RegionUpdate` 结构体、`m_pendingRegion` 成员、`shiftCache()` 模板方法。滚动操作简化为改变
`srcRect` 的 `srcX` 偏移。

**决策 4：PianoRollChartPanel 保持委托模式**

PianoRollChartPanel 内部使用 `PianoRollView` 子控件，该控件有自己的 viewport 和渲染管线。它通过 `renderFullData()` 委托给
`PianoRollView::render()`，但 PianoRollView 的内部实现不在本次重构范围内。

#### B.3.3 基类接口变更

```cpp
// ChartPanelBase.h — 新接口

class ChartPanelBase : public QWidget, public IChartPanel {
protected:
    // ========== 新增纯虚方法 ==========

    // 渲染完整数据到 QImage。子类决定分辨率。
    // 仅在数据变更或 widget 尺寸变更时调用，不在视口变更时调用。
    virtual void renderFullData(QImage& image) = 0;

    // 数据总时长（秒）
    virtual double dataDurationSec() const = 0;

    // ========== 新增可选虚方法 ==========

    // 完整数据图像的宽度（像素）。默认: widget 宽度。
    virtual int fullDataImageWidth() const { return width(); }

    // 完整数据图像的高度（像素）。默认: widget 高度。
    virtual int fullDataImageHeight() const { return height(); }

    // ========== 移除的虚方法 ==========

    // rebuildCache(const RegionUpdate&) — 移除，由 renderFullData() 替代
    // drawContent(QPainter&, const ChartCoordinate&) — 移除，基类实现

    // ========== 新增成员 ==========

    QImage m_fullDataImage;
    bool m_fullDataDirty = true;

    // ========== 修改的方法 ==========

    void onViewportUpdate(const ChartCoordinate& conv, int pixelWidth) override {
        m_converter = &conv;
        m_dataPixelWidth = pixelWidth;
        // 不再设置 m_cacheDirty = true
        // 视口变化仅改变裁剪区域，不触发缓存重建
        update();
    }

    void resizeEvent(QResizeEvent* event) override {
        m_dataPixelWidth = width();
        m_fullDataDirty = true;  // 图像尺寸可能变化
        QWidget::resizeEvent(event);
    }

    void paintEvent(QPaintEvent*) override {
        ensureFullDataCache();
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, false);

        auto cr = chartContentRect();
        if (cr.width() <= 0 || cr.height() <= 0) return;

        painter.save();
        painter.setClipRect(cr);

        if (!m_fullDataImage.isNull() && m_converter) {
            double totalDur = dataDurationSec();
            if (totalDur > 0.0) {
                double fracStart = m_converter->startSec() / totalDur;
                double fracEnd = m_converter->endSec() / totalDur;
                int srcX = static_cast<int>(fracStart * m_fullDataImage.width());
                int srcW = std::max(1,
                    static_cast<int>((fracEnd - fracStart) * m_fullDataImage.width()));
                QRect srcRect(srcX, 0, srcW, m_fullDataImage.height());

                if (supportsVerticalZoom() && m_amplitudeScale != 1.0) {
                    painter.save();
                    painter.translate(0, cr.height() / 2.0);
                    painter.scale(1.0, m_amplitudeScale);
                    painter.translate(0, -cr.height() / 2.0);
                    painter.drawImage(cr, m_fullDataImage, srcRect);
                    painter.restore();
                } else {
                    painter.drawImage(cr, m_fullDataImage, srcRect);
                }
            }
        }

        ChartCoordinate coord;
        if (m_converter) {
            coord.viewStart = m_converter->startSec();
            coord.viewEnd = m_converter->endSec();
            coord.pixelWidth = m_dataPixelWidth;
        }
        paintOutOfBoundsOverlay(painter, cr, coord);
        painter.restore();

        QRect axisRect(0, kYAxisPadding, yAxisWidth(), height() - 2 * kYAxisPadding);
        if (axisRect.width() > 0) {
            painter.save();
            painter.setClipRect(axisRect);
            paintYAxisContent(painter, axisRect);
            painter.restore();
        }
    }

    void ensureFullDataCache() {
        if (!m_fullDataDirty) return;
        int imgW = fullDataImageWidth();
        int imgH = fullDataImageHeight();
        if (imgW > 0 && imgH > 0) {
            m_fullDataImage = QImage(imgW, imgH, QImage::Format_ARGB32);
            m_fullDataImage.fill(Qt::transparent);
            renderFullData(m_fullDataImage);
        }
        m_fullDataDirty = false;
    }
};
```

#### B.3.4 各子类适配方案

##### WaveformChartPanel

```cpp
// 数据变更时标记脏
void WaveformChartPanel::onAudioDataChanged() override {
    m_fullDataDirty = true;
}

double WaveformChartPanel::dataDurationSec() const override {
    return m_sampleRate > 0
        ? static_cast<double>(m_samples.size()) / m_sampleRate
        : 0.0;
}

int WaveformChartPanel::fullDataImageWidth() const override {
    return width() * 4;  // 4x 超采样提高质量
}

void WaveformChartPanel::renderFullData(QImage& image) override {
    if (m_samples.empty()) return;
    int w = image.width();
    int h = image.height();
    int h2 = h / 2;
    double totalDur = dataDurationSec();
    if (totalDur <= 0.0) return;

    QPainter p(&image);
    p.setPen(QPen(m_waveColor, 1));

    for (int col = 0; col < w; ++col) {
        double tStart = totalDur * col / w;
        double tEnd = totalDur * (col + 1) / w;
        int sStart = std::max(0, static_cast<int>(tStart * m_sampleRate));
        int sEnd = std::min(static_cast<int>(m_samples.size()),
                            static_cast<int>(tEnd * m_sampleRate));
        if (sStart >= sEnd) continue;

        float yMax = 0.0f, yMin = 0.0f;
        for (int s = sStart; s < sEnd; ++s) {
            float val = static_cast<float>(m_samples[s]);
            yMax = std::max(yMax, val);
            yMin = std::min(yMin, val);
        }

        int yTop = h2 - static_cast<int>((yMax + 1.0f) * 0.5f * h2);
        int yBot = h2 - static_cast<int>((yMin + 1.0f) * 0.5f * h2);
        p.drawLine(col, yTop, col, yBot);
    }
    p.end();
}
```

**移除**：`rebuildCache()`、`drawContent()`、`drawWaveform()`、`rebuildWaveformCache()`、`rebuildWaveformCachePartial()`、
`m_yMax`、`m_yMin`、`m_rms`、`m_loudnessMask`

##### PowerChartPanel

```cpp
double PowerChartPanel::dataDurationSec() const override {
    return m_sampleRate > 0
        ? static_cast<double>(m_samples.size()) / m_sampleRate
        : 0.0;
}

void PowerChartPanel::renderFullData(QImage& image) override {
    if (m_samples.empty()) return;
    int w = image.width();
    int h = image.height();
    double totalDur = dataDurationSec();
    if (totalDur <= 0.0) return;

    QPainter p(&image);
    QPainterPath path;
    bool started = false;

    for (int col = 0; col < w; ++col) {
        double tCenter = totalDur * (col + 0.5) / w;
        int centerSample = static_cast<int>(tCenter * m_sampleRate);
        int halfWindow = m_configWindowSize / 2;
        int wStart = std::max(0, centerSample - halfWindow);
        int wEnd = std::min(static_cast<int>(m_samples.size()), centerSample + halfWindow);

        float dB = m_configMinDb;
        if (wStart < wEnd) {
            double sumSq = 0.0;
            for (int s = wStart; s < wEnd; ++s) {
                double val = m_samples[s];
                sumSq += val * val;
            }
            double rms = std::sqrt(sumSq / (wEnd - wStart));
            dB = rms < 1e-10 ? m_configMinDb
                : static_cast<float>(20.0 * std::log10(std::max(rms, 1e-10)));
            dB = std::clamp(dB, m_configMinDb, m_configMaxDb);
        }

        float norm = (dB - m_configMinDb) / (m_configMaxDb - m_configMinDb);
        int y = h - static_cast<int>(norm * h);

        if (!started) { path.moveTo(col, y); started = true; }
        else { path.lineTo(col, y); }
    }
    p.setPen(QPen(dsfw::Theme::instance().palette().audioVisualizer.powerColor, 1.5));
    p.setRenderHint(QPainter::Antialiasing, true);
    p.drawPath(path);
    p.end();
}
```

**移除**：`rebuildCache()`、`drawContent()`、`drawPower()`、`rebuildPowerCache()`、`rebuildPowerCachePartial()`、
`m_powerCache`

##### MouthCurveChartPanel

```cpp
double MouthCurveChartPanel::dataDurationSec() const override {
    return m_curve.durationSec();
}

int MouthCurveChartPanel::fullDataImageWidth() const override {
    return static_cast<int>(m_curve.values.size());  // 每个采样点一列
}

void MouthCurveChartPanel::renderFullData(QImage& image) override {
    if (m_curve.isEmpty()) return;
    int w = image.width();
    int h = image.height();

    QPainter p(&image);
    QPainterPath path;
    bool started = false;

    for (int col = 0; col < w; ++col) {
        double t = static_cast<double>(col) / w * m_curve.durationSec();
        int idx = m_curve.timeToIndex(t);
        if (idx < 0 || idx >= static_cast<int>(m_curve.values.size())) continue;

        float val = std::clamp(m_curve.values[idx], kCurveMin, kCurveMax);
        int y = h - static_cast<int>(val / (kCurveMax - kCurveMin) * h);

        if (!started) { path.moveTo(col, y); started = true; }
        else { path.lineTo(col, y); }
    }
    p.setPen(QPen(QColor(0x00, 0xDC, 0xDC), 2));
    p.setRenderHint(QPainter::Antialiasing, true);
    p.drawPath(path);
    p.end();
}
```

**移除**：`rebuildCache()`（空实现）、`drawContent()`、`paintCurve()`

##### SpectrogramChartPanel

频谱图每帧 FFT 计算开销大，`fullDataImageWidth()` 返回的宽度受 QImage
最大值（32767）限制。对于超长音频，这将导致深缩放时图像被拉伸。但作为初始实现，这是可接受的折中；未来可增加多分辨率金字塔。

```cpp
double SpectrogramChartPanel::dataDurationSec() const override {
    return m_sampleRate > 0
        ? static_cast<double>(m_samples.size()) / m_sampleRate
        : 0.0;
}

int SpectrogramChartPanel::fullDataImageWidth() const override {
    return std::min(m_totalFrames, 32767);  // QImage 最大宽度
}

int SpectrogramChartPanel::fullDataImageHeight() const override {
    return m_numFreqBins;
}

void SpectrogramChartPanel::renderFullData(QImage& image) override {
    if (m_totalFrames <= 0 || m_numFreqBins <= 0) return;

    int w = image.width();
    int h = image.height();
    double totalDur = dataDurationSec();
    if (totalDur <= 0.0) return;

    image.fill(qRgb(0, 0, 0));

    for (int x = 0; x < w; ++x) {
        double t = totalDur * (x + 0.5) / w;
        double frameIdx = (t / totalDur) * m_totalFrames;
        int frame0 = static_cast<int>(frameIdx);
        int frame1 = std::min(frame0 + 1, m_totalFrames - 1);
        if (frame0 < 0 || frame0 >= m_totalFrames) continue;

        ensureSpectrogramRange(frame0, frame1 + 1);
        if (!m_computedFrames[frame0]) continue;

        float frameFrac = static_cast<float>(frameIdx - frame0);
        const auto& bins0 = m_spectrogramData[frame0];
        const auto& bins1 = m_spectrogramData[frame1];

        for (int y = 0; y < h; ++y) {
            float binFrac = static_cast<float>(h - 1 - y) / (h - 1) * (m_numFreqBins - 1);
            int bin = static_cast<int>(binFrac);
            if (bin < 0 || bin >= m_numFreqBins) continue;

            float val0 = bins0[bin], val1 = bins1[bin];
            if (bin + 1 < m_numFreqBins) {
                float frac = binFrac - bin;
                val0 = bins0[bin] * (1.0f - frac) + bins0[bin + 1] * frac;
                val1 = bins1[bin] * (1.0f - frac) + bins1[bin + 1] * frac;
            }
            float val = val0 * (1.0f - frameFrac) + val1 * frameFrac;
            image.setPixelColor(x, y, intensityToColor(val));
        }
    }
}
```

**移除**：`rebuildCache()`、`drawContent()`、`rebuildViewImage()`、`rebuildViewImagePartial()`、`fillImageColumns()`、
`m_viewImage`、`m_pendingRegion`

##### PianoRollChartPanel

保持委托模式，`renderFullData()` 委托给 `PianoRollView`：

```cpp
double PianoRollChartPanel::dataDurationSec() const override {
    return m_pianoRoll->totalDurationSec();
}

void PianoRollChartPanel::renderFullData(QImage& image) override {
    QPainter p(&image);
    ChartCoordinate coord;
    m_pianoRoll->render(p, coord);
    p.end();
}
```

### B.4 架构变更对比

| 维度               | 当前架构                            | 新架构                        |
|------------------|---------------------------------|----------------------------|
| `drawContent()`  | 纯虚，子类各自实现                       | 基类 final 实现（裁剪+blit）       |
| `rebuildCache()` | 虚函数，可选覆盖                        | 移除，由 `renderFullData()` 替代 |
| `RegionUpdate`   | 用于增量渲染                          | 移除，不再需要                    |
| 视口变化             | 触发 `m_cacheDirty = true` → 重建缓存 | 仅改变 `srcRect`，不触发重建        |
| 子类职责             | 管理缓存 + 处理视口 + 绘制                | 仅提供完整数据图像                  |
| 基类职责             | 事件分发 + 空壳虚方法                    | 缓存管理 + 视口裁剪 + 变换 + 绘制      |
| 代码量（子类）          | 每子类 ~150-300 行                  | 每子类 ~50-100 行              |
| 行为一致性            | 各子类独立实现，不一致                     | 基类统一控制，保证一致                |

### B.5 实施阶段

#### Phase F-01：基类接口扩展（最低风险）

在 `ChartPanelBase` 中新增纯虚方法 `renderFullData()` 和 `dataDurationSec()`，提供默认实现（返回空/0），保持现有代码可编译。新增
`m_fullDataImage` 和 `m_fullDataDirty` 成员。

#### Phase F-02：WaveformChartPanel 先行迁移（验证可行性）

第一个迁移的图表，验证完整流程。实现 `renderFullData()`、`dataDurationSec()`、`fullDataImageWidth()`。保留旧的
`drawContent()` 和 `rebuildCache()` 但标记 `[[deprecated]]`，确保行为一致后再删除。

#### Phase F-03：PowerChartPanel + MouthCurveChartPanel 迁移

按相同模式迁移。MouthCurve 的 bug 在此阶段自动修复（不再需要索引计算公式）。

#### Phase F-04：SpectrogramChartPanel 迁移

需要处理 FFT 懒计算与 `renderFullData()` 的兼容。`renderFullData()` 内部调用 `ensureSpectrogramRange()` 进行按需计算。

#### Phase F-05：基类 `paintEvent()` 接管绘制

将 Phase F-02~F-04 验证过的 `paintEvent()` 逻辑迁移到基类。`drawContent()` 从虚函数变为基类 final 实现。所有子类删除
`drawContent()` 覆盖。

#### Phase F-06：清理废弃代码

删除 `RegionUpdate` 结构体、`rebuildCache()` 虚方法、`m_pendingRegion` 成员、`shiftCache()` 模板、各子类的旧缓存成员和绘制方法。

### B.6 验收标准

1. 口型曲线随 Ctrl+滚轮缩放同步更新，不再消失
2. 波形图、功率图、频谱图行为与重构前完全一致
3. Shift+滚轮纵向缩放所有图表表现一致
4. 拖动 MiniMap 平移视口，所有图表同步跟随
5. `cmake --build --preset release` 编译通过
6. 无新增 clang-tidy 警告
7. 手动冒烟测试：打开 ds-labeler，加载音频，确认所有图表正常工作

### B.7 边界条件与风险

| 风险                                 | 影响   | 缓解措施                                              |
|------------------------------------|------|---------------------------------------------------|
| 频谱图大文件（>1h）缓存图像过大                  | 内存占用 | `fullDataImageWidth()` 上限 32767；未来可增加多分辨率金字塔      |
| 钢琴卷帘图委托模式不兼容                       | 功能缺失 | Phase F 不改造 PianoRollView 内部，仅适配外部接口              |
| `renderFullData()` 在 widget 未显示时调用 | 无效渲染 | `ensureFullDataCache()` 检查 `imgW > 0 && imgH > 0` |
| 垂直缩放与图像裁剪的交互                       | 视觉偏移 | 垂直缩放以中心为锚点，保证对称性                                  |