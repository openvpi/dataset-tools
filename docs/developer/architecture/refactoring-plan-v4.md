# 大型重构方案 v4.2

> 版本：4.2.0 (深度修正版)
> 日期：2026-06-04
> 状态：方案设计阶段
> 取代：v4.1（修正版，含范围错误和遗漏模块，已作废）

---

## 0. 前置声明

### 0.1 方案目标

1. **功能不变**：所有现有功能行为完全一致，用户无感知
2. **数据安全**：工程数据稳定性、安全性不降低，所有 I/O 操作保持原子性
3. **设计一致**：严格遵循 [human-decisions.md](../../human-decisions.md) 全部准则（ARCH-01~11, CONCUR-01~03, ROBUST-01~03,
   INFRA-01~17, VIEW-01~18）
4. **零技术债**：抛开所有历史包袱，清理所有已验证的架构问题
5. **统一底层**：跨平台路径、编码、文件选择由固定模块负责，消除各应用行为不一致
6. **流水线适配**：重构后流程与标注流水线文档一致

### 0.2 方案约束

- 不引入新的外部依赖
- 不修改稳定核心代码来加新功能（遵循 ARCH-07）
- 保持 C++20/Qt6 技术栈
- 保持 4-space indent、120-col width、`#pragma once` 格式规范
- 不修改 `.dsproj` / `.dstext` 文件格式（向后兼容）
- 各阶段独立可验证，可独立回滚

### 0.3 核心文档引用

| 文档                                                       | 用途                                                                            |
|----------------------------------------------------------|-------------------------------------------------------------------------------|
| [human-decisions.md](../../human-decisions.md)           | 设计准则权威来源（速查表 ARCH-01~11, CONCUR-01~03, ROBUST-01~03, INFRA-01~17, VIEW-01~18） |
| [overview.md](overview.md)                               | 项目架构总览（框架分离原则、分层架构、命名规范、CMake 目标）                                             |
| [data-flow-design.md](data-flow/data-flow-design.md)     | 流水线定义、层依赖 DAG、脏数据传播                                                           |
| [component-design.md](data-flow/component-design.md)     | 核心组件设计说明                                                                      |
| [ds-format.md](data-flow/ds-format.md)                   | .dsproj/.dstext 格式规范                                                          |
| [unified-app-design.md](framework/unified-app-design.md) | LabelSuite/DsLabeler 统一应用设计                                                   |
| [conventions.md](../../guides/conventions.md)            | 编码规范                                                                          |
| [test-design.md](../testing/test-design.md)              | 测试架构                                                                          |
| [magic-numbers-audit.md](../magic-numbers-audit.md)      | 魔法数字审计                                                                        |

---

## 0A. v4.1 → v4.2 修正摘要

**方案本身的问题**（本节即"错误修正"）：

| # | v4.1 问题                                     | v4.2 修正                                                         |
|---|---------------------------------------------|-----------------------------------------------------------------|
| 1 | R1 声称 `EncodingUtils` "被多文件引用"需"全局搜索替换"     | 经 Grep 验证：**仅 PathUtils.cpp 内部调用**，无外部调用方。删除"全局搜索替换"描述          |
| 2 | R1 引用不存在的函数名 `openIfstream`/`openOfstream`  | 实际为 `openIfstream`/`openOfstream`，且无直接调用方（仅在 audio-util 层有弃用包装） |
| 3 | R5（删除 EncodingUtils::kInterfaceVersion）是死阶段 | 合并到 R1，R1 删除 EncodingUtils 后自动消除                                |
| 4 | R1 遗漏 audio-util/PathCompat.h 弃用层清理         | R1 新增步骤：迁移 SlicerService/Util/FlacDecoder 3 调用方到 PathUtils      |
| 5 | R2 列出不存在的 `FileDataSource`                  | 删除该引用                                                           |

**遗漏的代码问题**（需新增阶段）：

| #  | 发现                                                                                                                | ARCH 准则          | 处理                               |
|----|-------------------------------------------------------------------------------------------------------------------|------------------|----------------------------------|
| 6  | BuildDsPage / BuildCsvPage / GameAlignPage 三页面 ~70% 结构重复（RunProgressRow + QTextEdit + QAction + batch 模式）         | ARCH-01, ARCH-04 | 新增 R9                            |
| 7  | GameAlignPage 用 QLineEdit 替代 PathSelector，虽已 include PathSelector.h                                               | ARCH-13          | 新增 R10                           |
| 8  | audio-util/PathCompat.h 双重弃用层（`[[deprecated]]` 包装 → dsfw::PathCompat → PathUtils），3 处字符串转化写了 3 遍                  | INFRA-01         | 新增 R11                           |
| 9  | MoveBoundaryCommand(IBoundaryModel+TimePos) vs MoveSlicePointCommand(vector\<double\>+std::function) — 相同概念不同数据模型 | —                | 记录为文档债，暂不处理（两域有不同精度需求）           |
| 10 | ExportPage 未继承 EditorPageBase，自行实现 IPageActions/IPageLifecycle                                                    | —                | 记录为观察项，当前两层体系（数据驱动 vs 音频可视化）有合理性 |

---

## 1. 现状审计

### 1.1 代码库规模与模块划分

```
~42 CMake targets (excluding tests)

Framework (6):  dsfw-base (static) → dsfw-core (static) → dsfw-ui-core (static)
                → dsfw-widgets (shared) + dstools-audio (static)
                infer-common 源文件编译入 dsfw-core

Domain (1):     dstools-domain (static)

Libs (8):       slicer-lib, lyricfa-lib, hubertfa-lib, gameinfer-lib, rmvpepitch-lib,
                minlabel-lib, moelib, infer-bridge

App-Shared (9): data-sources, audio-visualizer, phoneme-editor, pitch-editor,
                min-label-editor, settings, log-page, model-init, mouth-curve-chart

Apps (5):       LabelSuite, DsLabeler, dstools-cli, WidgetGallery, TestShell

Infer (6):      audio-util, FunAsr, game-infer, hubert-infer, rmvpe-infer, moe-infer

Tests:          ~19 单元测试 + 1 集成测试
```

### 1.2 框架分层与命名空间（现状）

项目采用公认的**双命名空间分层架构**，详见 [overview.md](overview.md) Section 3 和 Section 10：

| 层        | CMake Target         | 主要命名空间                                                 | 职责                                         |
|----------|----------------------|--------------------------------------------------------|--------------------------------------------|
| 底层工具     | dsfw-base            | `dstools`                                              | JsonHelper                                 |
| 框架核心     | dsfw-core            | `dsfw` (工具类) + `dstools` (服务类)                         | 路径、编码、日志、AppSettings、Pipeline、Task         |
| 信号处理     | dsfw-core            | `dsfw::signal`, `dsfw::music`                          | 曲线工具、时序数据、音乐数学                             |
| UI 核心    | dsfw-ui-core         | `dsfw`, `dsfw::widgets`, `dstools`, `dstools::labeler` | AppShell、Theme、IPageActions、IPageLifecycle |
| Widget 库 | dsfw-widgets         | `dsfw::widgets` (主), `dstools` (少量)                    | PathSelector、LogViewer、TimeRulerWidget 等   |
| 音频       | dstools-audio        | `dstools::audio`                                       | AudioPlayer、AudioDecoder                   |
| 推理       | dstools-infer-common | `dstools::infer`                                       | IInferenceEngine、OnnxEnv、OnnxModelBase     |
| 领域       | dstools-domain       | `dstools`                                              | DsDocument、DsProject、ModelManager 等        |
| 应用       | dstools-apps         | `dstools`                                              | 所有页面、编辑器组件                                 |

**关键设计原则**：框架 `dsfw` 包含通用可复用代码（不含歌声合成领域逻辑），`dstools` 包含 DiffSinger 特定的领域逻辑。此分层是*
*有意设计**，不是混乱。

### 1.3 已识别的技术债与架构债（经代码验证）

#### 债-1：include 前缀与命名空间不完全对应

**已验证**：`<dsfw/AppSettings.h>` 声明 `namespace dstools`，`<dsfw/PathUtils.h>` 声明 `namespace dsfw`。同一 CMake
target (`dsfw::core`) 的 include 前缀统一为 `dsfw/`，但内部命名空间按职责分为 `dsfw`（工具类）和 `dstools`
（服务类）。这是有意的分层，但确实增加了新手学习成本。

**影响**：新开发者难以根据 include 路径推断命名空间。

**涉及文件**：`<dsfw/...>` 下约 20 个文件使用 `namespace dstools`，约 18 个使用 `namespace dsfw`。

#### 债-2：路径与编码模块分散但功能重叠

**已验证**：存在 4 个相关文件：

| 文件                     | 命名空间   | 核心功能                                          |
|------------------------|--------|-----------------------------------------------|
| `PathUtils.h/.cpp`     | `dsfw` | 路径操作、文件 I/O、CRC32、编码检测(`TextEncoding`)        |
| `EncodingUtils.h/.cpp` | `dsfw` | 文本编码检测/解码/编码（`Encoding` 枚举）                   |
| `PathCompat.h`         | `dsfw` | `std::ifstream`/`std::ofstream` 宽字符路径打开（18 行） |
| `FileDialogHelper.h`   | `dsfw` | QFileDialog 封装（QObject，非 Widget）              |

**关键发现**：

- `PathUtils::TextEncoding` 和 `EncodingUtils::Encoding` 枚举值**完全一一对应**（Utf8, Utf8Bom, Utf16LE, Utf16BE, Gbk,
  Latin1）
- `EncodingUtils` 还错误地包含 `kInterfaceVersion`（这是具体工具类，不应有接口版本号）
- `PathCompat.h` 仅 18 行，应合并到 `PathUtils` 或直接内联
- `TestPathUtils` 和 `TestEncoding` 两个测试文件**已存在**，分别测试各自功能

**影响**：两个枚举并存造成调用方困惑，同一类编码问题需要在两个类中查方法。

#### 债-3：PipelineContext 公开方法暴露 nlohmann::json

**已验证**：[PipelineContext.h](../../src/framework/core/include/dsfw/PipelineContext.h) L81：

```cpp
[[nodiscard]] static Result<void> validate(const nlohmann::json &j);
```

文件注释称 "nlohmann::json is isolated"，但此公开方法直接暴露该类型。`toJsonString()`/`fromJsonString()` 确实使用
`std::string` 隔离，但 `validate()` 破坏了隔离。

**影响**：调用方需要 `#include <nlohmann/json.hpp>`，违反 INFRA-13（PIMPL 隔离），增加增量编译时间。

#### 债-4：PathSelector 已存在但采用不统一

**已验证**：

| 组件                                | 类型                                     | 状态                                                |
|-----------------------------------|----------------------------------------|---------------------------------------------------|
| `dsfw::widgets::PathSelector`     | QWidget（行编辑 + 浏览按钮 + 拖放 + SettingsKey） | **已存在**，用于 BuildDsPage、BuildCsvPage、WidgetGallery |
| `dsfw::widgets::FilePathSelector` | QObject（Config 驱动 + SettingsKey）       | **已存在**，未被 apps 使用                                |
| `dsfw::FileDialogHelper`          | QObject 静态方法封装                         | **被广泛使用**：约 15 处 apps 代码直接调用                      |

**问题**：PathSelector 提供了一致的表单级路径选择体验（行编辑 + 浏览按钮 + 拖放），但绝大多数页面仍然直接使用
`FileDialogHelper::getXxx()` 静态方法，导致：

- 文件选择行为不一致（默认目录记忆、验证方式）
- 无法提供统一的路径验证（无效路径红色边框提示）
- 无法支持拖放选择

**涉及文件**（使用 FileDialogHelper 直接调用的页面）：
`DictionaryPanel`, `ModelPathPanel`, `SlicerPage`, `MinLabelPage`, `LogPage`, `SliceExportDialog`, `BatchProcessDialog`,
`ExportPage`, `WelcomePage`, `NewProjectDialog`, `GameAlignPage`(部分), `label-suite/main.cpp`

#### 债-5：文档引用不存在的接口

**已验证**：[overview.md](overview.md) Section 11 列出 11 个接口，经 Grep 验证：

| 文档中的接口           | 代码中是否存在 | 实际类名                           | 备注                        |
|------------------|---------|--------------------------------|---------------------------|
| IDocument        | 存在      | `dstools::IDocument`           | `dsfw/IDocument.h`        |
| IModelProvider   | 存在      | `dstools::IModelProvider`      | `dsfw/IModelProvider.h`   |
| IModelDownloader | **不存在** | —                              | 文档虚构                      |
| IModelManager    | **不存在** | `dstools::ModelManager` 是具体类   | 文档中写成接口                   |
| IG2PProvider     | 存在      | `dstools::IG2PProvider`        | `dsfw/IG2PProvider.h`     |
| IExportFormat    | 存在      | `dstools::IExportFormat`       | `dsfw/IExportFormat.h`    |
| IQualityMetrics  | **不存在** | `dsfw::QualityTypes` 仅类型定义     | 无接口定义                     |
| ISliceDataSource | 存在      | `dsfw::ISliceDataSource`       | `dsfw/ISliceDataSource.h` |
| IAudioPlayer     | 存在      | `dstools::audio::IAudioPlayer` | `dstools/IAudioPlayer.h`  |
| IStepPlugin      | **不存在** | —                              | 文档虚构                      |
| IInferenceEngine | 存在      | `dstools::IInferenceEngine`    | infer 层                   |

**文档中还列出不存在的默认实现**：`StubModelDownloader`、`StubG2PProvider`、`StubExportFormat`、`StubQualityMetrics`、
`StubStepPlugin` — 均未在源码中找到。实际实现为 `PinyinG2PProvider`、`HtsLabelExportFormat`、`SinsyXmlExportFormat`。

**影响**：文档可信度降低，新开发者被误导。

#### 债-6：Overview.md 仍使用旧编号体系

**已验证**：[overview.md](overview.md) Section 1 使用 P-01~P-19
编号引用设计准则，但 [human-decisions.md](../../human-decisions.md) 已升级为 ARCH/CONCUR/ROBUST/INFRA/VIEW 体系。

**影响**：两个文档编号不统一，交叉引用困难。

#### 债-7：Overview.md 目录树重复条目

**已验证**：

| 行号                    | 问题                                                                                                      |
|-----------------------|---------------------------------------------------------------------------------------------------------|
| L503                  | `src/widgets/` 标注为 `dstools-widgets (INTERFACE, header-only)` — 实际路径是 `src/framework/widgets/ (SHARED)` |
| L485-L501 和 L513-L537 | `src/framework/` 出现两次，第 2 次嵌套了 `infer/`（实际 `src/infer/` 是独立顶层目录）                                        |
| L525-526              | `src/apps/shared/audio-visualizer/` — 实际路径为 `src/apps/shared/audio-visualizer/`，需要确认                    |

#### 债-8：文档引用已删除的测试

**已验证**：[test-design.md](../testing/test-design.md) L110 和 [build.md](../../guides/build.md) L208 引用
`TestLocalFileIOProvider`，但 `IFileIOProvider` 已删除，测试已从 `CMakeLists.txt` 移除。

#### 债-9：EncodingUtils 错误包含 kInterfaceVersion

**已验证**：[EncodingUtils.h](../../src/framework/core/include/dsfw/EncodingUtils.h) L17：
`static constexpr int kInterfaceVersion = 1;`

`EncodingUtils` 是**静态工具类**（所有方法都是 static），不是抽象接口。`kInterfaceVersion` 约定仅用于抽象接口（`I` 前缀的纯虚类）。

**影响**：违反 INFRA-19（接口版本化仅用于接口）。

#### 债-10：魔法数字散布

**已验证**：详见 [magic-numbers-audit.md](../magic-numbers-audit.md)，覆盖频谱图、功率图、波形图、钢琴卷帘编辑器、音高提取等模块。

---

## 2. 重构准则补充

基于项目现状和 C++/Qt 最佳实践，在现有 `human-decisions.md` 准则基础上补充：

### 2.1 补充架构准则

#### ARCH-12：路径/编码统一模块原则

**原则**：跨平台路径处理、编码转换、文件 I/O 由**一个模块的单个类**统一负责。消除重复枚举、重复功能。

**适用**：`PathUtils` 和 `EncodingUtils` 合并为统一的 `dsfw::PathUtils`，吸收 `PathCompat` 的宽字符文件打开功能。

#### ARCH-13：统一控件使用原则

**原则**：项目已有的统一控件（如 `PathSelector`、`FilePathSelector`）必须优先使用。新增页面不得直接调用 `QFileDialog` 或
`FileDialogHelper` 静态方法。

**理由**：消除各应用文件选择行为不一致。PathSelector 提供内置的路径验证、拖放、SettingsKey 历史记录。

#### ARCH-14：配置驱动原则

**原则**：所有可调节的数值参数（阈值、尺寸、颜色等）必须通过 `AppSettings` + `SettingsKey<T>`
配置化，禁止硬编码魔法数字。例外：数学常量、协议常量（如默认采样率）。

### 2.2 补充基础准则

#### INFRA-18：文档即代码原则

**原则**：设计文档中引用的类名、方法名、文件路径必须与代码实际一致。每次重构后相关文档同步更新。

#### INFRA-19：接口版本化仅在接口中使用

**原则**：`kInterfaceVersion` 仅用于 `I` 前缀的纯虚接口类。具体工具类（如 `EncodingUtils`）不得有此常量。

#### INFRA-20：测试文件与代码同步

**原则**：删除被测代码时，必须同步删除对应测试文件和 CMake 注册。文档中对已删除测试的引用必须清理。

### 2.3 补充并发准则

#### CONCUR-04：文件 I/O 异步化

**原则**：所有超过 50ms 的文件读写操作必须在后台线程执行，通过信号通知完成。`PathUtils::readFile`/`writeFile`
保持同步用于小文件（<10KB），大文件操作通过 `AsyncTask` 包装。

### 2.4 补充视图准则

#### VIEW-19：路径选择控件统一

**原则**：所有页面中的文件/目录选择必须使用 `PathSelector` 或 `FilePathSelector` 控件。`FileDialogHelper`
降为内部实现细节，不直接暴露给页面代码。

---

## 3. 分阶段重构方案

### 阶段总览

| 阶段  | 名称                                                        | 风险 | 涉及范围                           | 回滚难度 |
|-----|-----------------------------------------------------------|----|--------------------------------|------|
| R1  | 底层统一：PathUtils 合并 EncodingUtils + PathCompat              | 中  | framework/core + 全项目引用         | 低    |
| R2  | 文档清理：修正过时引用和不存在的接口                                        | 低  | docs/                          | 极低   |
| R3  | 控件统一：推广 PathSelector/FilePathSelector 替换 FileDialogHelper | 中  | framework/widgets + apps       | 中    |
| R4  | PIMPL 完善：PipelineContext::validate() 隔离 JSON              | 中  | framework/core                 | 中    |
| R5  | EncodingUtils 清理：移除 kInterfaceVersion（已合并到 R1）            | —  | —                              | —    |
| R6  | 魔法数字配置化                                                   | 中  | apps/shared                    | 中    |
| R7  | 文档结构修正：overview.md 目录树和编号                                 | 低  | docs/                          | 极低   |
| R8  | 测试文档同步：移除 TestLocalFileIOProvider 引用                      | 低  | docs/                          | 极低   |
| R9  | label-suite 页面通用基类提取（WizardPageBase）                      | 中  | apps/label-suite               | 中    |
| R10 | GameAlignPage QLineEdit → PathSelector                    | 低  | apps/label-suite               | 极低   |
| R11 | audio-util/PathCompat.h 弃用包装层清理                           | 低  | infer/audio-util + libs/slicer | 低    |

---

### R1：底层统一 — PathUtils 合并 EncodingUtils + PathCompat + 清理 audio-util 弃用包装

**目标**：消除 `PathUtils::TextEncoding` 和 `EncodingUtils::Encoding` 两个等价枚举，统一路径/编码功能入口，清理所有弃用包装层。

**前置验证**：经 Grep 验证，`EncodingUtils` **仅 `PathUtils.cpp` 内部调用**（无外部调用方），PathUtils 已完全包装
EncodingUtils 的 detect/toUtf8/fromUtf8。重构只需内联实现，无需全局搜索替换外部调用方。

**范围**：

- [PathUtils.h](../../src/framework/core/include/dsfw/PathUtils.h) — 合并后扩展
- [PathUtils.cpp](../../src/framework/core/src/PathUtils.cpp) — 内联 EncodingUtils 实现
- [EncodingUtils.h](../../src/framework/core/include/dsfw/EncodingUtils.h) — **删除**
- [EncodingUtils.cpp](../../src/framework/core/src/EncodingUtils.cpp) — **删除**
- [PathCompat.h](../../src/framework/core/include/dsfw/PathCompat.h) — **删除**（`openIfstream`/`openOfstream` 无直接调用方）
- [audio-util/PathCompat.h](../../src/infer/audio-util/include/audio-util/PathCompat.h) — 迁移弃用函数调用方后删除
- [TestPathUtils.cpp](../../src/tests/framework/TestPathUtils.cpp) — 合并 TestEncoding 用例
- [TestEncoding.cpp](../../src/tests/framework/TestEncoding.cpp) — **删除**
- CMakeLists.txt 测试注册 — 移除 TestEncoding

**具体变更**：

1. 将 `EncodingUtils` 的全部实现内联到 `PathUtils`：
    - `EncodingUtils::detect()` → 已在 `PathUtils::detectTextEncoding()` 中委托 → 直接内联实现
    - `EncodingUtils::toUtf8()` → 已在 `PathUtils::decodeText()` 中委托 → 直接内联
    - `EncodingUtils::fromUtf8()` → 已在 `PathUtils::encodeText()` 中委托 → 直接内联
    - `EncodingUtils::isGbkFirstByte()` / `isGbkSecondByte()` / `tryGbkHeuristic()` → 移入 `PathUtils` 私有区

2. 删除 `EncodingUtils::Encoding` 枚举，统一使用 `PathUtils::TextEncoding`（删除 `EncodingUtils.cpp` 中的 static_cast）。

3. 删除 `dsfw/PathCompat.h`（`openIfstream`/`openOfstream` 无任何直接调用方，仅在 audio-util 层有弃用包装）。

4. 迁移 audio-util 弃用函数调用方到 PathUtils：
    - [SlicerService.cpp](../../src/libs/slicer/SlicerService.cpp) — `pathToUtf8()` → `dsfw::PathUtils::toUtf8()`
    - [Util.cpp](../../src/infer/audio-util/src/Util.cpp) — 同上
    - [FlacDecoder.cpp](../../src/infer/audio-util/src/FlacDecoder.cpp) — 同上
    - 删除 [audio-util/PathCompat.h](../../src/infer/audio-util/include/audio-util/PathCompat.h)

5. 合并测试：将 `TestEncoding` 的测试用例移入 `TestPathUtils`。

6. 文件删除清单：
    - `EncodingUtils.h` / `EncodingUtils.cpp` / `PathCompat.h` / `TestEncoding.cpp`
    - `audio-util/PathCompat.h`

**验证**：

- 全量编译通过（零 error）
- `TestPathUtils` 全部用例通过
- Grep 确认无残留 `EncodingUtils::` / `AudioUtil::pathToUtf8` 引用

**风险**：中等。SlicerService/Util/FlacDecoder 的编码行为依赖正确迁移。

---

### R2：文档清理 — 修正过时引用和不存在的接口

**目标**：修正 overview.md 中不存在的接口和默认实现列表。

**范围**：[overview.md](overview.md) Section 1、Section 11、Section 15

**具体变更**：

1. Section 1（L9-L30）：将 P-01~P-19 旧编号更新为 ARCH/CONCUR/ROBUST/INFRA/VIEW 新编号，添加新旧编号映射说明。

2. Section 11（L444-L458）表格修正为已验证状态：

   | 接口 | 层级 | 实际实现 |
      |------|------|---------|
   | IDocument | core | DsDocumentAdapter (domain) |
   | IModelProvider | core | FunAsrModelProvider, GameModelProvider, HuBERTModelProvider (libs) |
   | IG2PProvider | core | PinyinG2PProvider (domain) |
   | IExportFormat | core | HtsLabelExportFormat, SinsyXmlExportFormat (domain) |
   | ISliceDataSource | core | ProjectDataSource (ds-labeler) |
   | IAudioPlayer | audio | AudioPlayer (audio) |
   | IInferenceEngine | infer | GameEngine, HuBERTEngine, RmvpeEngine, MoeEngine, FunAsrEngine |

   删除：IModelDownloader, IModelManager（改为在备注中说明 ModelManager 是具体类）, IQualityMetrics（改为在备注中说明仅
   QualityTypes 存在）, IStepPlugin。

3. Section 15（L574-L576）：已更新为引用 refactoring-plan-v4.md（上次已完成）。

**验证**：

- 文档中所有类名经 Grep 验证存在于源码
- 新旧编号映射与 human-decisions.md 一致

**风险**：极低。仅文档修改。

---

### R3：控件统一 — 推广 PathSelector/FilePathSelector

**目标**：将散落在各页面的 `FileDialogHelper::getXxx()` 直接调用替换为 `PathSelector` 或 `FilePathSelector`。

**范围**：

- 约 15 处 apps 代码（见债-4 的涉及文件列表）
- `PathSelector` — 用于需要行编辑 + 浏览按钮的表单场景
- `FilePathSelector` — 用于以代码方式触发文件选择的场景

**具体变更**（按页面逐项替换）：

| 页面                     | 当前做法                                                                                                          | 替换方案                      |
|------------------------|---------------------------------------------------------------------------------------------------------------|---------------------------|
| `ModelPathPanel` (2 处) | `FileDialogHelper::getExistingDirectory()` / `getOpenFileName()`                                              | `PathSelector`（表单场景）      |
| `SlicerPage` (4 处)     | `FileDialogHelper::getOpenFileName()` / `getSaveFileName()` / `getOpenFileNames()` / `getExistingDirectory()` | `FilePathSelector`（对话框场景） |
| `MinLabelPage`         | `FileDialogHelper::getExistingDirectory()`                                                                    | `FilePathSelector`        |
| `LogPage`              | `FileDialogHelper::getSaveFileName()`                                                                         | `FilePathSelector`        |
| `SliceExportDialog`    | `FileDialogHelper::getExistingDirectory()`                                                                    | `FilePathSelector`        |
| `BatchProcessDialog`   | `FileDialogHelper::getSaveFileName()`                                                                         | `FilePathSelector`        |
| `ExportPage`           | `FileDialogHelper::getExistingDirectory()`                                                                    | `FilePathSelector`        |
| `WelcomePage`          | `FileDialogHelper::getOpenFileName()`                                                                         | `FilePathSelector`        |
| `NewProjectDialog`     | `FileDialogHelper::getExistingDirectory()`                                                                    | `FilePathSelector`        |
| `DictionaryPanel`      | `FileDialogHelper::getOpenFileName()`                                                                         | `PathSelector`            |
| `label-suite/main.cpp` | `FileDialogHelper::getExistingDirectory()`                                                                    | `FilePathSelector`        |
| `GameAlignPage`        | `FileDialogHelper::getOpenFileNames()`                                                                        | `FilePathSelector`        |

**注意事项**：

- 保留 `FileDialogHelper` 作为内部实现（PathSelector 和 FilePathSelector 内部可依赖它）
- 不删除 `FileDialogHelper` 公共 API（保持向后兼容），但标记为 `[[deprecated]]`
- 表单场景优先用 `PathSelector`（提供行编辑 + 验证 + 拖放），对话框场景用 `FilePathSelector`

**验证**：

- 全量编译通过
- 各页面文件选择交互正常
- 默认目录记忆（SettingsKey）功能正常

**风险**：中等。逐页面替换，每页一个 commit，方便回滚。

---

### R4：PIMPL 完善 — PipelineContext::validate() 隔离 JSON

**目标**：将 `PipelineContext::validate(const nlohmann::json &j)` 替换为基于字符串的版本。

**范围**：

- [PipelineContext.h](../../src/framework/core/include/dsfw/PipelineContext.h) L80-L81
- [PipelineContext.cpp](../../src/framework/core/src/PipelineContext.cpp)

**具体变更**：

1. 替换公开方法签名：
   ```cpp
   // 旧（暴露 nlohmann::json）
   [[nodiscard]] static Result<void> validate(const nlohmann::json &j);

   // 新（使用 std::string）
   [[nodiscard]] static Result<void> validateFromString(const std::string &jsonStr);
   ```

2. 内部实现：`validateFromString()` 解析 JSON 后调用私有 `validate(const nlohmann::json &j)` 方法。

3. 将 `validate(const nlohmann::json &j)` 移到 private 区域。

4. 更新所有调用方。

**验证**：

- 全量编译通过
- 确认没有任何公开头文件暴露 `nlohmann::json` 类型（不含 `#include <nlohmann/json.hpp>` 也可编译使用 PipelineContext.h）
- PipelineContext 相关测试通过

**风险**：中等。需要全局搜索 `PipelineContext::validate(` 替换为 `PipelineContext::validateFromString(`。

---

### R5：EncodingUtils 清理 — 移除 kInterfaceVersion（已合并到 R1）

**状态**：v4.2 已合并到 R1。R1 删除 EncodingUtils 后，`kInterfaceVersion` 自动消除。此阶段仅保留作为代号标记，不单独执行。

**注**：如果 R1 后 EncodingUtils 仍有残留，再作为独立步骤执行。

---

### R6：魔法数字配置化

**目标**：消除 [magic-numbers-audit.md](../magic-numbers-audit.md) 中的硬编码魔法数字。

**范围**：

- `src/apps/shared/settings/Keys.h` — 新增 SettingsKey 定义
- 频谱图、功率图、波形图、钢琴卷帘编辑器 — 替换硬编码

**具体变更**：

1. 在 `Keys.h` 中新增 `namespace dstools::settings` 下 SettingsKey 声明。

2. 各渲染组件在构造函数中读取 `AppSettings`，替换硬编码常量。

3. 不新增设置界面（设置界面延后到后续需求）。

**验证**：

- 参数通过 AppSettings 可读写，默认值等于原硬编码值
- 渲染效果不变

**风险**：中等。需要确保 AppSettings 在渲染组件构造时可用。

---

### R7：文档结构修正 — overview.md 目录树和编号

**目标**：修正 overview.md 的目录树错误和旧编号。

**注**：如果 R2 已执行 Section 1 编号更新，则此阶段仅处理目录树。

**范围**：[overview.md](overview.md) Section 12

**具体变更**：

1. 修正 `src/widgets/` → `src/framework/widgets/`（L503）
2. 删除重复的 `src/framework/` 条目（L513-L521），将 infer/ 移到独立的 `src/infer/` 段
3. 确认 `apps/shared/` 下子目录名称与文件系统一致

**验证**：

- 文档中所有目录路径经 LS 命令验证存在

**风险**：极低。

---

### R8：测试文档同步 — 移除 TestLocalFileIOProvider 引用

**目标**：清理 test-design.md 和 build.md 中对已删除测试的引用。

**范围**：

- [test-design.md](../testing/test-design.md) L69, L110
- [build.md](../../guides/build.md) L208

**具体变更**：

1. test-design.md：删除 `TestLocalFileIOProvider.cpp` 条目和注释行
2. build.md：从测试列表中移除 `TestLocalFileIOProvider`
3. test-design.md：更新测试数量（19 单元测试 + 1 集成测试）

**验证**：

- 文档中所有测试名经 Grep 验证在 `src/tests/` 中存在

**风险**：极低。

---

### R9：label-suite 页面通用基类提取 — WizardPageBase

**目标**：消除 BuildDsPage / BuildCsvPage / GameAlignPage 三页面 ~70% 的重复代码，遵循 ARCH-01（单一职责）和 ARCH-04（合并 >
60% 重叠模块）。

**当前重合度分析**：

```cpp
// 三个页面的共同结构（伪代码）
class XxxPage : public QWidget, public IPageActions {
    Q_OBJECT
public:
    void setWorkingDirectory(const QString &dir);
    QString workingDirectory() const;
    QMenuBar *createMenuBar(QWidget *parent) override;  // 统一：只有 Run 动作

protected:
    QString m_workingDir;
    dsfw::widgets::PathSelector *m_xxxPath = nullptr;
    dsfw::widgets::RunProgressRow *m_runProgress = nullptr;
    QTextEdit *m_log = nullptr;
    QAction *m_runAction = nullptr;

private:
    void buildUi();     // 每个页面自己实现
    void start();       // 每个页面自己实现
    void cancel();      // 每个页面自己实现
};
```

**新增基类**：`WizardPageBase`（位置：`src/apps/label-suite/pages/WizardPageBase.h`）

```cpp
class WizardPageBase : public QWidget, public IPageActions {
    Q_OBJECT
public:
    explicit WizardPageBase(QWidget *parent = nullptr);

    void setWorkingDirectory(const QString &dir);
    QString workingDirectory() const;
    QMenuBar *createMenuBar(QWidget *parent) override;

    // 子类必须实现
    virtual void start() = 0;
    virtual void cancel() = 0;

protected:
    QString m_workingDir;
    dsfw::widgets::RunProgressRow *m_runProgress = nullptr;
    QTextEdit *m_log = nullptr;
    QAction *m_runAction = nullptr;

    void appendLog(const QString &msg);
    void setRunning(bool running);

private:
    void buildUi();
};
```

**各页面变更**：

| 页面            | 变更                                                                                                                                  |
|---------------|-------------------------------------------------------------------------------------------------------------------------------------|
| BuildDsPage   | 继承改为 `WizardPageBase`，删除 `m_runProgress`/`m_log`/`m_runAction`/`m_workingDir`/`buildUi()`/`setWorkingDirectory()`/`createMenuBar()` |
| BuildCsvPage  | 同上                                                                                                                                  |
| GameAlignPage | 同上，同时执行 R10（QLineEdit → PathSelector）                                                                                               |

**注意事项**：

- 三个页面的 `start()`/`cancel()` 各自保留（具体 pipeline 逻辑不同）
- `buildUi()` 的差异化部分（各页面的表单字段）通过各自构造函数调用基类后的 `addFormRow()` 实现

**验证**：

- 全量编译通过
- 三个页面运行效果不变
- 进度条/日志/取消按钮行为一致

**风险**：中等。需要确保三个页面的 `createMenuBar()` 行为完全一致，GameAlignPage 的 Signal 连接方式可能有差异。

---

### R10：GameAlignPage QLineEdit → PathSelector

**目标**：消除 ARCH-13 违规 — GameAlignPage 已 include `PathSelector.h` 但使用 `QLineEdit`。

**范围**：

- [GameAlignPage.h](../../src/apps/label-suite/pages/GameAlignPage.h) — 替换 `QLineEdit *m_modelPath`
- [GameAlignPage.cpp](../../src/apps/label-suite/pages/GameAlignPage.cpp) — 替换构造函数和 `start()` 中的读写

**具体变更**：

1. 头文件：`QLineEdit *m_modelPath` → `dsfw::widgets::PathSelector *m_modelPath`
2. cpp：`m_modelPath = new QLineEdit` → `m_modelPath = new dsfw::widgets::PathSelector`
3. `m_modelPath->text()` → `m_modelPath->currentPath()`（PathSelector 协议）
4. 删除 `#include <QLineEdit>`（PathSelector 已包含）

**验证**：编译通过，GameAlignPage 界面和行为不变（PathSelector 提供与 QLineEdit 等效的文本 + 浏览按钮）。

**风险**：极低。

---

### R11：audio-util/PathCompat.h 弃用包装层清理

**目标**：消除 `pathToUtf8` 的 **3 次重复实现**（`audio-util/PathCompat.h` 弃用包装 → `dsfw/PathCompat.h` →
`dsfw/PathUtils::toUtf8()` 三重链路），直接迁移调用方。

**注意**：此阶段的 `dsfw/PathCompat.h` 删除部分已由 R1 覆盖。如果 R1 已执行则仅处理 audio-util 侧。

**范围**：

- [SlicerService.cpp](../../src/libs/slicer/SlicerService.cpp) — 替换 `pathToUtf8()` 调用
- [Util.cpp](../../src/infer/audio-util/src/Util.cpp) — 同上
- [FlacDecoder.cpp](../../src/infer/audio-util/src/FlacDecoder.cpp) — 同上
- [audio-util/PathCompat.h](../../src/infer/audio-util/include/audio-util/PathCompat.h) — **删除**

**具体变更**：

1. SlicerService.cpp：`AudioUtil::pathToUtf8(p)` → `dsfw::PathUtils::toUtf8(p)`
2. Util.cpp：同上
3. FlacDecoder.cpp：同上，且 `AudioUtil::openSndfile()` 已直接调用 `dsfw::PathUtils::toWide()`/`toUtf8()`，无需额外修改
4. 删除整个 `audio-util/PathCompat.h`

**验证**：编译通过，Slicer/Util/FlacDecoder 行为不变。

**风险**：低。编码行为完全一致（`pathToUtf8` 内部就是 `PathUtils::toUtf8` 的弃用包装）。

---

## 4. 实施顺序与依赖关系

```
R1 (底层统一) ────── 前置依赖：无
    │
    ├── R2 (文档清理) ── 前置依赖：无，可与 R1 并行
    │
    ├── R3 (控件统一) ── 前置依赖：R1（PathUtils 稳定后）
    │
    ├── R4 (PIMPL 完善) ── 前置依赖：无，可与 R1 并行
    │
    ├── R5 (EncodingUtils kInterfaceVersion) ── v4.2 已合并到 R1，自动跳过
    │
    ├── R6 (魔法数字) ── 前置依赖：无，可与 R1 并行
    │
    ├── R7 (文档结构) ── 前置依赖：R2
    │
    ├── R8 (测试文档) ── 前置依赖：无，可与 R1 并行
    │
    ├── R9 (label-suite 页面基类) ── 前置依赖：R10（GameAlignPage 先迁移到 PathSelector）
    │       │
    │       └── R10 (GameAlignPage PathSelector) ── 前置依赖：R1, R3
    │
    └── R11 (audio-util PathCompat 清理) ── 前置依赖：R1（dsfw/PathCompat 已删除）
```

**推荐执行顺序**：R1 → R2 → {R3, R4, R6, R8, R11} 并行 → R10 → R9 → R7

---

## 5. 质量保证

### 5.1 每阶段验证清单

- [ ] `clang-format` 格式检查通过
- [ ] 相关设计文档已同步更新
- [ ] 新代码符合编码规范

### 5.2 功能回归测试

每个阶段完成后执行：

1. **DsLabeler**：新建工程 → 导入音频 → 自动切片 → MinLabel → PhonemeLabeler → PitchLabeler → 导出 CSV/DS
2. **LabelSuite**：打开 TextGrid → 编辑 → 保存；波形/频谱显示
3. **数据安全**：工程文件 (.dsproj) 读写；标注文件 (.dstext) 读写

### 5.3 回滚策略

- 每阶段独立 Git 分支 `refactor/R{1-8}-v4`
- 阶段完成并验证后合并到本地主分支
- 不推送远程

---

## 6. 附录

### A. 文档清理清单

| 文档             | 行号        | 问题                                     | 修正方案                                 | 对应阶段 |
|----------------|-----------|----------------------------------------|--------------------------------------|------|
| overview.md    | L11-L30   | P-01~P-19 旧编号                          | 替换为 ARCH/CONCUR/ROBUST/INFRA/VIEW 体系 | R2   |
| overview.md    | L450-L451 | IModelDownloader, IModelManager (接口形式) | 删除不存在的接口，ModelManager 转为备注           | R2   |
| overview.md    | L453-L454 | StubExportFormat, StubQualityMetrics   | 替换为实际实现                              | R2   |
| overview.md    | L454-L457 | IQualityMetrics, IStepPlugin           | 删除不存在的接口                             | R2   |
| overview.md    | L503      | `src/widgets/` 路径错误                    | 修正为 `src/framework/widgets/`         | R7   |
| overview.md    | L513-L537 | 重复 `src/framework/` 条目                 | 删除重复段，infer/ 独立                      | R7   |
| test-design.md | L69, L110 | TestLocalFileIOProvider                | 删除引用                                 | R8   |
| build.md       | L208      | TestLocalFileIOProvider                | 删除引用                                 | R8   |

### B. 已删除/废弃项记录

| 原名称                            | 删除原因             | 状态    |
|--------------------------------|------------------|-------|
| `IFileIOProvider`              | INFRA-02：仅 1 个实现 | 已删除   |
| `ISettingsBackend`             | INFRA-02：仅 1 个实现 | 已删除   |
| `ISlicerService`               | INFRA-02：仅 1 个实现 | 已删除   |
| `EncodingUtils`                | R1：合并到 PathUtils | 计划删除  |
| `PathCompat` (dsfw)            | R1：合并到 PathUtils | 计划删除  |
| `PathCompat` (audio-util)      | R11：清理弃用包装层      | 计划删除  |
| `TestLocalFileIOProvider` 文档引用 | 代码已删除            | 待清理文档 |

### C. 与 human-decisions.md 准则的对照

| 阶段  | 涉及的准则                                                          |
|-----|----------------------------------------------------------------|
| R1  | INFRA-01（唯一负责）、INFRA-06（路径工具统一）、INFRA-19（kInterfaceVersion 限制） |
| R2  | INFRA-18（文档即代码）                                                |
| R3  | ARCH-13（统一控件使用）、VIEW-19（选择控件统一）                                |
| R4  | INFRA-13（PIMPL 隔离）                                             |
| R5  | （已合并到 R1）                                                      |
| R6  | ARCH-14（配置驱动）                                                  |
| R7  | INFRA-18（文档即代码）                                                |
| R8  | INFRA-20（测试与代码同步）                                              |
| R9  | ARCH-01（单一职责）、ARCH-04（合并 >60% 重叠模块）                            |
| R10 | ARCH-13（统一控件使用）                                                |
| R11 | INFRA-01（唯一负责）                                                 |

### D. 补充准则速查

| 编号        | 领域 | 摘要                                          |
|-----------|----|---------------------------------------------|
| ARCH-12   | 架构 | 路径/编码统一模块：一个模块的单个类负责全部路径和编码功能               |
| ARCH-13   | 架构 | 统一控件使用：优先使用已有统一控件，禁止裸调 FileDialog           |
| ARCH-14   | 架构 | 配置驱动：可调参数通过 AppSettings+SettingsKey 配置化     |
| INFRA-18  | 基础 | 文档即代码：文档引用必须与代码实际一致                         |
| INFRA-19  | 基础 | 接口版本化仅接口：kInterfaceVersion 仅用于纯虚接口          |
| INFRA-20  | 基础 | 测试代码同步：删除被测代码时同步清理测试和文档                     |
| CONCUR-04 | 并发 | 文件 I/O 异步：>50ms 文件操作在后台线程执行                 |
| VIEW-19   | 视图 | 路径选择控件统一：全部使用 PathSelector/FilePathSelector |

### E. 记录但不处理的项目（后续版本跟踪）

以下两项经分析后决定暂不纳入本次重构，理由和后续建议如下：

| # | 发现                                                                                                                                                        | 不处理理由                                                                                          | 后续建议                                                                     |
|---|-----------------------------------------------------------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------|--------------------------------------------------------------------------|
| 1 | `MoveBoundaryCommand`(IBoundaryModel+TimePos) vs `MoveSlicePointCommand`(vector\<double\>+std::function) — 相同概念不同数据模型，SliceCommands 用回调刷新而非 QUndoStack 信号 | 两域有不同精度需求（µs vs s），统一需引入 IBoundaryModel 到切片域，影响面大                                              | 长期：SliceListPanel 适配 IBoundaryModel 后，SliceCommands 可合并到 chart-framework |
| 2 | ExportPage 未继承 EditorPageBase，自行实现 IPageActions/IPageLifecycle                                                                                            | 当前两层体系有合理性：EditorPageBase（数据驱动编辑器）vs EditorContainerBase（音频可视化）。ExportPage 是"输出汇总"页，不属于任何编辑器类型 | 观察：如果未来更多非编辑器页面出现，考虑提取 `SimplePage` 基类                                   |