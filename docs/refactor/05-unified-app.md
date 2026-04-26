# 05 — 应用分组与部署方案

**Version**: 3.0  
**Date**: 2026-04-26  
**变更**: v3.0 修正：AudioSlicer / LyricFA / HubertFA 使用 Tab 分页隔开，各自独立运行，**不提供一键串联**。删除 PipelineRunner 自动串联引擎。

---

## 1. 总体方案

```
部署目录:
├── DatasetPipeline.exe     ← 数据集管线（切片/歌词对齐/音素对齐，Tab 分页独立运行）
├── MinLabel.exe            ← 独立：手工标注
├── SlurCutter.exe          ← 独立：MIDI 编辑
├── GameInfer.exe           ← 独立：音频转 MIDI
├── dstools-widgets.dll     ← 共享：统一样式/组件/深色模式
├── *.dll                   ← 推理/依赖库
└── ...
```

**为什么这样分？**

| 应用 | 使用场景 | 分组依据 |
|------|----------|----------|
| AudioSlicer | 数据集批处理 — 第 1 步 | 三者是顺序流水线 |
| LyricFA | 数据集批处理 — 第 2 步 | 输入来自上一步输出 |
| HubertFA | 数据集批处理 — 第 3 步 | 输入来自上一步输出 |
| MinLabel | 手工交互：逐文件标注 | 独立使用场景 |
| SlurCutter | 手工交互：钢琴卷帘编辑 | 独立使用场景 |
| GameInfer | 单文件推理 | 独立使用场景 |

---

## 2. DatasetPipeline.exe — 详细设计

### 2.1 窗口结构

```cpp
// src/apps/pipeline/PipelineWindow.h
#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include <dstools/Config.h>

class SlicerPage;
class LyricFAPage;
class HubertFAPage;

/// 数据集处理管线主窗口。
/// 整合 AudioSlicer + LyricFA + HubertFA 三个步骤为 Tab 分页。
/// 每个 Tab 页独立操作，不提供一键串联运行。
class PipelineWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit PipelineWindow(QWidget *parent = nullptr);
    ~PipelineWindow() override;

private:
    void setupUI();
    void setupMenuBar();
    void setupConnections();

    // Tab 页面
    QTabWidget *m_tabWidget;
    SlicerPage *m_slicerPage;
    LyricFAPage *m_lyricFAPage;
    HubertFAPage *m_hubertFAPage;

    dstools::Config m_config;
};
```

> **注意**: 不再需要 `PipelineRunner`。没有全局进度条、全局 Run All 按钮。
> 每个 Tab Page 内部自带各自的 Run/Stop 按钮、进度条和日志。

### 2.2 Tab 页独立运行

每个 Tab 页完全独立：

```
┌────────────────────────────────────────────────────────────┐
│ Dataset Pipeline                                            │
├────────────────────────────────────────────────────────────┤
│  [AudioSlicer] │ [LyricFA] │ [HubertFA]                    │
│  ─────────────┴───────────┴──────────────────────────────  │
│ ┌──────────────────────────────────────────────────────┐   │
│ │                                                      │   │
│ │  该 Tab 完整的 UI:                                   │   │
│ │  - 文件列表 + 拖放                                   │   │
│ │  - 参数配置面板                                       │   │
│ │  - [▶ Run]  [■ Stop]                                 │   │
│ │  - 进度条                                             │   │
│ │  - 日志输出                                           │   │
│ │                                                      │   │
│ └──────────────────────────────────────────────────────┘   │
├────────────────────────────────────────────────────────────┤
│ Ready                                                       │
└────────────────────────────────────────────────────────────┘
```

用户工作流：
1. 在 AudioSlicer Tab 中添加音频、配置参数、Run、等待完成
2. 切换到 LyricFA Tab，将 AudioSlicer 的输出目录设为输入
3. 在 LyricFA Tab 中加载 ASR 模型、Run、等待完成
4. 切换到 HubertFA Tab，将 LyricFA 的输出设为输入
5. 在 HubertFA Tab 中加载模型、Run、等待完成

### 2.3 SlicerPage 特殊处理

**AudioSlicer 不继承 TaskWindow**。原 AudioSlicer 有自己独特的 UI 和信号签名：

| 差异 | AsyncTaskWindow (LyricFA/HubertFA) | AudioSlicer |
|------|-----------------------------------|-|
| 基类 | `AsyncTaskWindow : QMainWindow` | `QMainWindow`（独立） |
| 文件列表 | `QListWidget` + `slot_addFile` | 自有 `QListWidget` + 自有 slot |
| 完成信号 | `slot_oneFinished(filename, msg)` | `slot_oneFinished(filename, listIndex)` — **签名不同** |
| 参数面板 | 子类在 `m_rightPanel` 添加 | 完整的手工 UI（阈值/长度/间隔/跳跃/静音/格式/模式/后缀位数） |
| 主题菜单 | 无 | `initStylesMenu()` 自有样式系统 |
| 任务栏进度 | 无 | `ITaskbarList3` + `nativeEvent` |
| 状态管理 | `m_isRunning` | `m_processing` + `setProcessing()` |

因此：
- `SlicerPage` 继承 `QWidget`（不继承 TaskWindow）
- 将原 `MainWindow` 的 UI、逻辑、信号完整平移
- `ITaskbarList3` HWND 通过 `window()->winId()` 获取
- 原 `showEvent` / `nativeEvent` 需适配（QWidget 无 nativeEvent，需从 PipelineWindow 转发或改用 `QAbstractNativeEventFilter`）
- 原 `initStylesMenu()` 删除（由全局 Theme 替代）

### 2.4 LyricFAPage / HubertFAPage

这两个继承 `TaskWindow`（原 `AsyncTaskWindow` 的重构版），与原代码保持一致：

**LyricFAPage**（原 LyricFAWindow）：
- 继承 `TaskWindow`
- `initCustomUI()` 中添加：模型路径、Lab/JSON/歌词输出目录、拼音选项、"Match Lyrics" 按钮
- `runTasks()` 中提交 `AsrTask` / `LyricMatchTask`
- 保留差异高亮

**HubertFAPage**（原 HubertFAWindow）：
- 继承 `TaskWindow`
- `initCustomUI()` 中添加：模型路径、TextGrid 输出目录、`GpuSelector`
- 模型加载后动态生成语言 radio 和非语音音素 checkbox
- `runTasks()` 中提交 `HfaTask`

### 2.5 main.cpp

```cpp
// src/apps/pipeline/main.cpp
#include <QApplication>
#include <dstools/AppInit.h>
#include <dstools/Theme.h>
#include "PipelineWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Dataset Pipeline");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("Team OpenVPI");

    dstools::AppInit::init(app, /*initPinyin=*/true);
    dstools::Theme::apply(app, dstools::Theme::Dark);

    PipelineWindow window;
    window.show();
    return app.exec();
}
```

---

## 3. 独立 EXE — 统一样式接入

### 3.1 MinLabel.exe

```cpp
// src/apps/MinLabel/main.cpp
#include <QApplication>
#include <dstools/AppInit.h>
#include <dstools/Theme.h>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("MinLabel");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Team OpenVPI");

    dstools::AppInit::init(app, /*initPinyin=*/true, /*initCrashHandler=*/true);
    dstools::Theme::apply(app, dstools::Theme::Dark);

    Minlabel::MainWindow window;
    window.show();
    return app.exec();
}
```

对比原 `main.cpp`（65 行）：
- 删除: 手动字体设置（`QFont("Microsoft YaHei")`）→ `AppInit::init()` 统一处理
- 删除: root 检查（`getuid()`）→ `AppInit::init()` 统一处理
- 删除: QBreakpad 初始化 → `AppInit::init()` 统一处理
- 删除: Pinyin dict 加载 → `AppInit::init()` 统一处理
- 新增: `Theme::apply()` 深色主题
- MainWindow **保持 QMainWindow**，不改为 Page

### 3.2 SlurCutter.exe

```cpp
// src/apps/SlurCutter/main.cpp
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("SlurCutter");
    app.setOrganizationName("Team OpenVPI");

    dstools::AppInit::init(app);
    dstools::Theme::apply(app, dstools::Theme::Dark);

    SlurCutter::MainWindow window;
    window.show();
    return app.exec();
}
```

### 3.3 GameInfer.exe

```cpp
// src/apps/GameInfer/main.cpp
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("GameInfer");
    app.setOrganizationName("Team OpenVPI");

    dstools::AppInit::init(app);
    dstools::Theme::apply(app, dstools::Theme::Dark);

    MainWindow window;
    window.show();
    return app.exec();
}
```

### 3.4 对比表

| 方面 | 原代码 | 重构后 |
|------|--------|--------|
| 字体设置 | 各自 10-15 行 Win32 代码 | `AppInit::init()` 一行 |
| root 检查 | MinLabel/SlurCutter 各 5 行 | `AppInit::init()` 内部 |
| 深色模式 | 无 | `Theme::apply(Dark)` 一行 |
| 样式表 | MinLabel/SlurCutter 各自 app.qss (57行×2) | 全局 dark.qss 由 Theme 加载 |
| PlayWidget | 各自一份 (~350行×2) | 链接 `dstools-widgets.dll` |
| 窗口类型 | QMainWindow | **不变**，仍是 QMainWindow |
| 菜单栏 | 各自创建 | **不变** |
| 独立进程 | 是 | **不变** |

---

## 4. CMake 构建

### 4.1 四个 EXE 的 CMakeLists

```cmake
# --- DatasetPipeline ---
# src/apps/pipeline/CMakeLists.txt
add_executable(DatasetPipeline WIN32
    main.cpp
    PipelineWindow.cpp
    slicer/SlicerPage.cpp
    slicer/Slicer.cpp
    slicer/WorkThread.cpp
    lyricfa/LyricFAPage.cpp
    lyricfa/AsrTask.cpp
    lyricfa/LyricMatchTask.cpp
    lyricfa/Asr.cpp
    lyricfa/MatchLyric.cpp
    lyricfa/LyricMatcher.cpp
    lyricfa/SequenceAligner.cpp
    lyricfa/ChineseProcessor.cpp
    lyricfa/SmartHighlighter.cpp
    hubertfa/HubertFAPage.cpp
    hubertfa/HfaTask.cpp
)
target_link_libraries(DatasetPipeline PRIVATE
    dstools-widgets          # 共享样式+TaskWindow+GpuSelector
    audio-util               # 音频处理
    FunAsr                   # ASR 推理
    hubert-infer             # HuBERT 推理
    cpp-pinyin::cpp-pinyin   # G2P (LyricFA)
    SndFile::sndfile         # WAV I/O (Slicer)
    Qt6::Core Qt6::Widgets
)

# --- MinLabel ---
# src/apps/MinLabel/CMakeLists.txt
add_executable(MinLabel WIN32
    main.cpp MainWindow.cpp TextWidget.cpp ExportDialog.cpp Common.cpp
)
target_link_libraries(MinLabel PRIVATE
    dstools-widgets          # 共享样式+PlayWidget
    cpp-pinyin::cpp-pinyin
    cpp-kana::cpp-kana
    Qt6::Core Qt6::Widgets
)

# --- SlurCutter ---
# src/apps/SlurCutter/CMakeLists.txt
add_executable(SlurCutter WIN32
    main.cpp MainWindow.cpp
    F0Widget/F0Widget.cpp
    F0Widget/PianoRollRenderer.cpp
    F0Widget/NoteDataModel.cpp
    F0Widget/UndoCommands.cpp
)
target_link_libraries(SlurCutter PRIVATE
    dstools-widgets          # 共享样式+PlayWidget
    Qt6::Core Qt6::Widgets
)

# --- GameInfer ---
# src/apps/GameInfer/CMakeLists.txt
add_executable(GameInfer WIN32
    main.cpp MainWindow.cpp MainWidget.cpp
)
target_link_libraries(GameInfer PRIVATE
    dstools-widgets          # 共享样式+GpuSelector
    game-infer::game-infer
    Qt6::Core Qt6::Widgets
)
```

### 4.2 dstools-widgets.dll

```cmake
# src/widgets/CMakeLists.txt
add_library(dstools-widgets SHARED
    src/PlayWidget.cpp
    src/TaskWindow.cpp
    src/GpuSelector.cpp
    ${GLOBAL_RESOURCES}      # resources/resources.qrc (QSS + icons)
)
target_include_directories(dstools-widgets PUBLIC include)
target_link_libraries(dstools-widgets PUBLIC
    dstools-core
    dstools-audio
    Qt6::Core Qt6::Widgets Qt6::Svg
)
# 导出宏
target_compile_definitions(dstools-widgets PRIVATE DSTOOLS_WIDGETS_BUILD)
# 公共头文件中:
# #ifdef DSTOOLS_WIDGETS_BUILD
# #define DSTOOLS_WIDGETS_API Q_DECL_EXPORT
# #else
# #define DSTOOLS_WIDGETS_API Q_DECL_IMPORT
# #endif
```

### 4.3 部署脚本

```cmake
# src/CMakeLists.txt — install/deploy

# 四个 EXE
set(DEPLOY_TARGETS DatasetPipeline MinLabel SlurCutter GameInfer)

# windeployqt 对每个 EXE 运行
foreach(target ${DEPLOY_TARGETS})
    install(TARGETS ${target} RUNTIME DESTINATION .)
endforeach()

# 共享 DLL
install(TARGETS dstools-widgets RUNTIME DESTINATION .)

# 推理 DLL
install(TARGETS audio-util game-infer some-infer rmvpe-infer hubert-infer
        RUNTIME DESTINATION .)

# 外部 DLL (onnxruntime, SDL2, sndfile, etc.)
install(CODE "
    file(GLOB _ext_dlls \"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/*.dll\")
    file(INSTALL \${_ext_dlls} DESTINATION \${CMAKE_INSTALL_PREFIX})
")

# cpp-pinyin 字典
install(DIRECTORY ${CPP_PINYIN_DICT_DIR} DESTINATION dict)

# windeployqt
add_custom_target(deploy ALL)
foreach(target ${DEPLOY_TARGETS})
    add_custom_command(TARGET deploy POST_BUILD
        COMMAND ${WINDEPLOYQT} --no-translations --no-opengl-sw
            --libdir ${CMAKE_INSTALL_PREFIX}
            --plugindir ${CMAKE_INSTALL_PREFIX}/plugins
            $<TARGET_FILE:${target}>
    )
endforeach()
```

---

## 5. 配置文件方案

每个 EXE 有自己的 INI 文件（保持独立、清晰）：

```
config/
├── DatasetPipeline.ini    # Pipeline 全局 + 三步骤参数
├── MinLabel.ini           # 与原格式兼容
├── SlurCutter.ini         # 与原格式兼容
└── GameInfer.ini          # 与原格式兼容
```

DatasetPipeline.ini 格式：

```ini
[General]
lastTab=0
theme=dark

[Slicer]
sourceDir=D:/recordings
outputDir=
threshold=-40
minLength=5000
minInterval=300
hopSize=10
maxSilKept=500
outputFormat=0
slicingMode=0
suffixDigits=4
overwriteMarkers=false

[LyricFA]
modelPath=model/ParaformerAsrModel
lyricDir=
labOutputDir=
jsonOutputDir=
saveAsPinyin=false          ; 对应 m_pinyinBox "ASR result saved as pinyin"

[HubertFA]
modelPath=model/HubertFA
language=mandarin
provider=CPU                ; 新增：当前代码硬编码 CPU，重构后通过 GpuSelector 选择
deviceId=0                  ; 新增：配合 provider 使用
outputDir=
```

> **注意**: 不再有 `workDir` 概念（因为没有一键串联）。每个 Tab 各自管理输入/输出目录。

各独立 EXE 的 INI **保持原格式**，已有用户配置不丢失。`dstools::Config` 类自动处理路径：

```cpp
// Config 构造函数根据 appName 选择 INI 文件
dstools::Config config("MinLabel");
// → 读写 <appDir>/config/MinLabel.ini
```

---

## 6. 用户使用流程

### 场景 A: 制作数据集（分步流水线）

1. 打开 `DatasetPipeline.exe`
2. 在 AudioSlicer Tab 中添加源音频，配置参数，点击 Run
3. 等待切片完成，检查输出结果
4. 切换到 LyricFA Tab，加载 ASR 模型
5. 将 AudioSlicer 的输出目录中的文件添加为输入，点击 Run
6. 等待识别完成，检查结果
7. 切换到 HubertFA Tab，加载 HuBERT 模型
8. 将 LyricFA 的输出文件添加为输入，点击 Run
9. 等待完成，在输出目录获得 TextGrid 训练数据

### 场景 B: 仅使用某一步

1. 打开 `DatasetPipeline.exe`
2. 直接切换到需要的 Tab（如 HubertFA）
3. 独立配置、运行
4. 无需关心其他 Tab

### 场景 C: 手工标注

1. 打开 `MinLabel.exe`
2. 与原使用方式完全一致

### 场景 D: 编辑 MIDI

1. 打开 `SlurCutter.exe`
2. 与原使用方式完全一致

### 场景 E: 音频转 MIDI

1. 打开 `GameInfer.exe`
2. 与原使用方式完全一致
