# Dataset Tools 编码规范与标准

**版本**：1.2  
**日期**：2026-05-09  
**适用范围**：所有向 dataset-tools 贡献代码的开发者

---

## §1 代码格式

项目使用 `.clang-format` 和 `.clang-tidy` 统一格式化。

- C++20 标准
- 缩进：4 空格
- 提交前请确保代码通过 clang-format 格式化

---

## §2 命名规范

| 项目 | 规范 | 示例 |
|------|------|------|
| 框架命名空间 | `dstools`、`dsfw`、`dsfw::widgets` | `dstools::Result<T>` |
| 应用命名空间 | `dstools` | `dstools::DsDocument` |
| 接口类 | `I[Name]` | `ITaskProcessor`, `IModelManager` |
| include 前缀 | `<dsfw/...>` (框架), `<dstools/...>` (领域) | `#include <dsfw/AppSettings.h>` |
| CMake target | `dsfw::[module]` | `dsfw::core`, `dsfw::ui-core` |
| 宏前缀 | `DSFW_` | `DSFW_LOG_INFO(...)` |
| 测试类 | `Test[Name]` | `TestAppSettings`, `TestResult` |

---

## §3 错误处理规范

### 基本原则

1. 应用层逻辑使用 `Result<T>` 传播错误，**禁止抛出异常**。
2. `try-catch` **仅**用于第三方库边界，将外部异常转为 `Result<T>::Error()`。
3. 每个 `catch` 块必须记录日志或返回错误，**禁止静默吞掉**。
4. 禁止在业务逻辑中使用 `try-catch` 做流程控制。

### Result<T> 使用模式

```cpp
// 返回成功值
Result<int> compute() {
    return 42;  // 隐式转换
}

// 返回错误
Result<void> initialize() {
    if (!file.exists())
        return Err("Config file not found: " + path);
    return Ok();
}

// 错误传播（检查后立即返回）
Result<void> process() {
    auto data = loadData();
    if (!data)
        return Err("Failed to load: " + data.error());
    // 使用 data.value() ...
    return Ok();
}
```

### 允许使用 try-catch 的第三方库边界

| 边界 | 来源 | 捕获类型 |
|------|------|----------|
| JSON 解析 | nlohmann/json | `nlohmann::json::exception` |
| ONNX 推理 | ONNX Runtime | `Ort::Exception` |
| 音频解码 | FFmpeg | 错误码（非异常，但需检查） |
| 文件 I/O | Qt/std | `std::exception`（仅在必要时） |

新增第三方库集成时，在调用入口处统一捕获并转为 `Result<T>::Error()`。

### 错误信息规范（P-04）

错误消息必须追溯到根因：

```cpp
// 错误示例：
return Err("Processing failed");  // 缺少根因

// 正确示例：
return Err("Failed to resample audio: " + ffmpegError);  // 包含根因
```

调用者收到错误后，**必须在第一时间检查并传播**，不得忽略后继续执行：

```cpp
// 禁止：
auto result = resample(input, output, msg);
// 忽略 msg，继续读 output... 

// 正确：
auto result = resample(input, output);
if (!result)
    return Err("Resample failed: " + result.error());
```

---

## §4 日志规范

### 基本用法

```cpp
#include <dsfw/Log.h>

DSFW_LOG_INFO("audio", "Loaded file: " + filePath.toStdString());
DSFW_LOG_WARN("infer", "Model path not set, using default");
DSFW_LOG_ERROR("pipeline", "Step failed: " + errorMsg);
```

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

生产环境使用 `dsfw::FileLogSink`，自动 7 天轮转，存储到 `dsfw::AppPaths::logsDir()`。

---

## §5 异步规范（P-03）

### 基本原则

任何可能耗时超过 50ms 的操作**禁止在主线程同步执行**。

### 实现要求

| 操作 | 实现方式 |
|------|---------|
| 模型加载 | `QtConcurrent::run` + 信号通知就绪 |
| 批量推理 | `AsyncTask` + `QMetaObject::invokeMethod` 回主线程 |
| 文件批处理 | `PipelineRunner`（后台线程） |
| 单文件导入 | 如 < 50ms 可同步，否则异步 |

### 禁止

- `QApplication::processEvents()` 作为"防止卡死"手段
- 异步回调中不检查上下文有效性（页面/切片是否仍为当前）

---

## §6 接口设计规范（P-02）

### 被动数据接口 + 容器通知

纯数据接口保持简洁，**不引入 QObject/信号**：

```cpp
// 正确：纯数据接口
class IBoundaryModel {
public:
    virtual int boundaryCount() const = 0;
    virtual double boundaryTime(int index) const = 0;
};

// 通知由容器负责
class AudioVisualizerContainer {
    void invalidateBoundaryModel();  // 一次调用刷新所有子组件
};
```

### ServiceLocator 注入

框架定义纯虚接口，应用层提供实现，通过 `ServiceLocator` 注入：

```cpp
ServiceLocator::set<IG2PProvider>(new PinyinG2PProvider());
auto *g2p = ServiceLocator::get<IG2PProvider>();
```

---

## §7 模块职责规范（P-01）

### 行为不得分散重复

相同的行为逻辑只允许存在于一个地方：

- 容器提供统一的 `invalidateXxx()` 方法，一次调用刷新所有子组件
- 基类实现生命周期骨架（模板方法模式），子类只实现差异化的 hook
- 路径处理、文件 I/O 等平台差异封装在一处（`PathUtils`），所有模块调同一个函数

### 禁止

- 同一操作在每个页面各写一份
- Widget 内部手动调用多个兄弟 widget 的 `update()`
- 每个页面各自拼凑生命周期序列

---

## §8 设计原则

| 编号 | 原则 | 含义 |
|------|------|------|
| P-01 | 模块职责单一 | 相同行为只存在一处；刷新/通知由容器统一分发 |
| P-02 | 被动接口 + 容器通知 | 纯数据接口不加 QObject；变更通知由容器的 `invalidateXxx()` 负责 |
| P-03 | 异步一切 | 超过 50ms 的操作禁止主线程同步执行 |
| P-04 | 错误根因传播 | 错误消息必须追溯到根因，不得忽略 out-parameter 继续报二次错误 |
| P-05 | 异常边界隔离 | `Result<T>` 传播应用层错误；`try-catch` 仅限第三方库边界，非必要不用 try-catch |
| P-06 | 接口稳定 | 公共头文件即契约；接口变更需考虑向后兼容 |
| P-07 | 简洁可靠 | 遇错直接返回，不设计重试或回滚（除明确需求外） |
| P-08 | 每页独立资源 | ServiceLocator 仅用于真正全局服务；页面资源不得全局共享 |
| P-09 | 异步任务存活令牌 | `QtConcurrent::run` 中捕获的原始指针必须持有 `shared_ptr<atomic<bool>>` 存活令牌 |
| P-10 | 统一路径库 | 基于 `std::filesystem` 的跨平台路径库，路径拼接、编码转换、debug 输出均使用此库，禁止各处自行拼接 |
| P-11 | 多线程安全优先 | 批处理和多页面设计必须考虑多线程竞争，共享资源必须加锁或使用原子操作 |
| P-12 | 相似模块统一设计 | 相似模块使用相似的设计思路；大部分功能相同的模块尽量使用同一个类、不同实例按需开启部分功能 |

---

## §9 CMake 规范

### 库创建

使用项目宏统一创建：

```cmake
dstools_add_library(my-lib STATIC
    SOURCES src/foo.cpp src/bar.cpp
    PUBLIC_HEADERS include/dsfw/Foo.h
)
```

### 依赖声明

- `PUBLIC`：接口中暴露的依赖（下游传递）
- `PRIVATE`：仅实现内部使用
- 禁止将 PRIVATE 依赖的头文件暴露到公共接口

### 测试注册

所有测试通过 `add_test()` 注册，可用 `ctest --output-on-failure` 统一运行。

### IDE 分组

非 exe 和非 test 的 target 必须有合理的 `FOLDER` 分组。

---

## §10 Git 提交规范

### Commit Message

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

## §11 文件组织

### 目录约定

- `include/` — 公共头文件（对外接口）
- `src/` — 实现文件（私有）
- 头文件与源文件一一对应
- 私有头文件放在 `src/` 内，不暴露到 `include/`

### 头文件保护

统一使用 `#pragma once`。

---

## §12 路径 I/O 规范

### 基本原则

所有文件 I/O 操作必须正确处理 Unicode 路径（含中文、日文等 CJK 字符），尤其是 Windows 上 ANSI codepage 不支持的字符。

### 统一路径库（P-10）

项目提供 `dsfw::PathUtils` 作为唯一的跨平台路径操作入口，禁止各处自行拼接或转换路径：

```cpp
#include <dsfw/PathUtils.h>

// 路径拼接 — 使用 PathUtils::join，禁止手动字符串拼接
auto configPath = PathUtils::join(modelDir, "config.json");

// 编码转换 — 使用 PathUtils 提供的方法
auto fsPath = PathUtils::toStdPath(qStringPath);  // QString → fs::path
auto qPath = PathUtils::fromStdPath(fsPath);       // fs::path → QString

// 路径输出到 debug 信息 — 使用 PathUtils::toUtf8，禁止 path.string()
auto msg = "Cannot open file: " + PathUtils::toUtf8(filePath);
DSFW_LOG_ERROR("io", msg.c_str());
```

### 路径转换

| 层级 | 入口方法 | 说明 |
|------|---------|------|
| Qt → std::filesystem | `dsfw::PathUtils::toStdPath()` | Windows 使用 wstring，Unix 使用 string |
| std::filesystem → Qt | `dsfw::PathUtils::fromStdPath()` | Windows 使用 wstring，Unix 使用 string |
| std::filesystem → UTF-8 string | `dsfw::PathUtils::toUtf8()` | 用于错误消息和日志输出，替代 `path.string()` |
| 路径拼接 | `dsfw::PathUtils::join()` | 基于 `operator/`，自动处理分隔符 |
| std::filesystem → SndfileHandle | `AudioUtil::openSndfile(path)` | Windows 使用 `sf_wchar_open`，Unix 使用 `string()` |
| std::filesystem → std::ifstream | `AudioUtil::openIfstream(path)` | MSVC 直接传 path 对象 |

### 禁止

- 推理层代码中使用 `path.string()` 进行文件 I/O（Windows 上会破坏 CJK 字符）
- 使用 `path.string()` 拼接错误消息或日志输出（Windows 上 CJK 路径会乱码，应使用 `PathUtils::toUtf8()`）
- 手动字符串拼接路径（如 `dir + "/" + filename` 或 `dir + "\\" + filename`）
- 直接构造 `SndfileHandle(path.string())`（应使用 `AudioUtil::openSndfile(path)`）
- 直接构造 `std::ifstream(path.string())`（应使用 `AudioUtil::openIfstream(path)` 或传 path 对象）
- 在不同模块中各自实现 `toFsPath()` 等路径转换函数（应统一使用 `dsfw::PathUtils`）

### 路径存储

- JSON 文件中的路径统一使用 POSIX 正斜杠 `/`
- 运行时通过 `QDir::toNativeSeparators()` / `QDir::fromNativeSeparators()` 转换
- 优先使用相对路径（相对于 .dsproj 或 .dstext 所在目录）

### macOS 兼容

- macOS HFS+ 使用 NFD（分解形式）Unicode，需通过 `dsfw::PathUtils::normalize()` 做 NFC 标准化
- 路径比较前必须经过 normalize

---

## §13 编码处理规范

### 基本原则

所有涉及编码转换的操作必须通过 `dsfw::PathUtils` 或 `dsfw::Encoding` 模块完成，禁止在业务代码中直接调用平台相关的编码 API。

### 编码转换矩阵

| 源 | 目标 | 方法 | 场景 |
|----|------|------|------|
| `QString` | `std::filesystem::path` | `PathUtils::toStdPath()` | 所有文件 I/O |
| `std::filesystem::path` | `QString` | `PathUtils::fromStdPath()` | Qt UI 显示 |
| `std::filesystem::path` | `std::string` (UTF-8) | `PathUtils::toUtf8()` | 错误消息、日志、debug 输出 |
| `std::filesystem::path` | `std::wstring` | `PathUtils::toWide()` | Windows API 调用 |
| `std::filesystem::path` | `std::string` (narrow) | `PathUtils::toNarrowPath()` | 旧式 C API（仅限无法避免时） |
| `const char*` (UTF-8) | `QString` | `QString::fromUtf8()` | 读取 UTF-8 文本 |
| `const wchar_t*` | `QString` | `QString::fromWCharArray()` | Windows API 返回值 |

### 文本文件编码

- 项目内部文件（.dsproj, .dstext, .json）统一使用 **UTF-8 无 BOM**
- 读取外部文件时需检测编码（BOM 标记），自动转换为 UTF-8
- 写出文件一律使用 UTF-8 无 BOM

### 禁止

- 在 Windows 上使用 `path.string()` 获取路径字符串（返回 ANSI 编码，CJK 乱码）
- 在日志/错误消息中使用 `path.string()`（应使用 `PathUtils::toUtf8()`）
- 使用 `QString::toLocal8Bit()` 传递路径给 C API（Windows 上可能丢失字符）
- 在不同模块中重复实现编码转换逻辑

---

## §14 多线程安全规范（P-11）

### 基本原则

批处理和多页面设计必须考虑多线程竞争问题。任何可能被多线程同时访问的可变状态，必须通过锁或原子操作保护。

### 共享资源保护

| 场景 | 保护方式 |
|------|---------|
| 推理引擎（ONNX Session） | `std::mutex` + `std::lock_guard`，防止并发推理 |
| 日志系统（Logger） | `std::mutex` 保护 sink 列表和 ring buffer |
| 配置读写（AppSettings） | `std::mutex` 保护定时保存和 reload |
| 注册表（ServiceLocator/Registry） | `std::mutex` 保护注册/查询 |
| 异步任务中的原始指针 | `std::shared_ptr<std::atomic<bool>>` 存活令牌（P-09） |

### 线程安全检查清单

新增或修改涉及多线程的代码时，必须检查：

1. **数据竞争**：是否有两个线程可能同时读写同一变量？
2. **生命周期**：异步任务中捕获的指针/引用，在任务执行时是否仍有效？
3. **信号槽跨线程**：`QtConcurrent::run` 中的信号发射是否通过 `QMetaObject::invokeMethod` 回到主线程？
4. **取消机制**：长时间后台任务是否有取消检查点？

### 禁止

- 在后台线程直接操作 UI 组件（必须通过信号/`QMetaObject::invokeMethod` 回主线程）
- 使用裸指针在异步任务间传递可变状态
- 假设单线程访问而不加保护（除非有明确注释说明仅在主线程使用）

---

## §15 相似模块统一设计规范（P-12）

### 基本原则

相似模块使用相似的设计思路。大部分功能相同的模块尽量使用同一个类、不同实例按需开启部分功能，而非创建多个高度相似的类。

### 设计模式

```cpp
// 正确：同一类 + 配置开关
class EditorPageBase {
public:
    struct Config {
        bool enableAutoInfer = true;
        bool enableBatchProcess = true;
        bool enableSliceList = true;
    };
    
    EditorPageBase(const Config &config, QWidget *parent = nullptr);
};

// SlicerPage 和 PhonemeLabelerPage 使用不同的 Config 实例
// 而非各自实现一套几乎相同的逻辑
```

### 判断标准

- 两个类有 **>60%** 相同代码 → 应合并为同一类 + 配置开关
- 两个类有 **30%~60%** 相同代码 → 提取共同基类，差异通过虚方法/配置实现
- 两个类有 **<30%** 相同代码 → 可独立实现，但接口风格应保持一致

### 已知需统一的模块

| 模块对 | 当前状态 | 统一方向 |
|--------|---------|---------|
| SlicerPage / DsSlicerPage | ~60% 代码重复 | DsSlicerPage 继承 SlicerPage，仅 override 工程保存逻辑 |
| MinLabelPage / PhonemeLabelerPage / PitchLabelerPage | 批量处理逻辑重复 | 提取到 EditorPageBase 基类 |
| WaveformWidget / SpectrogramWidget / PowerWidget | 拖拽逻辑重复 | 提取 BoundaryDragController |

---

**文档版本**: 1.2  
**最后更新**: 2026-05-09
