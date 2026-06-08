# 精简重构方案 v6.0.0

> 本文档设计于 2026-06-08，基于项目当前代码全量探索结果制定。
> 方案严格遵循 `docs/human-decisions.md` 设计准则，保证功能不变、行为一致，仅做精简与规范化。
>
> 修正记录：
> - v6.0.0：重新设计阶段顺序（准则先行→代码清理→架构修复→信号解耦→性能优化），
>   修正阶段七 OPT-1 实现方案（保留 paintEvent 中的 rebuildCache 以兼容 resizeEvent），
>   阶段五按模块拆分、纳入 `using namespace dsfw` 头文件清理。

---

## 1. 概述

### 1.1 目标

- 补充工程准则，覆盖 C++/Qt 项目最佳实践
- 清理死代码与历史包袱，消除技术债与架构债
- 修复 `dsfw-ui-core` ↔ `dsfw-widgets` 循环依赖（SUP-05）
- 统一文件选择路径记忆行为，消除各应用不一致
- 清理全局 `using namespace dsfw` 污染（INFRA-06 延伸）
- 解除接口对 Qt 信号的依赖，符合 ARCH-02 准则
- 图表渲染性能优化（保证逻辑等价）

### 1.2 原则

- 功能完全不变，行为一致
- 数据稳定性、安全性不降级
- 符合全部设计准则与人工决策
- 不引入新的外部依赖
- 不修改稳定核心代码实现新功能（新增类/适配器替代）

### 1.3 阶段总览

| 阶段 | 内容 | 风险 | 依赖 | 预估变更文件数 |
|------|------|------|------|---------------|
| 一 | 补充工程准则 SUP-01~SUP-10 | 低 | 无 | 1 |
| 二 | 清理死代码与历史包袱 | 低 | 无 | 3 |
| 三 | 修复 `dsfw-ui-core` ↔ `dsfw-widgets` 循环依赖 | 低 | 无 | 7 |
| 四 | 统一文件选择路径记忆行为 | 低 | 无 | 6 |
| 五 | 清理 `using namespace dsfw` 污染 | 低 | 无 | 55+ |
| 六 | 解除接口对 Qt 信号的依赖 | 中 | 无 | 11 |
| 七 | 图表渲染性能优化 | 低 | 无 | 4 |

执行顺序：阶段一 → 阶段二 → 阶段三 → 阶段四 → 阶段五 → 阶段六 → 阶段七。每个阶段独立编译验证后提交。

---

## 2. 阶段一：补充工程准则 (SUP-01 ~ SUP-10)

> 以下准则基于 LLVM Coding Standards、KDE Frameworks、Qt Best Practices、Chromium C++ Style Guide、Abseil Tip of the Week 等优秀 C++/Qt 项目提炼，补充至 `human-decisions.md`。

### SUP-01：编译期检查优先于运行时检查

优先使用 `static_assert`、`constexpr`、`consteval`、`requires` 在编译期捕获错误。类型安全优于运行时断言。

```cpp
// 推荐
static_assert(sizeof(Header) == 64, "Header size mismatch");
template <typename T> requires std::is_arithmetic_v<T>
auto compute(T value) -> T;

// 避免
assert(sizeof(Header) == 64);
```

### SUP-02：数据成员默认值优先于构造函数初始化列表冗余赋值

C++ 类内初始化器（NSDMI）优先级高于构造函数初始化列表，减少重复。

```cpp
// 推荐
class Widget {
    int m_count = 0;
    QString m_name;
};

// 避免
class Widget {
    int m_count;
    Widget() : m_count(0) {}
};
```

### SUP-03：禁止隐式类型转换

单参数构造函数标记 `explicit`，禁止隐式 bool 转换和隐式数值转换。

```cpp
// 推荐
explicit PathSelector(const Config &config, QWidget *parent = nullptr);

// 避免
PathSelector(const Config &config);  // 可能被隐式调用
```

### SUP-04：公开接口明确所有权语义

- 裸指针（`T*`）= 非持有、可为空
- 引用（`T&`）= 非持有、非空
- `std::unique_ptr<T>` = 独占所有权
- `std::shared_ptr<T>` = 共享所有权
- 禁止 `std::auto_ptr`、`std::unique_ptr` 的 `release()` 裸指针传递

```cpp
// 推荐
void process(const Config &config);               // 非持有，非空
void setParent(QWidget *parent);                   // 非持有，可为空
std::unique_ptr<IDecoder> createDecoder();         // 转移所有权
```

### SUP-05：模块间循环依赖零容忍

模块依赖必须形成有向无环图（DAG）。任何新增依赖必须通过依赖图验证无环。

```
dsfw-types → dsfw-core → dsfw-audio → dsfw-widgets
    ↓            ↓             ↓
 dstools-domain → dstools-ui-core → dstools-apps
```

### SUP-06：头文件自包含

每个 `.h` 文件必须可独立编译，不依赖使用者预先包含其他头文件。验证方式：每个 `.h` 在关联的 `.cpp` 中作为第一个 include。

```cpp
// Foo.cpp
#include "Foo.h"  // 必须是第一个 include
#include <QWidget>
// ...
```

### SUP-07：禁止可变全局状态

禁止 `mutable static` 变量、全局 `std::map`/`std::vector` 等可变容器。全局常量允许（`constexpr`、`const`）。

```cpp
// 允许：编译期常量
constexpr int kMaxRetries = 3;
inline const std::string kAppName = "dataset-tools";

// 禁止：可变全局状态
static std::map<QString, int> g_counter;  // 多线程不安全
```

### SUP-08：资源获取即初始化无例外

所有资源（内存、文件句柄、锁、线程）必须通过 RAII 管理。禁止手动 `new`/`delete`、`fopen`/`fclose`、`lock()`/`unlock()`。

```cpp
// 推荐
auto file = std::make_unique<QFile>(path);
std::lock_guard lock(m_mutex);
std::jthread worker{std::move(task)};

// 禁止
auto *file = new QFile(path);
m_mutex.lock();
```

### SUP-09：数值计算使用标准库

时间用 `std::chrono`，跨度用 `std::span`（C++20），浮点比较用 `std::fabs(a - b) < epsilon`。禁止裸 `int64_t` 表示时间戳。

```cpp
// 推荐
using namespace std::chrono_literals;
auto timeout = 500ms;
auto elapsed = std::chrono::steady_clock::now() - start;

// 避免
int64_t timeoutMs = 500;
```

### SUP-10：日志分级与结构化

使用 `DSFW_LOG_*` 宏，禁止 `printf`/`std::cout`/`qDebug()` 直接输出。日志级别：TRACE（开发调试）、DEBUG（诊断信息）、INFO（关键节点）、WARN（可恢复异常）、ERROR（不可恢复错误）。

```cpp
// 推荐
DSFW_LOG_INFO("audio", std::format("Decoded {} frames in {:.2f}s", frames, elapsed));

// 禁止
qDebug() << "Decoded" << frames << "frames";
std::cout << "Decoded " << frames << " frames\n";
```

### 2.1 影响范围

- 仅修改 `docs/human-decisions.md`，追加 SUP-01~SUP-10 至速查表与正文
- 无代码变更

---

## 3. 阶段二：清理死代码与历史包袱

### 3.1 移除空 `.gitkeep` 文件

| 文件 | 原因 |
|------|------|
| `src/framework/core/include/dsfw/.gitkeep` | 空目录标记文件，无实际用途 |
| `src/framework/core/src/.gitkeep` | 空目录标记文件，无实际用途 |

### 3.2 移除 `AUDIO_UTIL_BUILD_TESTS` 残留引用

**文件**: `src/CMakeLists.txt` 第 8 行

```cmake
# 删除以下行:
set(AUDIO_UTIL_BUILD_TESTS OFF CACHE BOOL "" FORCE)
```

原因：`src/infer/audio-util/` 目录已不存在，该 CMake 变量为死引用。

### 3.3 影响范围

- 无功能影响，仅清理构建系统残留
- 编译行为不变

---

## 4. 阶段三：修复 `dsfw-ui-core` ↔ `dsfw-widgets` 循环依赖

### 4.1 问题分析

当前存在循环依赖，违反 SUP-05（模块间循环依赖零容忍）：

```
dsfw-ui-core ──(include)──→ dsfw-widgets
    ↑                           │
    └────────(CMake PRIVATE)────┘
```

具体依赖关系：

| 方向 | 文件 | 依赖内容 |
|------|------|----------|
| ui-core → widgets | `LogPanelWidget.cpp` | `#include <dsfw/widgets/LogViewer.h>`, `#include <dsfw/FileDialogHelper.h>` |
| ui-core → widgets | `AppShell.h` / `AppShell.cpp` | `#include "dsfw/widgets/ToastNotification.h"` |
| widgets → ui-core | `ShortcutEditorWidget.cpp` | `#include <dsfw/FramelessHelper.h>` |
| widgets → ui-core | `CMakeLists.txt` | `PRIVATE dsfw-ui-core` |

### 4.2 方案

将 `FramelessHelper` 从 `dsfw-ui-core` 移至 `dsfw-widgets`，移除 `dsfw-widgets` 对 `dsfw-ui-core` 的 CMake 依赖。

理由：
- `FramelessHelper` 是窗口工具类，语义上属于 widgets 层
- `ShortcutEditorWidget`（widgets 层）和 `AppShell`（ui-core 层）均使用它
- `AppShell` 已依赖 `dsfw-widgets`（`ToastNotification`），移动后不新增依赖

### 4.3 文件变更

| 操作 | 文件 | 说明 |
|------|------|------|
| 移动 | `ui-core/include/dsfw/FramelessHelper.h` → `widgets/include/dsfw/widgets/FramelessHelper.h` | 移至 widgets 模块 |
| 移动 | `ui-core/src/FramelessHelper.cpp` → `widgets/src/FramelessHelper.cpp` | 移至 widgets 模块 |
| 修改 | `widgets/CMakeLists.txt` | 移除 `PRIVATE dsfw-ui-core` 依赖 |
| 修改 | `ui-core/CMakeLists.txt` | 移除 FramelessHelper 源文件 |
| 修改 | `ShortcutEditorWidget.cpp` | `#include <dsfw/FramelessHelper.h>` → `<dsfw/widgets/FramelessHelper.h>` |
| 修改 | `AppShell.cpp` | `#include <dsfw/FramelessHelper.h>` → `<dsfw/widgets/FramelessHelper.h>` |
| 修改 | `FramelessHelper.cpp` | 更新自身 include 路径 |

### 4.4 影响范围

- 仅移动文件，逻辑不变
- 解除循环依赖后，`dsfw-widgets` 可独立编译（不再依赖 `dsfw-ui-core`）

---

## 5. 阶段四：统一文件选择路径记忆行为

### 5.1 问题分析

当前文件选择场景存在路径记忆（`RecentPathStore`）行为不一致：

| 类 | 基类 | 用途 | 是否使用 RecentPathStore |
|----|------|------|--------------------------|
| `PathSelector` | QWidget | 嵌入式控件（行编辑+浏览按钮），用于设置面板 | 是 |
| `FilePathSelector` | QObject | 模态对话框（立刻弹窗返回路径），用于 SlicerPage、导出页等 | 是 |
| `FileDialogHelper` 裸调用 | 静态工具类 | `TaskWindow`、`DroppableFileListPanel`、`LogPanelWidget` 直接使用 | **否** |

`FileDialogHelper` 是对 `QFileDialog` 静态方法的薄封装，`PathSelector` 和 `FilePathSelector` 内部均通过它调用对话框。但 `TaskWindow` 和 `DroppableFileListPanel` 绕过了 `FilePathSelector`，直接使用 `FileDialogHelper`，导致路径记忆功能缺失。

此外，`FileDialogHelper` 位于 `dsfw/widgets/include/dsfw/FileDialogHelper.h`，但包含路径为 `<dsfw/FileDialogHelper.h>`（无 `widgets/` 子目录），与同目录下其他 widget 头文件命名不一致。

### 5.2 方案

**不合并 PathSelector 和 FilePathSelector**。两者服务于不同的 UX 模式：
- `PathSelector`：嵌入表单布局，显示行编辑+浏览按钮
- `FilePathSelector`：模态对话框，立即弹出返回路径

**统一路径记忆**：所有文件选择场景均通过 `FilePathSelector` 或 `PathSelector`，确保 `RecentPathStore` 集成一致。

**修正 FileDialogHelper 位置**：将 `FileDialogHelper.h` 移到 `dsfw/widgets/include/dsfw/widgets/FileDialogHelper.h`，包含路径改为 `<dsfw/widgets/FileDialogHelper.h>`。

### 5.3 迁移路径

| 原用法 | 新用法 |
|--------|--------|
| `TaskWindow.cpp`: `FileDialogHelper::getOpenFileNames({this, ...})` | `FilePathSelector({Mode::OpenFiles, ...}, this).exec()` + `selectedPaths()` |
| `TaskWindow.cpp`: `FileDialogHelper::getExistingDirectory({this, ...})` | `FilePathSelector({Mode::OpenDirectory, ...}, this).exec()` + `selectedPath()` |
| `DroppableFileListPanel.cpp`: `FileDialogHelper::getOpenFileNames({this, ...})` | `FilePathSelector({Mode::OpenFiles, ...}, this).exec()` + `selectedPaths()` |
| `DroppableFileListPanel.cpp`: `FileDialogHelper::getExistingDirectory({this, ...})` | `FilePathSelector({Mode::OpenDirectory, ...}, this).exec()` + `selectedPath()` |
| `LogPanelWidget.cpp`: `FileDialogHelper::getSaveFileName({this, ...})` | `FilePathSelector({Mode::SaveFile, ...}, this).exec()` + `selectedPath()` |

### 5.4 文件变更

| 操作 | 文件 | 说明 |
|------|------|------|
| 移动 | `dsfw/FileDialogHelper.h` → `dsfw/widgets/FileDialogHelper.h` | 修正命名不一致 |
| 修改 | `PathSelector.cpp` | `#include <dsfw/FileDialogHelper.h>` → `<dsfw/widgets/FileDialogHelper.h>` |
| 修改 | `FilePathSelector.cpp` | 同上 |
| 修改 | `TaskWindow.cpp` | FileDialogHelper → FilePathSelector |
| 修改 | `DroppableFileListPanel.cpp` | FileDialogHelper → FilePathSelector |
| 修改 | `LogPanelWidget.cpp` | FileDialogHelper → FilePathSelector |

### 5.5 影响范围

| 文件 | 迁移内容 |
|------|---------|
| `TaskWindow.cpp` | 2 处 FileDialogHelper → FilePathSelector |
| `DroppableFileListPanel.cpp` | 2 处 FileDialogHelper → FilePathSelector |
| `LogPanelWidget.cpp` | 1 处 FileDialogHelper → FilePathSelector |
| `PathSelector.cpp` | 更新 include 路径 |
| `FilePathSelector.cpp` | 更新 include 路径 |

---

## 6. 阶段五：清理 `using namespace dsfw` 污染

### 6.1 问题分析

当前约 55 个文件存在 `using namespace dsfw;` 声明，污染命名空间。违反 INFRA-06（RAII 资源管理）的延伸要求（禁止全局级命名空间污染）。

除 `.cpp` 文件外，部分**头文件**也存在 `using namespace dsfw`，这会导致所有包含该头文件的翻译单元都被污染，危害更大。

### 6.2 方案

分 5 个子阶段执行，每子阶段独立编译验证：

| 子阶段 | 模块 | 文件数 | 说明 |
|--------|------|--------|------|
| 5a | 头文件 | ~5 | 优先清理头文件污染（影响范围最大） |
| 5b | `apps/shared/phoneme-editor` | 8 | 音素编辑器 |
| 5c | `apps/shared/chart-framework` | 8 | 图表框架 |
| 5d | `apps/shared/data-sources` | 12 | 数据源 |
| 5e | `apps/shared/audio-visualizer` + `bridges` | 4 | 音频可视化 + 桥接层 |
| 5f | `engine/adapters` + `domain/src/adapters` | 12 | 引擎适配器 + 领域适配器 |
| 5g | `ui-core` + `tests` + 其他 | 16 | UI 核心 + 测试 + 其余 |

每文件逐行将 `Result`、`PathUtils`、`Log` 等符号显式加上 `dsfw::` 前缀。

### 6.3 影响范围

- 约 55 个文件，纯机械重构，无行为变更
- 头文件优先，防止污染扩散

---

## 7. 阶段六：解除接口对 Qt 信号的依赖

### 7.1 问题分析

ARCH-02 明确规定："被动数据接口+容器通知，接口不加 QObject"。当前 2 个接口违反此规则：

| 接口 | 文件 | Qt 信号 | 使用者 |
|------|------|---------|--------|
| `IAudioPlayerAdapter` | `framework/audio/playback/include/dsfw/audio/IAudioPlayerAdapter.h` | `stateChanged`, `deviceChanged` | `PlayWidget` |
| `ISliceDataSource` | `framework/core/include/dsfw/ISliceDataSource.h` | `sliceListChanged`, `sliceSaved`, `audioAvailabilityChanged` | `SliceListModel` 等 |

`IEditorDataSource`（`src/domain/include/dstools/IEditorDataSource.h`）继承自 `ISliceDataSource`，同样受影响。

### 7.2 设计要点

- **仅移除 Q_OBJECT**，保留 Qt 类型（`QString`、`QStringList`）。ARCH-02 禁止的是 QObject 信号机制，不是 Qt 类型。框架层已依赖 Qt，Qt 类型在接口中完全合法。
- 底层 `AudioPlaybackAdapter` 已使用 `std::function` 回调，`AudioPlayerAdapter` 中 `emit` 语句仅需改为调用注册的回调。
- `PlayWidget` 回调通过 `QMetaObject::invokeMethod` 安全投递到 UI 线程。

### 7.3 IAudioPlayerAdapter 重构

**新接口**（纯虚类，无 QObject，保留 Qt 类型）：

```cpp
namespace dsfw::audio {

class IAudioPlayerAdapter {
public:
    using StateCallback = std::function<void(State newState)>;
    using DeviceCallback = std::function<void(const QString &device)>;

    static constexpr int kInterfaceVersion = 2;

    enum class State { Stopped, Playing };

    virtual ~IAudioPlayerAdapter() = default;

    virtual dsfw::Result<void> open(const std::filesystem::path &path) = 0;
    virtual void close() = 0;
    [[nodiscard]] virtual bool isOpen() const = 0;
    [[nodiscard]] virtual double duration() const = 0;
    [[nodiscard]] virtual double position() const = 0;
    virtual void setPosition(double sec) = 0;
    virtual void play() = 0;
    virtual void stop() = 0;
    [[nodiscard]] virtual bool isPlaying() const = 0;
    [[nodiscard]] virtual QStringList devices() const = 0;
    [[nodiscard]] virtual QString currentDevice() const = 0;
    virtual void setDevice(const QString &device) = 0;
    virtual bool setup(int sampleRate, int channels, int bufferSize) = 0;

    virtual void setStateCallback(StateCallback cb) = 0;
    virtual void setDeviceCallback(DeviceCallback cb) = 0;
};

} // namespace dsfw::audio
```

**AudioPlayerAdapter 适配**：现有 `emit stateChanged(...)` 和 `emit deviceChanged(...)` 改为调用注册的回调：

```cpp
// AudioPlayerAdapter.cpp 构造函数中
d->playback->setStateCallback([this](bool playing) {
    if (m_stateCallback) {
        m_stateCallback(playing ? State::Playing : State::Stopped);
    }
});

d->playback->setDeviceCallback([this](const std::string &device) {
    if (m_deviceCallback) {
        m_deviceCallback(QString::fromStdString(device));
    }
});
```

**PlayWidget 适配**：将 `connect(m_player, &IAudioPlayerAdapter::stateChanged, ...)` 改为 `m_player->setStateCallback([this](State s) { QMetaObject::invokeMethod(this, [this, s] { handleStateChanged(s); }, Qt::QueuedConnection); })`。

### 7.4 ISliceDataSource / IEditorDataSource 重构

**新接口**（纯虚类，无 QObject，保留 Qt 类型）：

```cpp
namespace dsfw {

class ISliceDataSource {
public:
    using SliceListCallback = std::function<void()>;
    using SliceSavedCallback = std::function<void(const QString &sliceId)>;
    using AudioAvailabilityCallback = std::function<void()>;

    static constexpr int kInterfaceVersion = 2;

    virtual ~ISliceDataSource() = default;

    [[nodiscard]] virtual int getSliceCount() const = 0;
    [[nodiscard]] virtual QStringList sliceIds() const = 0;
    [[nodiscard]] virtual QString audioPath(const QString &sliceId) const = 0;
    // ... 其他纯虚方法不变 ...

    virtual void setSliceListCallback(SliceListCallback cb) = 0;
    virtual void setSliceSavedCallback(SliceSavedCallback cb) = 0;
    virtual void setAudioAvailabilityCallback(AudioAvailabilityCallback cb) = 0;
};

} // namespace dsfw
```

**派生类适配**：将 `emit` 改为调用注册的回调（如 `if (m_sliceListCallback) m_sliceListCallback();`）。

| 派生类 | 文件 | emit 语句 |
|--------|------|-----------|
| `FileDataSource` | `apps/shared/data-sources/FileDataSource.cpp` | 2 处 `sliceListChanged`, 3 处 `sliceSaved` |
| `DirectoryDataSource` | `apps/shared/data-sources/DirectoryDataSource.cpp` | 1 处 `sliceListChanged`, 1 处 `sliceSaved` |
| `ProjectDataSource` | `apps/ds-labeler/core/ProjectDataSource.cpp` | 2 处 `sliceListChanged`, 1 处 `sliceSaved`, 1 处 `audioAvailabilityChanged` |

**SliceListModel 适配**：将 `connect(m_source, &IEditorDataSource::sliceListChanged, ...)` 改为 `m_source->setSliceListCallback([this]() { ... })`。

### 7.5 影响范围

| 文件 | 变更 |
|------|------|
| `IAudioPlayerAdapter.h` | 移除 Q_OBJECT，添加 `std::function` 回调注册方法 |
| `AudioPlayerAdapter.h` | 继承纯虚接口，实现回调注册，内部存储 `StateCallback`/`DeviceCallback` 成员 |
| `AudioPlayerAdapter.cpp` | emit → 回调调用 |
| `PlayWidget.cpp` | connect → setCallback + `QMetaObject::invokeMethod` |
| `ISliceDataSource.h` | 移除 Q_OBJECT，添加 `std::function` 回调注册方法 |
| `IEditorDataSource.h` | 移除 Q_OBJECT（仅继承声明，无额外信号） |
| `FileDataSource.cpp` | emit → 回调调用 |
| `DirectoryDataSource.cpp` | emit → 回调调用 |
| `ProjectDataSource.cpp` | emit → 回调调用 |
| `SliceListModel.cpp` | connect → setCallback |

---

## 8. 阶段七：图表渲染性能优化

### 8.1 问题分析

Phoneme 编辑器包含 4~5 个图表面板（波形图、功率图、频谱图、口型曲线、钢琴卷帘），
共享同一个 `ViewportController`。用户缩放或调整窗口大小时，渲染管线如下：

```
ViewportController::viewportChanged 信号
  → AudioVisualizerContainer::connectViewportToWidget
    → ChartPanelBase::onViewportUpdate(coord, pixelWidth)
      → m_cacheDirty = true; update()
        → paintEvent → rebuildCache(m_pendingRegion) → drawContent
```

**关键瓶颈**：

| 瓶颈 | 描述 | 严重度 |
|------|------|--------|
| 无更新合并 | 连续缩放（Ctrl+滚轮）时，每个滚轮事件在独立事件循环迭代中触发一次全量重建，所有图表同时阻塞 UI 线程 | 高 |
| 无视图状态变化检测 | `onViewportUpdate` 总是设置 `m_cacheDirty = true`，即使 `coord` 与上次完全相同 | 中 |
| 频谱图 QImage 重复创建 | `rebuildViewImage` 每次都在堆上分配新 `QImage`，即使尺寸未变 | 中 |
| 不可见图表仍重建缓存 | `onViewportUpdate` 不检查 `isVisible()`，隐藏的图表也执行 `rebuildCache` | 低 |

**数据流追踪**（以缩放为例）：

1. `EditorContainerBase::onZoomIn()` → `m_viewport->zoomIn(centerSec)` → `clampAndEmit()` → `emit viewportChanged(state)`
2. `AudioVisualizerContainer` 中 `connectViewportToWidget` 的 lambda 对每个已注册 chart 调用 `panel->onViewportUpdate(m_coordConverter, ...)`
3. `ChartPanelBase::onViewportUpdate` 设置 `m_converter = &conv; m_dataPixelWidth = pixelWidth; m_cacheDirty = true; update();`
4. Qt 事件循环处理 `paintEvent`（注：同一事件循环迭代内多次 `update()` 会被 Qt 合并，但不同滚轮事件在不同迭代中，无法合并）
5. `paintEvent` → `rebuildCache(m_pendingRegion)` → 子类实现（`rebuildWaveformCache` / `rebuildViewImage` / `rebuildPowerCache`）
6. `drawContent` 使用缓存数据绘制

注意：`m_pendingRegion` 始终为默认值 `{fullRebuild=true}`，即每次都是全量重建。
`RegionUpdate` 的增量更新（`shiftCache` + 部分重算）仅在边界拖拽等场景使用，缩放场景从未触发。

### 8.2 优化方案（保证逻辑等价）

#### OPT-1：视口更新合并（ChartPanelBase）

**原理**：连续缩放时，多个 `onViewportUpdate` 调用在不同事件循环迭代中到达。
使用 16ms 单次定时器将多次更新合并为一次，减少中间状态的无效重建。

**关键设计决策**：保留 `paintEvent` 中的 `rebuildCache` 逻辑不变。
`resizeEvent` 也通过设置 `m_cacheDirty = true` 并依赖 `paintEvent` 来触发重建，
因此不能移除 `paintEvent` 中的 `rebuildCache`。定时器仅用于延迟 `update()` 调用，
`paintEvent` 中已有的 `rebuildCache` 逻辑作为统一入口。

**改动**：

```cpp
// ChartPanelBase.h 新增成员
private:
    QTimer *m_updateTimer = nullptr;  // 16ms single-shot coalescing timer

// ChartPanelBase.cpp 构造函数末尾
m_updateTimer = new QTimer(this);
m_updateTimer->setSingleShot(true);
connect(m_updateTimer, &QTimer::timeout, this, [this]() {
    update();  // 触发 paintEvent，paintEvent 中检查 m_cacheDirty 并调用 rebuildCache
});

// ChartPanelBase::onViewportUpdate 改为
void ChartPanelBase::onViewportUpdate(const ChartCoordinate &conv, int pixelWidth) {
    // OPT-2 的跳过逻辑在此处插入（见下文）
    m_converter = &conv;
    m_dataPixelWidth = pixelWidth;
    m_cacheDirty = true;
    if (!m_updateTimer->isActive()) {
        m_updateTimer->start(16);  // ~60fps, 合并同一时间段内的多次调用
    }
}
```

**注意**：`paintEvent` 中的 `rebuildCache` 逻辑完全不变。定时器仅负责延迟 `update()` 调用，
避免每个滚轮事件都立即触发 `paintEvent`。`resizeEvent` 通过 `QWidget::resizeEvent` 间接触发
`paintEvent`，仍然走 `paintEvent → rebuildCache` 路径。

**影响**：4 个图表面板 + 钢琴卷帘均受益。代码增加约 10 行。

#### OPT-2：跳过冗余重建（ChartPanelBase）

**原理**：当 `onViewportUpdate` 收到的 `coord` 与上次已应用的 `coord` 完全相同时，
跳过缓存重建。这在以下场景触发：
- 图表可见性切换时，`rebuildChartLayout` 可能触发 `viewportChanged`
- 同一 viewport 状态被重复 emit（如 `syncStateFields` 后 `clampAndEmit`）

**改动**：

```cpp
// ChartPanelBase.h 新增成员
private:
    ChartCoordinate m_lastAppliedCoord{};
    bool m_lastAppliedValid = false;

// onViewportUpdate 开头添加（在 OPT-1 设置字段之前）
void ChartPanelBase::onViewportUpdate(const ChartCoordinate &conv, int pixelWidth) {
    if (m_lastAppliedValid &&
        std::abs(m_lastAppliedCoord.viewStart - conv.viewStart) < 1e-9 &&
        std::abs(m_lastAppliedCoord.viewEnd - conv.viewEnd) < 1e-9 &&
        m_lastAppliedCoord.pixelWidth == conv.pixelWidth &&
        m_dataPixelWidth == pixelWidth) {
        return;  // 与上次已应用的坐标相同，跳过
    }
    // ... 原有逻辑（OPT-1 的字段设置 + 定时器启动）
}

// paintEvent 中，rebuildCache 之后记录
// 在 m_cacheDirty = false; 之后添加：
if (m_converter) {
    m_lastAppliedCoord = *m_converter;
    m_lastAppliedCoord.pixelWidth = m_dataPixelWidth;
    m_lastAppliedValid = true;
}
```

**影响**：4 个图表面板 + 钢琴卷帘均受益。代码增加约 12 行。

#### OPT-3：频谱图 QImage 缓冲区复用（SpectrogramChartPanel）

**原理**：`rebuildViewImage` 每次调用都创建新 `QImage(dataW, h, Format_RGB32)`，
旧 `QImage` 被析构释放。当窗口尺寸未变时，可以复用现有缓冲区。

**改动**：

```cpp
// SpectrogramChartPanel::rebuildViewImage 中
// 原来：
m_viewImage = QImage(dataW, h, QImage::Format_RGB32);
m_viewImage.fill(qRgb(0, 0, 0));

// 改为：
if (m_viewImage.isNull() || m_viewImage.width() != dataW || m_viewImage.height() != h) {
    m_viewImage = QImage(dataW, h, QImage::Format_RGB32);
}
m_viewImage.fill(qRgb(0, 0, 0));  // 始终清零，fillImageColumns 只覆盖可见范围
```

**影响**：仅频谱图面板受益。代码增加约 3 行。缩放时（尺寸不变）避免重复内存分配。

#### OPT-4：不可见图表跳过更新（connectViewportToWidget）

**原理**：`connectViewportToWidget` 无条件调用 `onViewportUpdate`，即使图表被隐藏。
隐藏图表的 `rebuildCache` 是无用功。

**改动**：

```cpp
// AudioVisualizerContainer::connectViewportToWidget 中
connect(m_viewport, &ViewportController::viewportChanged, this, [safeWidget, this](const ViewportState& state) {
    if (!safeWidget || !safeWidget->isVisible())  // 新增 isVisible 检查
        return;
    if (auto* panel = qobject_cast<dstools::ChartPanelBase*>(safeWidget.data())) {
        panel->onViewportUpdate(m_coordConverter, safeWidget->width());
    }
});
```

**注意**：图表从隐藏变为可见时，需在 `rebuildChartLayout` 或 `setChartVisible` 中
主动触发一次 `onViewportUpdate` 以确保缓存刷新。需验证现有代码中可见性切换后是否已有
`onViewportUpdate` 调用——通常 `rebuildChartLayout` 会触发 `viewportChanged` 信号，
间接调用 `onViewportUpdate`，但被 `isVisible()` 拦截。需在 `setChartVisible(true)` 后
显式调用 `panel->onViewportUpdate(m_coordConverter, panel->width())`。

**影响**：所有图表面板受益。当用户隐藏频谱图或钢琴卷帘时，缩放不再触发其重建。
代码增加约 3 行。

### 8.3 初次渲染分析

初次渲染（加载音频后）的性能瓶颈主要在频谱图 FFT 计算。当前 `prepareSpectrogramParams`
在 `setAudioData` → `onAudioDataChanged` 时调用，仅计算全局参数（`m_totalFrames`, `m_numFreqBins`），
尚未执行 FFT。实际 FFT 在首次 `rebuildViewImage` → `ensureSpectrogramRange` → `computeSpectrogramRange` 时
按需计算。

**现状已合理**：FFT 已按需懒加载（仅计算可见帧 + 2 帧余量），无需额外优化。
将 FFT 移到后台线程会显著增加复杂度（需处理 `QImage` 线程安全、数据同步），
不符合"不大幅增加代码复杂度"的约束。

### 8.4 改动范围

| 文件 | 变更内容 | 优化项 |
|------|---------|--------|
| `chart-framework/ChartPanelBase.h` | 新增 `m_updateTimer`, `m_lastAppliedCoord`, `m_lastAppliedValid` | OPT-1, OPT-2 |
| `chart-framework/ChartPanelBase.cpp` | 修改构造函数、`onViewportUpdate`、`paintEvent` | OPT-1, OPT-2 |
| `chart-framework/SpectrogramChartPanel.cpp` | 修改 `rebuildViewImage` 中的 QImage 创建逻辑 | OPT-3 |
| `audio-visualizer/AudioVisualizerContainer.cpp` | 修改 `connectViewportToWidget` 添加 `isVisible` 检查 | OPT-4 |

### 8.5 性能收益预估

| 场景 | 优化前 | 优化后 | 原理 |
|------|--------|--------|------|
| 连续缩放 3 步 | 3 次全量重建 | 1 次全量重建 | OPT-1 合并 |
| 频谱图缩放（尺寸不变） | 每次分配 QImage 堆内存 | 复用已有缓冲区 | OPT-3 |
| 隐藏频谱图后缩放 | 频谱图仍执行 FFT 重建 | 跳过 | OPT-4 |
| 重复 viewportChanged | 重复重建缓存 | 跳过 | OPT-2 |

**逻辑等价性保证**：所有优化不改变任何渲染结果。`m_cacheDirty` → `rebuildCache` → `drawContent` 的
逻辑流程完全保持，仅调整触发时机和频率。

---

## 9. 实施计划

| 阶段 | 内容 | 风险 | 文件数 | 验收要点 |
|------|------|------|--------|---------|
| 一 | 补充工程准则 SUP-01~SUP-10 | 低 | 1 | human-decisions.md 新增 10 条准则 |
| 二 | 清理死代码 | 低 | 3 | .gitkeep 删除、cmake 变量清理 |
| 三 | 修复循环依赖 | 低 | 7 | dsfw-widgets 不再依赖 dsfw-ui-core |
| 四 | 统一文件选择 | 低 | 6 | 所有场景通过 FilePathSelector/PathSelector |
| 五 | 清理 using namespace | 低 | 55+ | 所有文件无 `using namespace dsfw` |
| 六 | 解除 Qt 信号依赖 | 中 | 11 | 接口无 Q_OBJECT，回调正确投递 UI 线程 |
| 七 | 图表渲染优化 | 低 | 4 | 连续缩放仅触发一次重建，不可见图表跳过 |

每个阶段独立编译验证后提交，不推送。

---

## 10. 验收标准

- [ ] 编译通过（`cmake --build --preset release`），无新增警告
- [ ] 所有现有功能行为不变（手动回归测试）
- [ ] 阶段一：`human-decisions.md` 新增 SUP-01~SUP-10 准则
- [ ] 阶段二：`.gitkeep` 删除、`AUDIO_UTIL_BUILD_TESTS` 变量清理
- [ ] 阶段三：`dsfw-widgets` 不再依赖 `dsfw-ui-core`，循环依赖消除
- [ ] 阶段四：所有文件选择场景均通过 `FilePathSelector` 或 `PathSelector`，`RecentPathStore` 行为一致
- [ ] 阶段四：`FileDialogHelper` 移至 `dsfw/widgets/` 命名子目录下
- [ ] 阶段五：所有文件无 `using namespace dsfw;` 声明（含头文件）
- [ ] 阶段六：`IAudioPlayerAdapter`、`ISliceDataSource`、`IEditorDataSource` 不再继承 QObject
- [ ] 阶段六：`PlayWidget` 回调正确投递到 UI 线程，无跨线程 UI 操作
- [ ] 阶段七：连续缩放仅触发一次缓存重建，不可见图表不参与重建
- [ ] 阶段七：频谱图缩放时 QImage 缓冲区复用，无可观察的渲染差异
- [ ] 阶段七：图表从隐藏变为可见时缓存正确刷新
- [ ] `clang-format` 格式检查通过
- [ ] 设计文档引用与实际代码一致