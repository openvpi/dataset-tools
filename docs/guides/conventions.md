# Dataset Tools 编码规范与标准

**版本**：1.4
**日期**：2026-06-05
**适用范围**：所有向 dataset-tools 贡献代码的开发者

---

## §1 代码格式

- C++20 标准
- 缩进：4 空格
- 使用 `.clang-format` 和 `.clang-tidy` 统一格式化，提交前确保通过

---

## §2 命名规范

| 项目 | 规范 | 示例 |
|------|------|------|
| 框架命名空间 | `dsfw`（core 多数类：AppPaths, PathUtils, CrashHandler, CrashSafeGuard, FileLogSink, SingleInstanceGuard, TranslationManager, QualityTypes, ISliceDataSource, string_utils, CommonKeys）、`dsfw::widgets`（widgets 组件）、`dsfw::signal`（信号处理）、`dsfw::CommonKeys`（通用配置键）、`dsfw::infer`（推理接口 IInferenceEngine, IInferenceService）、`dstools::audio`（音频）、`dstools::labeler`（页面接口 IPageActions/IPageLifecycle） | `dsfw::Result<T>` |
| 应用命名空间 | `dstools` | `dstools::DsDocument` |
| 接口类 | `I[Name]` | `ITaskProcessor`, `IExportFormat` |
| include 前缀 | `<dsfw/...>` (框架), `<dstools/...>` (领域/音频) | `#include <dsfw/AppSettings.h>` |
| CMake target | `dsfw::[module]`（框架）、`dstools-infer-common::`（推理公共）、`dstools-domain`（领域） | `dsfw::core`, `dsfw::ui-core` |
| 宏前缀 | `DSFW_` | `DSFW_LOG_INFO(...)` |
| 测试类 | `Test[Name]` | `TestAppSettings`, `TestResult` |

---

## §3 错误处理规范

### 基本原则

1. 应用层逻辑使用 `Result<T>` 传播错误，**禁止抛出异常**。
2. `try-catch` **仅**用于第三方库边界，将外部异常转为 `Result<T>::Error()`。
3. 每个 `catch` 块必须记录日志或返回错误，**禁止静默吞掉**。
4. 禁止在业务逻辑中使用 `try-catch` 做流程控制。

### 允许使用 try-catch 的第三方库边界

| 边界 | 来源 | 捕获类型 |
|------|------|----------|
| JSON 解析 | nlohmann/json | `nlohmann::json::exception` |
| ONNX 推理 | ONNX Runtime | `Ort::Exception` |
| 音频解码 | FFmpeg | 错误码（非异常，但需检查） |
| 文件 I/O | Qt/std | `std::exception`（仅在必要时） |

### 错误信息规范

错误消息必须追溯到根因，不得忽略后继续执行。

---

## §4 日志规范

### 日志级别

| 级别 | 用途 |
|------|------|
| Trace | 详细调试信息（循环内数据、逐帧信息） |
| Debug | 开发调试（函数入口、关键分支） |
| Info | 正常操作记录（初始化完成、文件加载） |
| Warning | 可恢复的异常情况（使用默认值、跳过无效项） |
| Error | 不可恢复错误（文件缺失、模型加载失败） |
| Fatal | 致命错误（程序即将崩溃） |

### 日志分类

| 分类字符串 | 用途 |
|-----------|------|
| `"audio"` | 音频解码、播放、重采样 |
| `"infer"` | ONNX 推理、模型加载 |
| `"pipeline"` | PipelineRunner 流程 |
| `"ui"` | UI 组件生命周期 |
| `"io"` | 文件 I/O、格式适配 |
| `"settings"` | 配置读写 |
| `"app"` | 应用启动/关闭 |

### FileLogSink

生产环境使用 `dsfw::createFileLogSink(logDir, appName)` 自由函数创建文件日志 sink，自动 7 天轮转，存储到 `dsfw::AppPaths::logsDir()`。过期日志通过 `dsfw::cleanOldLogFiles(logDir, 7)` 清理。

---

## §5 异步规范

- 任何可能耗时超过 50ms 的操作**禁止在主线程同步执行**
- 模型加载：`QtConcurrent::run` + 信号通知就绪
- 批量推理：`AsyncTask` + `QMetaObject::invokeMethod` 回主线程
- 文件批处理：`PipelineRunner`（后台线程）
- **禁止** `QApplication::processEvents()` 作为"防止卡死"手段
- **禁止** 异步回调中不检查上下文有效性

---

## §6 接口设计规范

- 纯数据接口保持简洁，**不引入 QObject/信号**
- 通知由容器负责（`invalidateXxx()` 一次调用刷新所有子组件）
- 框架定义纯虚接口，应用层提供实现，通过 `ServiceLocator` 注入

---

## §7 模块职责规范

- 相同的行为逻辑只允许存在于一个地方
- 容器提供统一的 `invalidateXxx()` 方法
- 基类实现生命周期骨架（模板方法模式），子类只实现差异化的 hook
- 路径处理、文件 I/O 等平台差异封装在一处

---

## §8 CMake 规范

- 使用 `dstools_add_library()` 宏统一创建库
- `PUBLIC`：接口中暴露的依赖；`PRIVATE`：仅实现内部使用
- 禁止将 PRIVATE 依赖的头文件暴露到公共接口
- 所有测试通过 `add_test()` 注册
- 非 exe 和非 test 的 target 必须有合理的 `FOLDER` 分组

---

## §9 Git 提交规范

格式：`[scope] message`

常用 scope：
- `framework` — dsfw-core/ui-core/widgets/base
- `domain` — dstools-domain
- `infer` — 推理库
- `app` — LabelSuite/DsLabeler
- `build` — CMake/vcpkg/CI
- `docs` — 文档
- `test` — 测试

示例：
```
[framework] Add FileLogSink with 7-day rotation
[app] Fix DsLabeler crash on empty project
[build] Update Qt requirement to 6.8+
```

---

## §10 文件组织

- `include/` — 公共头文件（对外接口）
- `src/` — 实现文件（私有）
- 头文件与源文件一一对应
- 私有头文件放在 `src/` 内，不暴露到 `include/`
- 头文件保护统一使用 `#pragma once`

---

## §11 路径 I/O 规范

### 基本原则

所有文件 I/O 操作必须正确处理 Unicode 路径（含 CJK 字符），尤其是 Windows ANSI codepage。

### 统一路径库

项目提供 `dsfw::PathUtils` 作为唯一的跨平台路径操作入口，禁止各处自行拼接或转换路径。

### 路径转换

| 层级 | 入口方法 | 说明 |
|------|---------|------|
| Qt → std::filesystem | `dsfw::PathUtils::toStdPath()` | Windows 使用 wstring，Unix 使用 string |
| std::filesystem → Qt | `dsfw::PathUtils::fromStdPath()` | Windows 使用 wstring，Unix 使用 string |
| std::filesystem → UTF-8 string | `dsfw::PathUtils::toUtf8()` | 用于错误消息和日志输出 |
| 路径拼接 | `dsfw::PathUtils::join()` | 基于 `operator/`，自动处理分隔符 |
| std::filesystem → SndfileHandle | `AudioUtil::openSndfile(path)` | Windows 使用 `sf_wchar_open` |
| std::filesystem → std::ifstream | `AudioUtil::openIfstream(path)` | MSVC 直接传 path 对象 |

### 禁止

- 推理层代码中使用 `path.string()` 进行文件 I/O
- 使用 `path.string()` 拼接错误消息或日志输出
- 手动字符串拼接路径
- 直接构造 `SndfileHandle(path.string())` 或 `std::ifstream(path.string())`
- 在不同模块中各自实现 `toFsPath()` 等路径转换函数

### 路径存储

- JSON 文件中的路径统一使用 POSIX 正斜杠 `/`
- 运行时通过 `QDir::toNativeSeparators()` / `QDir::fromNativeSeparators()` 转换
- 优先使用相对路径（相对于 .dsproj 或 .dstext 所在目录）

### macOS 兼容

- macOS HFS+ 使用 NFD Unicode，需通过 `dsfw::PathUtils::normalize()` 做 NFC 标准化
- 路径比较前必须经过 normalize

---

## §12 编码处理规范

### 基本原则

所有编码转换必须通过 `dsfw::PathUtils`，禁止在业务代码中直接调用平台相关编码 API。

### 编码转换矩阵

| 源 | 目标 | 方法 | 场景 |
|----|------|------|------|
| `QString` | `std::filesystem::path` | `PathUtils::toStdPath()` | 所有文件 I/O |
| `std::filesystem::path` | `QString` | `PathUtils::fromStdPath()` | Qt UI 显示 |
| `std::filesystem::path` | `std::string` (UTF-8) | `PathUtils::toUtf8()` | 错误消息、日志 |
| `std::filesystem::path` | `std::wstring` | `PathUtils::toWide()` | Windows API 调用 |
| `const char*` (UTF-8) | `QString` | `QString::fromUtf8()` | 读取 UTF-8 文本 |
| `const wchar_t*` | `QString` | `QString::fromWCharArray()` | Windows API 返回值 |

### 文本文件编码

- 内部文件（.dsproj, .dstext, .json）统一使用 **UTF-8 无 BOM**
- 读取外部文件时需检测编码（BOM 标记），自动转换为 UTF-8
- 写出文件一律使用 UTF-8 无 BOM

### 禁止

- 在 Windows 上使用 `path.string()` 获取路径字符串
- 在日志/错误消息中使用 `path.string()`（应使用 `PathUtils::toUtf8()`）
- 使用 `QString::toLocal8Bit()` 传递路径给 C API
- 在不同模块中重复实现编码转换逻辑

---

## §13 多线程安全规范

### 基本原则

批处理和多页面设计必须考虑多线程竞争。任何可能被多线程同时访问的可变状态，必须通过锁或原子操作保护。

### 共享资源保护

| 场景 | 保护方式 |
|------|---------|
| 推理引擎（ONNX Session） | `std::mutex` + `std::lock_guard` |
| 日志系统（Logger） | `std::mutex` 保护 sink 列表和 ring buffer |
| 配置读写（AppSettings） | `std::mutex` 保护定时保存和 reload |
| 注册表（ServiceLocator/Registry） | `std::mutex` 保护注册/查询 |
| 异步任务中的原始指针 | `std::shared_ptr<std::atomic<bool>>` 存活令牌 |

### 禁止

- 在后台线程直接操作 UI 组件
- 使用裸指针在异步任务间传递可变状态
- 假设单线程访问而不加保护（除非明确注释说明仅在主线程使用）

---

## §14 相似模块统一设计规范

### 基本原则

相似模块使用相似的设计思路。大部分功能相同的模块尽量使用同一个类 + 配置开关，而非多个高度相似的类。

### 判断标准

| 相同代码比例 | 方案 |
|-------------|------|
| >60% | 合并为同一类 + 配置开关 |
| 30%~60% | 提取共同基类，差异通过虚方法/配置实现 |
| <30% | 可独立实现，但接口风格应保持一致 |

### 已知已统一的模块

| 模块对 | 状态 |
|--------|------|
| SlicerPage / DsSlicerPage | DsSlicerPage 继承 SlicerPage，消除 ~90% 重复代码 |
| MinLabelPage / PhonemeLabelerPage / PitchLabelerPage | `runBatchProcess()` 已提取到基类 |
| WaveformWidget / SpectrogramWidget / PowerWidget | 拖拽逻辑已提取为 BoundaryDragController；旧 Widget 已由 ChartPanel 系列取代 |

---

## §15 文档模型与适配器隔离规范

### 基本原则

本项目维护统一的内部文档模型（`DsDocument`、`TextGridDocument` 等），所有具体文件格式**一律通过 `IFormatAdapter` 适配器**对接内部模型。**业务代码禁止直接操作文件**。

### 架构模式

```
业务代码层                适配器层                底层 I/O
EditorPageBase            IFormatAdapter          QFile / JsonHelper / PathUtils
  DsDocument (内部模型)      CsvAdapter
  TextGridDocument           DsFileAdapter
  ExportService              TextGridAdapter
  ITaskProcessor             LabAdapter
```

### 核心规则

1. **内部模型是唯一的真相源**：所有页面、处理流程、推理引擎只与内部模型交互。
2. **格式 I/O 必须经过适配器**：每种文件格式对应一个 `IFormatAdapter` 实现。
3. **迁移模块必须重构为适配器模式**：提取内部模型 + 适配器，业务代码只与内部模型交互。
4. **适配器内部可直接使用 QFile + PathUtils**：无需额外 I/O 抽象层。

### 新增格式检查清单

1. 该格式是否有对应的 `IFormatAdapter` 实现？
2. 适配器已注册到 `FormatAdapterRegistry`？
3. 业务代码是否只通过 `FormatAdapterRegistry` 获取适配器？
4. 适配器导入的目标是否是内部文档模型？
5. 如从其他项目迁移模块，是否已完成"提取内部模型 + 新增适配器"重构？

---

## §9 ChartConfig 图表参数配置化

所有图表渲染参数通过 `ChartConfigRegistry` 统一管理，支持运行时修改和持久化存储。
各图表在 `registerChartConfig()` 中注册参数，`loadConfigParams()` 中加载。

| 图表类型 | 可配置参数数 |
|----------|------------|
| 频谱图 (Spectrogram) | 5 |
| 功率图 (Power) | 3 |
| 波形图 (Waveform) | 3 |
| 钢琴卷帘 (PianoRoll) | 10 |
| 音高提取 (Pitch) | 2 |
| **合计** | **23** |

---

**文档版本**: 1.4
**最后更新**: 2026-06-05
