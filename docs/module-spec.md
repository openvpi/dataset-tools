# 02 — 模块接口规范

**前置**: architecture.md  
**变更**: v3.0 删除 PipelineRunner（不再一键串联）；明确 SlicerPage 不继承 TaskWindow；补充 AudioSlicer 特有的信号签名差异。

本文档定义重构后每个模块的公共接口、方法签名、信号槽、数据结构和使用示例。确保实施时每个模块可独立开发并正确集成。

---

## 1. Core Layer

### 1.1 AppInit

```cpp
// src/core/include/dstools/AppInit.h
#pragma once
#include <QApplication>

namespace dstools {

/// 统一应用初始化。在 main() 中 QApplication 构造后立即调用。
/// 完成：字体设置、root 权限检查、崩溃处理器、cpp-pinyin 字典加载。
struct AppInit {
    /// @param app          QApplication 实例
    /// @param initPinyin   是否加载 cpp-pinyin 字典（MinLabel/LyricFA 需要）
    /// @param initCrashHandler 是否初始化 QBreakpad（仅 Release 构建生效）
    static void init(QApplication &app,
                     bool initPinyin = false,
                     bool initCrashHandler = false);
};

} // namespace dstools
```

**实现要点**：

```cpp
// AppInit.cpp
void AppInit::init(QApplication &app, bool initPinyin, bool initCrashHandler) {
    // 1. Root 权限检查 — 原 MinLabel/SlurCutter main.cpp 中的逻辑
#ifdef Q_OS_WIN
    // IsUserAnAdmin() 检查
#else
    if (getuid() == 0) { QMessageBox::critical(...); std::exit(1); }
#endif

    // 2. 字体设置 — 原各 main.cpp 中的 Windows 字体逻辑
#ifdef Q_OS_WIN
    QFont font("Microsoft YaHei", 9);
    font.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(font);
#endif

    // 3. 崩溃处理（可选）
    if (initCrashHandler) {
#ifdef HAS_QBREAKPAD
        static QBreakpadHandler handler(app.applicationDirPath() + "/crash");
#endif
    }

    // 4. cpp-pinyin 字典（可选）
    if (initPinyin) {
        Pinyin::setDictionaryPath(app.applicationDirPath() + "/dict");
    }
}
```

### 1.2 AppSettings（原 Config，已重新设计）

> **设计偏离**：原始设计为 `dstools::Config`（QSettings/INI 封装），实际实现已演进为
> `dstools::AppSettings`（类型安全、响应式、JSON 后端）。详见 `status.md` 偏离说明。

```cpp
// src/core/include/dstools/AppSettings.h
#pragma once
#include <QObject>
#include <QString>
#include <QKeySequence>
#include <nlohmann/json.hpp>
#include <functional>
#include <mutex>

namespace dstools {

/// 类型化配置 key，带编译期默认值。
/// path 使用 '/' 分隔映射到嵌套 JSON 对象。
template <typename T>
struct SettingsKey {
    const char *path;
    T defaultValue;
    SettingsKey(const char *path, T defaultValue);
};

/// 持久化、响应式配置存储，基于单个 JSON 文件。
/// 线程安全，即时原子写入 (QSaveFile)。
class AppSettings : public QObject {
    Q_OBJECT
public:
    explicit AppSettings(const QString &appName, QObject *parent = nullptr);

    // 类型安全读写
    template <typename T> T get(const SettingsKey<T> &key) const;
    template <typename T> void set(const SettingsKey<T> &key, const T &value);

    // 响应式观察（可选 QObject* context 自动断连）
    template <typename T>
    int observe(const SettingsKey<T> &key, std::function<void(const T &)> cb,
                QObject *context = nullptr);
    void removeObserver(int id);

    // 快捷键便利方法
    QKeySequence shortcut(const SettingsKey<QString> &key) const;
    void setShortcut(const SettingsKey<QString> &key, const QKeySequence &seq);

    // 杂项
    template <typename T> bool contains(const SettingsKey<T> &key) const;
    template <typename T> void remove(const SettingsKey<T> &key);
    void reload();
    void flush();
    QString filePath() const;

signals:
    void keyChanged(const QString &keyPath);
};

} // namespace dstools
```

**Schema 定义示例**（每个 app 一个头文件）：

```cpp
// src/apps/MinLabel/MinLabelKeys.h
#pragma once
#include <dstools/AppSettings.h>

namespace MinLabelKeys {
    inline const dstools::SettingsKey<QString> LastDir("General/lastDir", "");
    inline const dstools::SettingsKey<QString> ShortcutOpen("Shortcuts/open", "Ctrl+O");
    inline const dstools::SettingsKey<QString> ShortcutExport("Shortcuts/export", "Ctrl+E");
    inline const dstools::SettingsKey<QString> NavigationPrev("Shortcuts/prevFile", "PgUp");
    inline const dstools::SettingsKey<QString> NavigationNext("Shortcuts/nextFile", "PgDown");
    inline const dstools::SettingsKey<QString> PlaybackPlay("Shortcuts/play", "F5");
}
```

**与原代码的映射**：

| 原代码 | 新代码 |
|--------|--------|
| `QSettings settings(appDir + "/config/MinLabel.ini", ...)` | `AppSettings settings("MinLabel");` |
| `settings.value("Shortcuts/Open", "Ctrl+O").toString()` | `settings.shortcut(MinLabelKeys::ShortcutOpen)` |
| `settings.setValue("General/LastDir", dir)` | `settings.set(MinLabelKeys::LastDir, dir)` |

每个 EXE 使用独立的 JSON 文件：

```
config/
├── DatasetPipeline.json
├── MinLabel.json
├── SlurCutter.json
└── GameInfer.json
```

```cpp
// 每个 EXE 使用自己的 AppSettings 实例
dstools::AppSettings settings("MinLabel");    // → <appDir>/config/MinLabel.json
dstools::AppSettings settings("SlurCutter");  // → <appDir>/config/SlurCutter.json
```

### 1.3 ErrorHandling

```cpp
// src/core/include/dstools/ErrorHandling.h
#pragma once
#include <QString>
#include <optional>
#include <utility>

namespace dstools {

/// 轻量结果类型，用于可能失败的操作。
template<typename T>
struct Result {
    std::optional<T> value;
    QString error;

    [[nodiscard]] bool ok() const { return value.has_value(); }
    [[nodiscard]] const T &get() const { return value.value(); }
    [[nodiscard]] T &&take() { return std::move(value.value()); }

    static Result success(T val) { return {std::move(val), {}}; }
    static Result fail(const QString &msg) { return {std::nullopt, msg}; }
};

/// 无返回值版本
struct Status {
    QString error;
    [[nodiscard]] bool ok() const { return error.isEmpty(); }
    static Status success() { return {}; }
    static Status fail(const QString &msg) { return {msg}; }
};

} // namespace dstools
```

**使用约定**：

```cpp
// 文件 I/O — 返回 Result 或 bool
dstools::Result<QJsonObject> readJsonFile(const QString &path);
bool saveFile(const QString &path);  // 失败弹 QMessageBox，不 exit

// 工作线程 — 通过信号传递错误，永不直接弹对话框
signals:
    void taskFailed(const QString &filename, const QString &errorMsg);

// ONNX session 创建 — 返回 nullptr + 错误消息
std::unique_ptr<Ort::Session> create(..., QString &errorMsg);
```

### 1.4 ThreadUtils

```cpp
// src/core/include/dstools/ThreadUtils.h
#pragma once
#include <QMetaObject>
#include <QCoreApplication>
#include <functional>

namespace dstools {

/// 在主线程执行回调（从工作线程安全地操作 UI）。
/// 替代工作线程中直接调用 QMessageBox。
inline void invokeOnMain(std::function<void()> fn) {
    QMetaObject::invokeMethod(
        qApp, std::move(fn), Qt::QueuedConnection);
}

} // namespace dstools
```

### 1.5 Theme

```cpp
// src/core/include/dstools/Theme.h
#pragma once
#include <QApplication>
#include <QColor>

namespace dstools {

class Theme {
public:
    enum Mode { Light, Dark, System };

    /// 应用主题到整个应用程序
    static void apply(QApplication &app, Mode mode);

    /// 获取当前主题模式
    static Mode currentMode();

    /// 获取主题色板（供自绘组件使用，如 F0Widget）
    struct Palette {
        QColor bgPrimary;       // 主背景
        QColor bgSecondary;     // 次背景（面板/列表）
        QColor bgSurface;       // 卡片/浮层
        QColor accent;          // 强调色
        QColor textPrimary;     // 主文字
        QColor textSecondary;   // 次文字
        QColor border;          // 边框
        QColor success;         // 成功
        QColor error;           // 错误
        QColor warning;         // 警告
    };

    static const Palette &palette();

private:
    static Mode s_mode;
    static Palette s_palette;
};

} // namespace dstools
```

### 1.6 JsonHelper

> **设计偏离**：原始设计中未包含此模块。实际实现中发现需要安全的 JSON 文件 I/O 和值访问封装，
> 故新增 `JsonHelper` 作为 dstools-core 的一部分。

```cpp
// src/core/include/dstools/JsonHelper.h
#pragma once
#include <nlohmann/json.hpp>
#include <filesystem>
#include <string>
#include <map>
#include <vector>

namespace dstools {

/// 安全的 JSON 文件 I/O 和值访问封装。
/// 所有方法优雅处理错误——不抛未捕获异常，不崩溃。
class JsonHelper {
public:
    // ── 文件 I/O ──

    /// 加载并解析 JSON 文件。失败时返回空对象。
    static nlohmann::json loadFile(const std::filesystem::path &path, std::string &error);

    /// 原子写入 JSON 文件（写临时文件 + 重命名）。
    static bool saveFile(const std::filesystem::path &path, const nlohmann::json &data,
                         std::string &error, int indent = 4);

    // ── 安全值访问 ──

    /// 按 "/" 分隔路径获取值，类型不匹配时返回默认值。不抛异常。
    template <typename T>
    static T get(const nlohmann::json &root, const char *path, const T &defaultValue);

    /// 获取嵌套对象/数组。不存在时返回空对象/空数组。
    static nlohmann::json getObject(const nlohmann::json &root, const char *path);
    static nlohmann::json getArray(const nlohmann::json &root, const char *path);

    /// 路径存在性检查。
    static bool contains(const nlohmann::json &root, const char *path);

    // ── 必需值访问（报告错误而非静默默认值） ──

    template <typename T>
    static bool getRequired(const nlohmann::json &root, const char *path, T &out, std::string &error);

    // ── 集合辅助 ──

    template <typename K, typename V>
    static std::map<K, V> getMap(const nlohmann::json &root, const char *path);
    template <typename T>
    static std::vector<T> getVec(const nlohmann::json &root, const char *path);

    // ── 路径工具 ──

    /// 按 "/" 分隔路径导航嵌套 JSON。路径不存在时返回 nullptr。
    static const nlohmann::json *resolve(const nlohmann::json &root, const char *path);

private:
    JsonHelper() = default; // 纯静态类
};

} // namespace dstools
```

**使用场景**：

| 使用者 | 用法 |
|--------|------|
| AppSettings | 配置文件的 JSON 加载/保存 |
| hubert-infer (规划中) | 模型 config.json 解析 |
| 各推理库 | 模型配置读取 |

**与 ErrorHandling 的关系**：

JsonHelper 通过 `std::string &error` 输出参数报告错误，是一种轻量级错误处理方式。待 AD-03（ErrorHandling 模块）完成后，可考虑将错误报告迁移到 `Result<T>` / `Status` 类型。

---

## 2. Audio Layer

### 2.1 WaveFormat

```cpp
// src/audio/include/dstools/WaveFormat.h
#pragma once
#include <cstdint>

namespace dstools::audio {

/// 音频格式描述。保留原 qsmedia 的 WaveFormat 语义。
class WaveFormat {
public:
    WaveFormat();
    WaveFormat(int sampleRate, int bitsPerSample, int channels);

    int sampleRate() const;
    int bitsPerSample() const;
    int channels() const;
    int blockAlign() const;      // channels * bitsPerSample / 8
    int averageBytesPerSecond() const;

private:
    int m_sampleRate = 44100;
    int m_bitsPerSample = 16;
    int m_channels = 2;
};

} // namespace dstools::audio
```

### 2.2 AudioDecoder

```cpp
// src/audio/include/dstools/AudioDecoder.h
#pragma once
#include <dstools/WaveFormat.h>
#include <QString>
#include <memory>

namespace dstools::audio {

/// 音频解码器。内部使用 FFmpeg。
/// 替代原 qsmedia::IAudioDecoder + FFmpegDecoder 插件。
///
/// 保留的原始接口语义：
///   - open(path) 打开文件
///   - Read(buffer, offset, count) 读取 PCM 数据
///   - SetPosition(pos) 设置播放位置
///   - close() 关闭文件
///   - length() 返回总采样数（字节）
class AudioDecoder {
public:
    AudioDecoder();
    ~AudioDecoder();

    // 不可拷贝
    AudioDecoder(const AudioDecoder &) = delete;
    AudioDecoder &operator=(const AudioDecoder &) = delete;

    /// 打开音频文件（支持 wav/mp3/m4a/flac）
    bool open(const QString &path);

    /// 关闭当前文件
    void close();

    /// 是否已打开文件
    bool isOpen() const;

    /// 获取音频格式
    WaveFormat format() const;

    /// 读取 PCM 数据到 buffer+offset，最多 count 字节。
    /// 返回实际读取的字节数，0 表示 EOF。
    /// 保留原 IAudioDecoder::Read 签名语义。
    int read(char *buffer, int offset, int count);

    /// 设置播放位置（字节偏移）
    void setPosition(qint64 pos);

    /// 获取当前位置（字节偏移）
    qint64 position() const;

    /// 获取总长度（字节）
    qint64 length() const;

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace dstools::audio
```

### 2.3 AudioPlayback

```cpp
// src/audio/include/dstools/AudioPlayback.h
#pragma once
#include <QObject>
#include <QStringList>
#include <memory>

namespace dstools::audio {

class AudioDecoder;

/// 音频播放器。内部使用 SDL2。
/// 替代原 qsmedia::IAudioPlayback + SDLPlayback 插件。
///
/// 线程模型（与原实现一致）：
///   - 主线程：所有公共方法调用
///   - SDL poll 线程：处理 SDL 事件（设备增删）
///   - SDL audio callback 线程：从 decoder 读取 PCM 数据
///
/// 修复点（vs 原实现）：
///   - std::atomic<int> 替代 volatile int state（BUG-018）
///   - std::condition_variable 替代忙等待（BUG-018）
///   - std::lock_guard 替代手动 lock/unlock（BUG-019）
///   - 空设备列表守卫（BUG-020）
///   - pcm_buffer 分配修复（CQ-011）
class AudioPlayback : public QObject {
    Q_OBJECT
public:
    explicit AudioPlayback(QObject *parent = nullptr);
    ~AudioPlayback() override;

    // 不可拷贝
    AudioPlayback(const AudioPlayback &) = delete;
    AudioPlayback &operator=(const AudioPlayback &) = delete;

    /// 初始化音频子系统。
    /// @return false 表示初始化失败（此时所有播放方法为空操作）。
    bool setup(int sampleRate = 44100, int channels = 2, int bufferSize = 1024);

    /// 释放音频子系统。
    void dispose();

    /// 设置解码器源
    void setDecoder(AudioDecoder *decoder);

    /// 播放控制
    void play();
    void stop();

    /// 设备管理
    QStringList devices() const;
    QString currentDevice() const;
    void setDevice(const QString &device);

    /// 状态
    enum State { Stopped, Playing, Paused };
    State state() const;

signals:
    void stateChanged(AudioPlayback::State newState);
    void deviceChanged(const QString &device);
    void deviceAdded(const QString &device);
    void deviceRemoved(const QString &device);

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace dstools::audio
```

---

## 3. Widgets Layer

> **DLL 导出宏**: `src/widgets/include/dstools/WidgetsGlobal.h` 定义了 `DSTOOLS_WIDGETS_API` 宏
> （编译 dstools-widgets 时为 `Q_DECL_EXPORT`，使用时为 `Q_DECL_IMPORT`）。
> 所有 widgets 层的公共类声明应使用此宏。

### 3.1 PlayWidget

```cpp
// src/widgets/include/dstools/PlayWidget.h
#pragma once
#include <QWidget>
#include <memory>

class QPushButton;
class QSlider;
class QComboBox;
class QLabel;
class QMenu;

namespace dstools::audio {
class AudioDecoder;
class AudioPlayback;
}

namespace dstools::widgets {

/// 统一音频播放控件。合并原 MinLabel::PlayWidget 和 SlurCutter::PlayWidget。
///
/// 保留的原始功能：
///   - 播放/暂停/停止按钮
///   - 进度滑块（点击定位 + 拖拽）
///   - 音频设备选择菜单
///   - 时间显示（当前/总时长）
///   - 可选范围播放（SlurCutter 使用）
///   - 可选播放头位置信号（SlurCutter 使用）
///
/// 合并策略：
///   MinLabel: 使用基本模式（openFile + play/stop）
///   SlurCutter: 使用范围模式（setPlayRange + playheadChanged）
class PlayWidget : public QWidget {
    Q_OBJECT
public:
    explicit PlayWidget(QWidget *parent = nullptr);
    ~PlayWidget() override;

    /// 打开音频文件
    void openFile(const QString &path);

    /// 关闭当前文件
    void closeFile();

    /// 是否正在播放
    bool isPlaying() const;

    /// 设置播放/暂停
    void setPlaying(bool playing);

    /// 设置播放范围（秒）。设置后只播放此范围内的音频。
    /// 原 SlurCutter::PlayWidget::setRange() 语义。
    void setPlayRange(double startSec, double endSec);

    /// 清除播放范围（恢复全文件播放）
    void clearPlayRange();

signals:
    /// 播放头位置变化（秒）。
    /// 原 SlurCutter::PlayWidget::playheadChanged 信号。
    /// 仅在设置了播放范围后有效（通过 timerEvent 发出，约 60fps）。
    void playheadChanged(double positionSec);

private:
    // UI 组件（与原 PlayWidget 布局一致）
    QPushButton *m_playBtn;
    QPushButton *m_stopBtn;
    QPushButton *m_devBtn;
    QSlider *m_slider;
    QLabel *m_timeLabel;
    QMenu *m_deviceMenu;

    // 音频后端
    dstools::audio::AudioDecoder *m_decoder = nullptr;
    dstools::audio::AudioPlayback *m_playback = nullptr;
    bool m_valid = false;  // 初始化是否成功（BUG-001 修复）

    // 范围播放
    double m_rangeStart = -1.0;
    double m_rangeEnd = -1.0;
    bool m_hasRange = false;

    // 播放头追踪（SlurCutter steady_clock 精度）
    std::chrono::steady_clock::time_point m_playStartTime;
    double m_playStartPos = 0.0;

    // 内部方法（原 PlayWidget 的 private slots）
    void initAudio();
    void uninitAudio();
    void reloadDevices();
    void onPlayClicked();
    void onStopClicked();
    void onSliderReleased();
    void onPlaybackStateChanged(dstools::audio::AudioPlayback::State state);
    void onDeviceAction(QAction *action);
    void onDeviceChanged(const QString &device);
    void onDeviceAdded(const QString &device);
    void onDeviceRemoved(const QString &device);

protected:
    void timerEvent(QTimerEvent *event) override;
};

} // namespace dstools::widgets
```

**原 connect 映射**：

| 原连接 | 新连接 | 备注 |
|--------|--------|------|
| `playButton::clicked → _q_playButtonClicked` | `m_playBtn::clicked → onPlayClicked` | 重命名 |
| `stopButton::clicked → _q_stopButtonClicked` | `m_stopBtn::clicked → onStopClicked` | 重命名 |
| `slider::sliderReleased → _q_sliderReleased` | `m_slider::sliderReleased → onSliderReleased` | 重命名 |
| `playback::stateChanged → _q_playStateChanged` | `m_playback::stateChanged → onPlaybackStateChanged` | 类型改为枚举 |
| `playback::deviceChanged → _q_audioDeviceChanged` | `m_playback::deviceChanged → onDeviceChanged` | 重命名 |
| `playback::deviceAdded → _q_audioDeviceAdded` | `m_playback::deviceAdded → onDeviceAdded` | 重命名 |
| `playback::deviceRemoved → _q_audioDeviceRemoved` | `m_playback::deviceRemoved → onDeviceRemoved` | 重命名 |
| `devButton::clicked → 显示 deviceMenu` | `m_devBtn::clicked → m_deviceMenu->popup()` | 不变 |

### 3.2 TaskWindow

```cpp
// src/widgets/include/dstools/TaskWindow.h
#pragma once
#include <QWidget>
#include <QStringList>
#include <QThreadPool>

class QListWidget;
class QPushButton;
class QProgressBar;
class QPlainTextEdit;

namespace dstools::widgets {

/// 批处理任务窗口基类。
/// 替代原 AsyncTask::AsyncTaskWindow（原继承 QMainWindow，改为 QWidget 以支持嵌入 Tab）。
/// 功能完全保留：文件列表、拖放添加、进度条、日志、运行/停止。
///
/// LyricFA 和 HubertFA 继承此类。
class TaskWindow : public QWidget {
    Q_OBJECT
public:
    explicit TaskWindow(QWidget *parent = nullptr);
    ~TaskWindow() override;

    /// 设置最大并发线程数（默认 1，与原行为一致）
    void setMaxThreadCount(int count);

    /// 获取任务列表中的文件路径
    QStringList taskList() const;

    /// 设置运行按钮文本
    void setRunButtonText(const QString &text);

    /// 获取线程池（子类用于提交任务）
    QThreadPool *threadPool() const;

public slots:
    /// 用户操作
    void addFiles();            // 原 slot_addFile
    void addFolder();           // 原 slot_addFolder
    void removeSelected();      // 原 slot_removeSelected
    void clearList();           // 原 slot_clearList

    /// 批处理回调（工作线程通过信号连接到这里）
    void onTaskFinished(const QString &filename, const QString &info);
    void onTaskFailed(const QString &filename, const QString &error);

signals:
    /// 所有任务完成时发出
    void allTasksFinished();

protected:
    /// 子类重写：初始化自定义 UI（在基类 UI 之上添加控件）
    virtual void initCustomUI() {}

    /// 子类重写：启动批处理任务（遍历 taskList 创建 QRunnable）
    virtual void runTasks() = 0;

    /// 子类重写：所有任务完成后的清理
    virtual void onAllTasksFinished() {}

    /// 添加自定义控件到顶部区域
    void addTopWidget(QWidget *widget);

    // UI 组件（子类可访问）
    QListWidget *m_taskList;
    QProgressBar *m_progressBar;
    QPlainTextEdit *m_logView;              // 原 m_logOutput（QPlainTextEdit），保留类型不变
    QPushButton *m_runBtn;

    // 状态
    int m_totalTasks = 0;
    int m_completedTasks = 0;               // 原 m_finishedTasks
    int m_failedTasks = 0;                  // 原 m_errorTasks
    QStringList m_errorDetails;             // 原有成员，错误摘要列表
    bool m_isRunning = false;               // 新增：运行状态标志

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    QThreadPool *m_threadPool;
    void updateProgress();
    void onRunClicked();
};

} // namespace dstools::widgets
```

**与原 AsyncTaskWindow 的接口映射**：

| 原方法 | 新方法 | 变化 |
|--------|--------|------|
| `slot_addFile()` | `addFiles()` | 重命名，去 slot_ 前缀 |
| `slot_addFolder()` | `addFolder()` | 同上 |
| `slot_removeSelected()` | `removeSelected()` | 同上 |
| `slot_clearList()` | `clearList()` | 同上 |
| `slot_oneFinished(QString, QString)` | `onTaskFinished(...)` | 重命名 |
| `slot_oneFailed(QString, QString)` | `onTaskFailed(...)` | 重命名 |
| `virtual init()` | `virtual initCustomUI()` | 更清晰 |
| `virtual runTask()` | `virtual runTasks()` | 名词形式 |
| `virtual onTaskFinished()` | `virtual onAllTasksFinished()` | 区分单个/全部 |
| `QMainWindow` (基类) | `QWidget` (基类) | 改为 QWidget 以支持嵌入 Tab |
| `QPlainTextEdit *m_logOutput` | `QPlainTextEdit *m_logView` | 仅重命名，保留 QPlainTextEdit 类型 |
| `int m_finishedTasks` | `int m_completedTasks` | 重命名 |
| `int m_errorTasks` | `int m_failedTasks` | 重命名 |
| `QStringList m_errorDetails` | `QStringList m_errorDetails` | 保留 |
| （无） | `bool m_isRunning` | 新增运行状态标志 |
| （无） | `signal allTasksFinished()` | 新增信号 |

### 3.3 GpuSelector

```cpp
// src/widgets/include/dstools/GpuSelector.h
#pragma once
#include <QComboBox>
#include <QVector>
#include <QString>

namespace dstools::widgets {

struct GpuInfo {
    int index;
    QString name;
    size_t dedicatedMemory;
};

/// GPU 选择下拉框。自动枚举 DirectML (DXGI) GPU 设备。
/// 替代原 GameInfer/DmlGpuUtils 和 HubertFA/DmlGpuUtils。
class GpuSelector : public QComboBox {
    Q_OBJECT
public:
    explicit GpuSelector(QWidget *parent = nullptr);

    /// 获取选中的设备 ID（用于 OrtSessionOptionsAppendExecutionProvider_DML）
    int selectedDeviceId() const;

    /// 获取所有可用 GPU 信息
    QVector<GpuInfo> availableGpus() const;

    /// 刷新设备列表
    void refresh();

private:
    QVector<GpuInfo> m_gpus;
    void enumerateGpus();
};

} // namespace dstools::widgets
```

---

## 4. Inference Layer

### 4.1 OnnxEnv

```cpp
// src/infer/common/include/dstools/OnnxEnv.h
#pragma once
#include <onnxruntime_cxx_api.h>
#include <dstools/ExecutionProvider.h>
#include <string>
#include <memory>

namespace dstools::infer {

/// 全局 ONNX Runtime 环境（单例）。
/// 替代各推理库中重复的 Ort::Env 创建。
class OnnxEnv {
public:
    /// 获取全局 Ort::Env（进程唯一）
    static Ort::Env &env();

    /// 创建配置好的 SessionOptions
    /// 原各模型中重复的 SetInterOpNumThreads(4) + SetGraphOptimizationLevel 等。
    static Ort::SessionOptions createSessionOptions(
        ExecutionProvider provider = ExecutionProvider::CPU,
        int deviceId = 0,
        int interOpThreads = 4);

    /// 从模型文件创建 Session
    /// @param errorMsg 失败时填充错误消息
    /// @return 成功返回 Session，失败返回 nullptr
    static std::unique_ptr<Ort::Session> createSession(
        const std::wstring &modelPath,
        ExecutionProvider provider = ExecutionProvider::CPU,
        int deviceId = 0,
        QString *errorMsg = nullptr);

private:
    OnnxEnv() = delete;
};

} // namespace dstools::infer
```

### 4.2 ExecutionProvider

```cpp
// src/infer/common/include/dstools/ExecutionProvider.h
#pragma once

namespace dstools::infer {

/// 推理执行提供程序。统一替代原先 4 个独立枚举：
///   Game::ExecutionProvider, Some::ExecutionProvider,
///   Rmvpe::ExecutionProvider, HFA 的 Provider.h
enum class ExecutionProvider {
    CPU = 0,
    DML = 1,
    CUDA = 2
};

} // namespace dstools::infer
```

**兼容性**：各推理库内部原本使用自己的枚举。重构时需修改：
- `game-infer`: `Game::ExecutionProvider` → `dstools::infer::ExecutionProvider`，或在 `Game` 命名空间内 `using dstools::infer::ExecutionProvider;`
  - **⚠ 注意枚举值序不同**：`game-infer` 原始顺序为 `{CPU, CUDA, DML}`（CUDA=1, DML=2），而统一枚举为 `{CPU, DML, CUDA}`（DML=1, CUDA=2）。迁移时必须全面替换符号引用，禁止使用整数值做隐式转换，否则会导致 CUDA/DML 静默互换。
- `some-infer`, `rmvpe-infer`, `hubert-infer`: 枚举值序与统一枚举一致，直接替换即可

### 4.3 hubert-infer（新提取）

从 `src/apps/HubertFA/util/` 中提取的推理核心：

```cpp
// src/infer/hubert-infer/include/hubert/Hfa.h
#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <filesystem>

namespace HFA {

class AlignmentDecoder;
class NonLexicalDecoder;
class DictionaryG2P;
class HfaModel;
struct WordList;

/// HuBERT 强制对齐推理引擎。
/// 原 src/apps/HubertFA/util/Hfa.h — 接口完全保留。
class HFA {
public:
    /// @param modelDir  模型目录（含 model.onnx, config.json, vocab.json）
    /// @param provider  执行提供程序
    /// @param deviceId  GPU 设备 ID（仅 CUDA/DML 有效）
    explicit HFA(const std::filesystem::path &modelDir,
                 dstools::infer::ExecutionProvider provider,
                 int deviceId = 0);
    ~HFA();

    /// 对单个 WAV 文件执行音素对齐
    /// @param wavPath       输入音频路径
    /// @param language       语言标识
    /// @param non_speech_ph  非词汇音素列表
    /// @param words          [out] 输出词列表
    /// @param msg            [out] 错误消息
    /// @return 成功返回 true
    bool recognize(const std::filesystem::path &wavPath,
                   const std::string &language,
                   const std::vector<std::string> &non_speech_ph,
                   WordList &words,
                   std::string &msg) const;

    bool initialized() const;

private:
    std::unique_ptr<HfaModel> m_model;                                  // BUG-007: unique_ptr (原 m_hfa)
    std::map<std::string, std::unique_ptr<DictionaryG2P>> m_dictG2p;    // BUG-007: unique_ptr
    std::unique_ptr<AlignmentDecoder> m_alignmentDecoder;               // BUG-007: unique_ptr
    std::unique_ptr<NonLexicalDecoder> m_nonLexicalDecoder;             // BUG-007: unique_ptr
    std::unordered_set<std::string> m_silent_phonemes;
    int hfa_input_sample_rate = 44100;
};

} // namespace HFA
```

---

## 5. Applications

### 5.1 DatasetPipeline（合并 AudioSlicer + LyricFA + HubertFA）

```cpp
// src/apps/pipeline/PipelineWindow.h
class PipelineWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit PipelineWindow(QWidget *parent = nullptr);
    ~PipelineWindow() override;

private:
    // 三个 Tab 页面（各自独立操作）
    SlicerPage *m_slicerPage;       // Tab 1: 原 AudioSlicer（继承 QWidget，不继承 TaskWindow）
    LyricFAPage *m_lyricFAPage;     // Tab 2: 原 LyricFA（继承 TaskWindow）
    HubertFAPage *m_hubertFAPage;   // Tab 3: 原 HubertFA（继承 TaskWindow）

    // 无全局进度条/Run All 按钮 — 每个 Tab 页有各自的运行控件
    QTabWidget *m_tabWidget;
    dstools::Config m_config;
};
```

**SlicerPage**（原 AudioSlicer 完整 UI，迁移为 QWidget）：

> **重要**: SlicerPage **不继承 TaskWindow**，因为 AudioSlicer 的 UI 和信号
> 与 AsyncTaskWindow 不兼容（参见 architecture.md §2.3）。

```cpp
// src/apps/pipeline/slicer/SlicerPage.h
class SlicerPage : public QWidget {
    Q_OBJECT
public:
    explicit SlicerPage(QWidget *parent = nullptr);

private:
    // 原 AudioSlicer MainWindow 的全部 UI 平移
    // 注意：原代码参数使用 QLineEdit（非 QSpinBox），重构时可改为 QSpinBox 提升体验
    QListWidget *m_fileList;
    QLineEdit *m_lineThreshold;       // 原 lineEditThreshold
    QLineEdit *m_lineMinLength;       // 原 lineEditMinLen
    QLineEdit *m_lineMinInterval;     // 原 lineEditMinInterval
    QLineEdit *m_lineHopSize;         // 原 lineEditHopSize
    QLineEdit *m_lineMaxSilence;      // 原 lineEditMaxSilence
    QComboBox *m_cmbOutputFormat;     // 原 cmbOutputWaveFormat
    QComboBox *m_cmbSlicingMode;      // 原 cmbSlicingMode
    QSpinBox *m_spnSuffixDigits;      // 原 spinBoxSuffixDigits
    QCheckBox *m_chkOverwriteMarkers; // 原 cbOverwriteMarkers
    QLineEdit *m_lineOutputDir;       // 原 lineEditOutputDir
    QPushButton *m_btnRun;            // 原 pushButtonStart
    QProgressBar *m_progressBar;      // 原 progressBar
    QTextEdit *m_logOutput;           // 原 txtLogs（QTextEdit，非 QPlainTextEdit）
    QThreadPool *m_threadPool;

    // Windows 任务栏进度
    ITaskbarList3 *m_taskbarList = nullptr;
    bool m_processing = false;

    // 注意：信号签名与 TaskWindow 不同
    // slot_oneFinished(const QString &filename, int listIndex)
    // slot_oneFailed(const QString &errmsg, int listIndex)
};
```

**LyricFAPage**（原 LyricFA 完整 UI，继承 TaskWindow）：
- 模型路径 + 加载按钮
- ASR 模式 / 歌词匹配模式
- 歌词目录、Lab 输出目录、JSON 输出目录
- 差异高亮
- 各自的 Run/Stop/进度条/日志

**HubertFAPage**（原 HubertFA 完整 UI，继承 TaskWindow）：
- 模型路径 + 加载按钮
- 语言选择、GpuSelector
- TextGrid 输出目录
- 动态 UI（模型加载后生成语言 radio + 非语音音素 checkbox）
- 各自的 Run/Stop/进度条/日志

### 5.2 MinLabel（独立 EXE，保持 QMainWindow）

```cpp
// src/apps/MinLabel/MainWindow.h
// 保持原结构，变化仅限于：
// - 使用共享 PlayWidget（替代自有 PlayWidget）
// - main.cpp 调用 AppInit + Theme
// - saveFile → bool（BUG-001）
// - covertAction → convertAction（CQ-006）

namespace Minlabel {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void openDirectory(const QString &dir);
    void openFile(const QString &filename);
    bool saveFile(const QString &filename);   // void → bool
    void labToJson(const QString &dir);
    void exportAudio(const ExportInfo &info);

private:
    QFileSystemModel *m_fileModel;
    QTreeView *m_treeView;
    TextWidget *m_textWidget;
    dstools::widgets::PlayWidget *m_playWidget;  // 共享组件

    QAction *m_openAction;
    QAction *m_exportAction;
    QAction *m_convertAction;   // 原 covertAction

    void setupUI();
    void setupMenuBar();        // 保留自己的菜单栏
    void setupShortcuts();
    void onTreeCurrentChanged(const QModelIndex &current, const QModelIndex &previous);
    void updateProgress();
};

} // namespace Minlabel
```

### 5.3 SlurCutter（独立 EXE，保持 QMainWindow）

```cpp
// src/apps/SlurCutter/MainWindow.h
// 保持原结构，变化仅限于：
// - 使用共享 PlayWidget（启用范围播放模式）
// - F0Widget 拆分为 4 文件（CQ-002）
// - main.cpp 调用 AppInit + Theme

namespace SlurCutter {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void openDirectory(const QString &dir);
    void openFile(const QString &filename);
    bool saveFile(const QString &filename);

private:
    QTreeView *m_treeView;
    QListWidget *m_sentenceWidget;
    F0Widget *m_f0Widget;
    dstools::widgets::PlayWidget *m_playWidget;

    QVector<QJsonObject> m_dsContent;
    int m_currentSentence = -1;

    // 保留的所有 signal-slot 连接
    // m_playWidget::playheadChanged → m_f0Widget::setPlayHeadPos
    // m_f0Widget::requestReloadSentence → reloadDsSentenceRequested
    // m_sentenceWidget::currentRowChanged → onSentenceChanged

    void pullEditedMidi();
    void switchFile(bool next);
    void switchSentence(bool next);
    void onSentenceChanged(int row);
    void reloadDsSentenceRequested();
};

} // namespace SlurCutter
```

### 5.4 GameInfer（独立 EXE，保持 QMainWindow）

```cpp
// src/apps/GameInfer/MainWindow.h
// 保持原结构，变化仅限于：
// - 使用共享 GpuSelector（替代自有 DmlGpuUtils）
// - main.cpp 调用 AppInit + Theme
// - 错误提示修复（BUG-012）

class MainWindow : public QMainWindow {
    // ... 原结构保留 ...
    // MainWidget 使用 dstools::widgets::GpuSelector
};
```

---

## 6. 数据结构保留清单

重构不改变任何数据格式：

| 数据 | 格式 | 读/写位置 | 保留 |
|------|------|-----------|------|
| MinLabel 标注 | `{"lab":"...", "raw_text":"...", "isCheck":true}` | Common.cpp readJsonFile/writeJsonFile | 完全保留 |
| DiffSinger 句子 | `.ds` JSON (ph_seq, note_seq, f0_seq...) | SlurCutter MainWindow | 完全保留 |
| F0 数据 | f0_seq 字符串 + f0_timestep | F0Widget | 完全保留 |
| AudioSlicer CSV | `start:end.decimal` 每行 | WorkThread | 完全保留（修复 BUG-006） |
| ASR 输出 | `.lab` 纯文本 | AsrTask | 完全保留 |
| 歌词匹配输出 | `.json` (raw_text + lab) | LyricMatcher | 完全保留 |
| TextGrid | Praat TextGrid (phones + words tier) | HfaTask via textgrid.hpp | 完全保留 |
| MIDI | Format 1, PPQ 480, 2 tracks | game-infer via wolf-midi | 完全保留 |
| INI 配置 | `QSettings::IniFormat` | 各应用 | 每个 EXE 独立 INI 文件 |
| SlurEditedFiles.txt | 每行一个文件名 | SlurCutter MainWindow | 完全保留 |
