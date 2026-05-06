# Dataset Tools 编码规范与标准

**版本**：1.0  
**日期**：2026-05-06  
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
| P-05 | 异常边界隔离 | `Result<T>` 传播应用层错误；`try-catch` 仅限第三方库边界 |
| P-06 | 接口稳定 | 公共头文件即契约；接口变更需考虑向后兼容 |
| P-07 | 简洁可靠 | 遇错直接返回，不设计重试或回滚（除明确需求外） |

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

### 路径转换

| 层级 | 入口方法 | 说明 |
|------|---------|------|
| Qt → std::filesystem | `dstools::toFsPath()` 或 `dsfw::PathUtils::toStdPath()` | Windows 使用 wstring，Unix 使用 string |
| std::filesystem → SndfileHandle | `AudioUtil::openSndfile(path)` | Windows 使用 `sf_wchar_open`，Unix 使用 `string()` |
| std::filesystem → std::ifstream | `AudioUtil::openIfstream(path)` | MSVC 直接传 path 对象 |

### 禁止

- 推理层代码中使用 `path.string()` 进行文件 I/O（Windows 上会破坏 CJK 字符）
- 直接构造 `SndfileHandle(path.string())`（应使用 `AudioUtil::openSndfile(path)`）
- 直接构造 `std::ifstream(path.string())`（应使用 `AudioUtil::openIfstream(path)` 或传 path 对象）

### 路径存储

- JSON 文件中的路径统一使用 POSIX 正斜杠 `/`
- 运行时通过 `QDir::toNativeSeparators()` / `QDir::fromNativeSeparators()` 转换
- 优先使用相对路径（相对于 .dsproj 或 .dstext 所在目录）

### macOS 兼容

- macOS HFS+ 使用 NFD（分解形式）Unicode，需通过 `dsfw::PathUtils::normalize()` 做 NFC 标准化
- 路径比较前必须经过 normalize

> 详见 [refactoring-roadmap-v2.md](refactoring-roadmap-v2.md) VII.1 统一路径管理模块。

---

**文档版本**: 1.1  
**最后更新**: 2026-05-06
