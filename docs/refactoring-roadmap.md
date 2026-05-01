# 架构重构路线图

> 基于 2026-04-30 架构分析，目标：通用框架分离、模块解耦、前后端分离、推理层统一
>
> **范围**: 在本仓库内完成库分离，不新建独立仓库。所有模块作为 CMake 子目录共存。

---

## 目标总览

| # | 目标 | 总工作量 | 优先级 | 状态 |
|---|------|---------|--------|------|
| G1 | 通用框架分离改名（仓库内独立库） | L (3-4w) | P0 | ✅ 已完成 |
| G2 | 通用框架补充常用功能 | XL (4-6w) | P2 | ✅ 已完成 |
| G3 | 解耦模块 | M (1-2w) | P1 | ⏳ 部分完成 |
| G4 | 提供稳定可靠的标准控件 | L (3-4w) | P2 | ⏳ 部分完成 |
| G5 | Qt 和非 Qt 依赖分开 | M (1-2w) | P0 | ✅ 已完成 |
| G6 | 前后端分开 | L (2-3w) | P1 | ✅ 已完成 |
| G7 | DS 和非 DS 相关分开 | S (<1w) | P1 | ✅ 已完成 |
| G8 | 相似 infer 模块构建共用底层 | L (2-3w) | P1 | ✅ 已完成 |

---

## Phase 0 — 预备工作 ✅ 全部完成

| 任务 | 目标 | 成果 |
|------|------|------|
| T-0.1 泛化 ModelType 枚举 | G1+G7 | `ModelTypeId` 整数 ID + 字符串注册表替代硬编码枚举；`DocumentFormat` 同步泛化为 `DocumentFormatId` |
| T-0.2 dsfw-core 去 Qt::Gui | G5 | dsfw-core 仅链接 Qt::Core + Qt::Network |
| T-0.3 推理公共层去 Qt | G5 | dstools-infer-common 移除 Qt::Core 依赖，`errorMsg` 改为 `std::string *` |
| T-0.4 pipeline 相对路径修复 | G3 | slicer/lyricfa/hubertfa 独立为 STATIC 库，pipeline-pages 通过 target_link_libraries 链接 |
| T-0.5 DS 业务泄漏审计 | G7 | 发现 4 处泄漏（ModelType 枚举、DocumentFormat::DsFile、IG2PProvider、ModelManager 传递性），均已纳入后续任务修复 |

---

## Phase 1 — 核心分离 ✅ 全部完成

| 任务 | 目标 | 成果 |
|------|------|------|
| T-1.1 ModelManager 迁移至 domain | G1+G7 | `IModelManager` 接口定义于 dsfw-core，具体实现迁至 dstools-domain |
| T-1.2 dsfw-core 拆分 dsfw-base | G5 | dsfw-base 已创建于 src/framework/base/，包含 JsonHelper（Qt-free） |
| T-1.3 丰富 IInferenceEngine | G8 | 添加 `load()/unload()/estimatedMemoryBytes()` 方法 |
| T-1.4 创建 OnnxModelBase | G8 | OnnxModelBase + CancellableOnnxModel 已实现；GameModel/RmvpeModel 继承 CancellableOnnxModel，`loadSessionTo()` 支持多 session |
| T-1.5 定义后端服务接口 | G6 | ISlicerService、IAlignmentService、IAsrService、IPitchService、ITranscriptionService 已定义 |

---

## Phase 2 — 库边界固化 ✅ 全部完成

| 任务 | 目标 | 成果 |
|------|------|------|
| T-2.1 整理 dsfw 目录结构与导出 | G1 | src/audio/ → src/framework/audio/，src/infer/common/ → src/framework/infer/ |
| T-2.2 版本化 dsfw 公共 API | G1 | DSFW_VERSION 宏 + DSFW_EXPORT 符号导出 |
| T-2.3 标准化模型配置加载 | G8 | OnnxModelBase 添加 `loadConfig()` + `onConfigLoaded()` 虚方法 |
| T-2.4 IInferenceEngine↔IModelProvider 桥接 | G8 | `InferenceModelProvider<Engine>` 模板实现 |
| T-2.5 FunASR 适配器 | G3+G8 | FunAsrAdapter 实现 IInferenceEngine，不修改 vendor 源码 |
| T-2.6 提取 pipeline 后端逻辑 | G6 | SlicerService/AsrService/AlignmentService 已实现 |

---

## Phase 3 — 框架增强 ✅ 全部完成

| 任务 | 目标 | 成果 |
|------|------|------|
| T-3.1 结构化日志系统 | G2 | Logger 单例，6 级 severity，category 标签，可插拔 sink |
| T-3.2 Command / Undo-Redo 框架 | G2 | ICommand + UndoStack 于 dsfw-core |
| T-3.3 类型安全事件总线 | G2 | EventBus 基于 std::any + std::type_index |
| T-3.4 控件稳定性审计 | G4 | 空指针防护 + dsfw-widgets-test 单元测试 |
| T-3.5 通用控件迁移到 dsfw | G4 | dsfw-widgets (SHARED) 含 8 个通用控件，dstools-widgets 保留 4 个 DS 专有控件 |
| T-3.6 CLI 命令行工具 | G6 | dstools-cli 支持 slice/asr/align/pitch/transcribe 子命令 |

---

## Phase 4 — 完善与扩展 ⏳ 基本完成

| 任务 | 目标 | 成果 | 状态 |
|------|------|------|------|
| T-4.1 插件系统 | G2 | PluginManager 基于 QPluginLoader，支持 plugins\<T\>() 查询 | ✅ |
| T-4.2 崩溃收集 | G2 | CrashHandler — Windows MiniDump / Unix signal handler | ✅ |
| T-4.3 自动更新检查 | G2 | UpdateChecker 通过 GitHub Releases API | ✅ |
| T-4.4 MRU 最近文件列表 | G2 | RecentFiles 基于 QSettings | ✅ |
| T-4.5 新增标准控件 | G4 | PropertyEditor/SettingsDialog/LogViewer/ProgressDialog | ✅ |
| T-4.6 控件画廊 App | G4 | WidgetGallery 测试应用 | ❌ 未开始 |
| T-4.7 DI 强化：应用层模型注入 | G3 | 统一通过 ServiceLocator 获取推理引擎 | ❌ 未开始 |

---

## 执行顺序总览

```
Phase 0 (✅) → Phase 1 (✅) → Phase 2 (✅) → Phase 3 (✅) → Phase 4 (⏳ 剩余 T-4.6, T-4.7)
                                                                    │
                                                                    ▼
                                                              Phase 5 — 深度优化
```

---

## 架构决策记录 (ADR)

| ADR | 决策 | 理由 |
|-----|------|------|
| ADR-1 | dsfw-base 为 Qt-free 静态库/header-only | 支持 CLI 工具和非 Qt 消费者 |
| ADR-2 | ModelType 改为 int ID 注册表，非枚举 | 框架层不应感知业务模型类型 |
| ADR-3 | OnnxModelBase 为 protected 继承，不影响公共 API | 各推理 DLL 保持现有 API 稳定 |
| ADR-4 | audio-util 保持应用层，不入框架 | 音频 DSP 过于专业化，不属于通用桌面框架 |
| ADR-5 | dsfw-audio (FFmpeg/SDL2 封装) 归入框架 | 音频播放/解码是通用桌面能力，非 DS 专有 |
| ADR-6 | Undo/Redo 迁移 opt-in | 现有 app 按各自节奏迁移 |
| ADR-7 | FunASR 永远不直接修改 | vendor 代码只用适配器包装 |
| ADR-8 | 所有模块在本仓库内共存，不拆分独立仓库 | 降低维护复杂度，避免双仓库协调开销 |

---

## Phase 5 — 深度优化 (新阶段)

> 基于已完成的架构重构成果，聚焦于：质量加固、架构债清偿、开发体验提升、生产就绪。

### 5.1 架构债清偿

#### T-5.1.1 错误处理统一为 Result\<T\> [ARCH-09] — P1, M (2-8h) ✅ 已完成

**现状**: 三套错误处理模式共存（`Result<T>` / `bool+error` / `throw`）。推理层 `throw std::runtime_error` 穿透 `bool` 返回值 API，调用方未预期异常。

**方案**:
1. 短期：在 `GameModel::forward()`、`HfaModel::forward()` 等入口添加 `try/catch`，异常转为 `errorMsg + return false`
2. 长期：推理层公共 API 统一改为 `Result<T>` 返回值，消除 `bool + std::string &error` 模式
3. 编写 lint 规则或 review checklist 禁止新增 throw-through-bool 模式

**验收标准**:
- [x] 推理层所有 public 方法不再有未捕获异常穿透
- [x] 新增代码统一使用 `Result<T>`

**成果**:
- 修复 HfaModel.cpp 中 `throw std::runtime_error` 穿透 `catch(Ort::Exception)` 的 BUG，添加 `catch(const std::exception&)`
- `Result<T>` 增加 `Ok()`/`Err()` 便捷函数，简化调用方代码
- 推理层公共 API 全部改为 `Result<void>`：`IInferenceEngine::load()`、`OnnxModelBase::loadSession()`/`loadSessionTo()`、`Game::load_model()`/`get_midi()`/`get_notes()`/`align()`/`alignCSV()`、`HFA::load()`/`recognize()`、`Rmvpe::load()`/`get_f0()`
- 框架核心接口全部改为 `Result<T>`：`IDocument`、`IExportFormat`、`IQualityMetrics`、`IModelDownloader`、`IG2PProvider`
- `JsonHelper::loadFile()` 改为 `Result<json>`，`JsonHelper::saveFile()` 改为 `Result<void>`
- `DsItemManager` 方法改为 `Result<T>`
- 消除 `dsfw-core` 与 `dsfw-base` 中 JsonHelper 的重复定义
- 所有调用方已更新适配新签名

---

#### T-5.1.2 应用层推理解耦 [ARCH-08 + T-4.7] — P1, M (2-8h) ✅ 已完成

**现状**: GameAlignPage、MainWidget (GameInfer)、HubertFAPage 直接 `#include` 推理库头文件并管理引擎生命周期，违反分层架构。

**方案**:
1. UI 页面改为通过 `ServiceLocator::get<ITranscriptionService>()` 等服务接口间接使用推理能力
2. 推理引擎实例由 domain 层工厂创建，注册到 ServiceLocator
3. Apps 启动时统一注册（`registerServices()`），UI 层不再直接构造引擎

**验收标准**:
- [x] UI 层 `.cpp` 文件不再 `#include` 任何 `*-infer/*.h` 头文件
- [x] 推理引擎生命周期由 domain 层管理

**成果**:
- 扩展 IAlignmentService 接口：添加 loadModel()/isModelLoaded()/unloadModel()/alignCSV()/vocabInfo()
- 扩展 ITranscriptionService 接口：添加 loadModel()/isModelLoaded()/unloadModel()/exportMidi()/alignCSV()/modelInfo()
- 扩展 IPitchService 接口：添加 loadModel()/isModelLoaded()/unloadModel()/extractPitchWithProgress()
- 创建 GameAlignmentService 和 GameTranscriptionService（libs/gameinfer/）
- 创建 RmvpePitchService（libs/rmvpepitch/）
- GameInferService 实现 ITranscriptionService 接口，注册到 ServiceLocator
- GameAlignPage 改为通过 IAlignmentService 间接使用推理能力
- HubertFAPage 改为通过 IAlignmentService 间接使用推理能力
- MainWidget 移除对 Game::ExecutionProvider 的直接依赖
- UI 层头文件不再 include 任何 *-infer/*.h

---

#### T-5.1.3 MinLabelPage 业务逻辑提取 [ARCH-10] — P2, M (2-8h)

**现状**: MinLabelPage.cpp (~560 行) 包含直接文件 I/O、JSON 解析、.lab 写入、目录扫描等业务逻辑。

**方案**:
1. 提取 `MinLabelService` 到 `src/domain/` 或 `src/libs/`
2. 服务接口: `loadProject()`, `saveLabels()`, `scanDirectory()`, `convertFormat()`
3. MinLabelPage 仅保留 UI 布局和事件绑定

**验收标准**:
- [ ] MinLabelPage 不再包含直接 QFile/QJsonDocument 操作
- [ ] MinLabelService 可独立单元测试

---

#### T-5.1.4 合并两套 Slicer 实现 [ARCH-11] — P2, M (2-8h)

**现状**: `audio-util/Slicer`（C 风格，无 Qt）和 `libs/slicer/Slicer`（Qt 风格）实现相同 RMS 切片算法，互不引用。

**方案**:
1. 以 `audio-util/Slicer` 为底层实现（无 Qt 依赖，纯 C++ DSP）
2. `libs/slicer/Slicer` 改为薄封装，内部调用 audio-util 版本
3. 验证 DatasetPipeline 和 DiffSingerLabeler 功能不变

**验收标准**:
- [ ] 仅保留一套核心算法实现
- [ ] libs/slicer 委托调用 audio-util

---

#### T-5.1.5 dstools-widgets 冗余依赖清理 [ARCH-13] — P3, S (<2h)

移除 dstools-widgets CMakeLists 中对 `dsfw-core` 的冗余显式链接（已通过 dstools-ui-core PUBLIC 传递）。

---

### 5.2 测试与质量

#### T-5.2.1 补齐领域模块单元测试 — P2, L (1-3d)

**待测模块**:

| 模块 | 测试重点 | 需要的测试数据 |
|------|---------|---------------|
| CsvToDsConverter | 正常转换、格式错误输入、边界 CSV | 样本 CSV + 预期 .ds |
| TextGridToCsv | TextGrid 解析、多层级、空区间 | 样本 .TextGrid 文件 |
| PitchProcessor | DSP 编辑算法、边界条件 | 合成 F0 数据 |
| TranscriptionPipeline | 4 步骤独立测试（已拆分） | mock Deps 回调 |

**方案**:
1. 创建 `src/tests/data/` 目录放置测试样本数据
2. 为每个模块编写 QTest 用例
3. 字符串解析纯函数提取（TranscriptionPipeline 中 phSeq/phDur/phNum split→vector 逻辑）

**验收标准**:
- [ ] 4 个待测模块均有单元测试
- [ ] 测试样本数据入库

---

#### T-5.2.2 God class 拆分 [TD-04] — P3, L (1-3d)

5 个超 500 行的文件需要职责拆分：

| 文件 | 行数 | 拆分方向 |
|------|------|---------|
| PitchLabelerPage.cpp | 781 | 文件 I/O → PitchFileService，滚动条 → ScrollController |
| PhonemeLabelerPage.cpp | 630 | 波形渲染 → WaveformRenderer，音频设置 → AudioSettingsController |
| GameModel.cpp | 602 | 5 个 ONNX session 按阶段拆为子处理器 |
| PianoRollView.cpp | 579 | 输入处理 → InputHandler，滚动缩放 → ViewportController |
| MainWidget.cpp (GameInfer) | 505 | 推理编排 → GameInferController |

---

#### T-5.2.3 魔法数字常量化 [TD-05] — P3, S (<2h)

提取为命名常量：
- `4096` (音频 buffer size, 3 处) → `AudioConstants::kDefaultBufferSize`
- 窗口尺寸 `1280×720`, `1200×800` → `DefaultWindowSize`
- `5000` (最小切片长度) → `SlicerDefaults::kMinSliceLength`
- Blackman-Harris 窗函数系数 → `WindowCoefficients::kBlackmanHarris[]`
- Hop size / 采样率范围 → `DspConstants::kDefaultHopSize` 等

---

#### T-5.2.4 TODO/FIXME 清理 [TD-06] — P3, S (<2h)

| 文件 | TODO | 行动 |
|------|------|------|
| GameInferPage.cpp:60 | Forward file paths to MainWidget | 实现文件路径传递 |
| BuildDsPage.cpp:89 | RMVPE-based F0 extraction | 集成 RMVPE 推理或标记为 won't-fix |
| game-infer/tests/main.cpp:322 | Map language string to ID | 从 config.json 读取 language map |

---

### 5.3 CI/CD 与工程化

#### T-5.3.1 CI 矩阵构建 — P1, L (1-3d) ✅ 已完成

**方案**:
1. GitHub Actions 矩阵：Windows (MSVC 2022) / macOS (arm64) / Linux (Ubuntu, GCC)
2. 构建全部 6 个 app + 3 个 test target
3. 运行单元测试并报告结果
4. 缓存 vcpkg 安装目录

**验收标准**:
- [x] 三平台 CI 全绿
- [x] 测试结果上报

**成果**:
- 添加 `refactor` 分支到 CI 触发器
- 添加 `permissions`（contents: read, checks: write）和 `concurrency` 组（取消冗余运行）
- 添加 ccache 加速 Linux/macOS 编译（`hendrikmuhs/ccache-action` + `CMAKE_CXX_COMPILER_LAUNCHER`）
- 添加 vcpkg 已安装包缓存（`actions/cache@v4`，以 vcpkg.json hash 为 key）
- ctest 添加 `--output-junit test-results.xml` 生成 JUnit XML 测试报告
- 添加测试结果上传（`actions/upload-artifact`）和测试报告（`dorny/test-reporter`）
- 添加构建产物验证步骤，检查 6 个 app + 4 个 test target 是否生成

---

#### T-5.3.2 CI 集成 clang-tidy — P2, M (2-8h)

`.clang-tidy` 配置已创建。需要：
1. CMake configure 生成 `compile_commands.json`
2. CI step 运行 `clang-tidy` 检查变更文件
3. PR 注释报告 warnings

---

#### T-5.3.3 CI Doxygen 文档生成 — P3, S (<2h)

Doxyfile 已配置，23 个框架头文件已有 Doxygen 注释。需要：
1. CI step 运行 `doxygen Doxyfile`
2. 发布到 GitHub Pages

---

#### T-5.3.4 各框架模块独立编译 CI 验证 — P2, M (2-8h)

dsfw-core 和 dsfw-ui-core 已添加 `find_package` guards。需要 CI 验证脚本分别独立构建各框架模块，确认外部消费路径畅通。

---

### 5.4 开发体验

#### T-5.4.1 控件画廊 App [T-4.6] — P3, S (<2h)

新建 `src/apps/widget-gallery/`，展示所有 dsfw-widgets 控件 + 代码示例。用于：
- 框架控件功能验证
- 外部开发者快速了解可用组件

---

#### T-5.4.2 示例项目：非 DiffSinger 应用 — P3, M (2-8h)

在 `examples/` 创建一个最小可用的非 DiffSinger 桌面应用，演示 dsfw 框架的独立使用：
- AppShell 单页面模式
- AppSettings 配置
- ServiceLocator DI
- 插件加载

---

#### T-5.4.3 CHANGELOG.md — P2, S (<2h)

建立变更日志，记录 dsfw 公共 API 变更。配合 T-2.2 的语义版本控制。

---

### 5.5 运行时健壮性

#### T-5.5.1 registerDocumentFormat() 线程安全验证 [ARCH-12] — P2, S (<2h)

确认 `DocumentFormatId` 注册模式是否已消除文件作用域静态变量的线程安全问题。若未解决，改为线程安全的注册表实现。

---

#### T-5.5.2 现有 app 迁移至 dsfw Undo/Redo — P3, L (1-3d)

PhonemeLabeler 和 PitchLabeler 已有自定义 undo/redo 实现，可逐步迁移至 dsfw 的 ICommand + UndoStack（ADR-6: opt-in 迁移）。

---

#### T-5.5.3 文件操作错误处理补全 [TD-03] — P3, S (<2h)

补充 `file.open()` 缺少 else 分支的代码点，统一使用 `Result<T>` 包装返回值。

---

## Phase 5 执行优先级总览

```
P1 — 必须优先 (阻塞生产就绪)
  T-5.1.1 错误处理统一 (M)
  T-5.1.2 应用层推理解耦 (M)
  T-5.3.1 CI 矩阵构建 (L)

P2 — 重要 (显著提升质量)
  T-5.1.3 MinLabel 业务逻辑提取 (M)
  T-5.1.4 合并 Slicer 双实现 (M)
  T-5.2.1 补齐领域测试 (L)
  T-5.3.2 clang-tidy CI (M)
  T-5.3.4 框架独立编译验证 (M)
  T-5.4.3 CHANGELOG (S)
  T-5.5.1 注册线程安全验证 (S)

P3 — 锦上添花 (提升体验)
  T-5.1.5 冗余依赖清理 (S)
  T-5.2.2 God class 拆分 (L)
  T-5.2.3 魔法数字常量化 (S)
  T-5.2.4 TODO/FIXME 清理 (S)
  T-5.3.3 Doxygen CI (S)
  T-5.4.1 控件画廊 (S)
  T-5.4.2 示例项目 (M)
  T-5.5.2 Undo/Redo 迁移 (L)
  T-5.5.3 文件操作错误处理 (S)
```

**建议执行顺序**:

```
批次 1 (Week 1-2): T-5.1.1 + T-5.1.2 + T-5.3.1 (并行，解除核心阻塞)
批次 2 (Week 3-4): T-5.1.4 + T-5.2.1 + T-5.3.2 (并行，质量加固)
批次 3 (Week 5-6): T-5.1.3 + T-5.3.4 + T-5.4.3 + T-5.5.1 (并行，收尾 P2)
批次 4 (Week 7+):  P3 任务按需拾取
```
