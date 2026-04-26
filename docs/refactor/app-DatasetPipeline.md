# DatasetPipeline.exe — 重构实施文档

**Version**: 1.0  
**Date**: 2026-04-26  
**前置**: 01-architecture.md, 02-module-spec.md  
**项目规范**: 见本文档 §A「项目规范」

---

## 1. 概述

DatasetPipeline.exe 是重构后的**新建应用**，整合原 AudioSlicer、LyricFA、HubertFA 三个独立 EXE 为一个 Tab 分页窗口。每个 Tab 页独立操作，不提供一键串联运行。

| 属性 | 值 |
|------|-----|
| 目标名 | `DatasetPipeline` |
| 类型 | GUI EXE (WIN32_EXECUTABLE) |
| 版本 | 1.0.0.0 |
| 来源 | 合并 AudioSlicer 0.0.1.4 + LyricFA 0.0.0.2 + HubertFA 0.0.0.1 |
| 窗口类型 | QMainWindow + QTabWidget (3 Tab) |

---

## 2. 源文件清单

### 2.1 新建文件

| 文件 | 来源 | 说明 |
|------|------|------|
| `src/apps/pipeline/main.cpp` | 新建 | 入口，调用 AppInit + Theme |
| `src/apps/pipeline/PipelineWindow.h/.cpp` | 新建 | 主窗口：QMainWindow + QTabWidget |

### 2.2 Slicer Tab（从 AudioSlicer 迁移）

| 新路径 | 原路径 | 变化 |
|--------|--------|------|
| `pipeline/slicer/SlicerPage.h/.cpp` | `AudioSlicer/slicer/mainwindow.h/.cpp` + `mainwindow_ui.h/.cpp` | QMainWindow → QWidget；合并 UI 文件；删除 `initStylesMenu()`；nativeEvent 适配 |
| `pipeline/slicer/Slicer.h/.cpp` | `AudioSlicer/slicer/slicer.h/.cpp` | 原样迁移 |
| `pipeline/slicer/WorkThread.h/.cpp` | `AudioSlicer/slicer/workthread.h/.cpp` | 修复 BUG-006 (CSV 索引) |
| `pipeline/slicer/MathUtils.h` | `AudioSlicer/slicer/mathutils.h` | 原样迁移 |
| `pipeline/slicer/Enumerations.h` | `AudioSlicer/slicer/enumerations.h` | 原样迁移 |
| `pipeline/slicer/WinFont.h/.cpp` | `AudioSlicer/utils/winfont.h/.cpp` | 原样迁移（**原方案遗漏**，此文件提供 Windows 字体回退逻辑，迁移后由 AppInit 统一处理，可评估是否仍需保留） |

> **勘误**: 原 01-architecture.md 目录结构中遗漏了 `winfont.h/.cpp`。重构后此逻辑由 `AppInit::init()` 统一处理，该文件**可删除**。若 AudioSlicer 有特殊字体需求超出 AppInit 范围，则保留并迁移。

### 2.3 LyricFA Tab（从 LyricFA 迁移）

| 新路径 | 原路径 | 变化 |
|--------|--------|------|
| `pipeline/lyricfa/LyricFAPage.h/.cpp` | `LyricFA/gui/LyricFAWindow.h/.cpp` | AsyncTaskWindow → TaskWindow；QMainWindow → QWidget |
| `pipeline/lyricfa/AsrTask.h/.cpp` | `LyricFA/util/AsrThread.h/.cpp` | 重命名；修复 BUG-009 (线程 QMessageBox) |
| `pipeline/lyricfa/LyricMatchTask.h/.cpp` | `LyricFA/util/FaTread.h/.cpp` | 重命名 (CQ-006 拼写修正 Tread→Task) |
| `pipeline/lyricfa/Asr.h/.cpp` | `LyricFA/util/Asr.h/.cpp` | 原样迁移 |
| `pipeline/lyricfa/MatchLyric.h/.cpp` | `LyricFA/util/MatchLyric.h/.cpp` | 原样迁移 |
| `pipeline/lyricfa/LyricMatcher.h/.cpp` | `LyricFA/util/LyricMatcher.h/.cpp` | 原样迁移 |
| `pipeline/lyricfa/SequenceAligner.h/.cpp` | `LyricFA/util/SequenceAligner.h/.cpp` | 原样迁移 |
| `pipeline/lyricfa/ChineseProcessor.h/.cpp` | `LyricFA/util/ChineseProcessor.h/.cpp` | 修复 BUG-005 (中文正则) |
| `pipeline/lyricfa/SmartHighlighter.h/.cpp` | `LyricFA/util/SmartHighlighter.h/.cpp` | 原样迁移 |
| `pipeline/lyricfa/LyricData.h` | `LyricFA/util/LyricData.h` | 原样迁移（**原方案遗漏**） |
| `pipeline/lyricfa/Utils.h/.cpp` | `LyricFA/util/Utils.h/.cpp` | 原样迁移（**原方案遗漏**） |

> **勘误**: 原 01-architecture.md 目录结构中遗漏了 `LyricData.h` 和 `Utils.h/.cpp`。这两个文件是 LyricFA 的数据定义和工具函数，必须一并迁移。

### 2.4 HubertFA Tab（从 HubertFA 迁移）

| 新路径 | 原路径 | 变化 |
|--------|--------|------|
| `pipeline/hubertfa/HubertFAPage.h/.cpp` | `HubertFA/gui/HubertFAWindow.h/.cpp` | AsyncTaskWindow → TaskWindow；使用 GpuSelector |
| `pipeline/hubertfa/HfaTask.h/.cpp` | `HubertFA/util/HfaThread.h/.cpp` | 重命名；使用 hubert-infer 库 |

> HubertFA 的核心推理代码（Hfa, HfaModel, AlignmentDecoder, NonLexicalDecoder, DictionaryG2P, AlignWord）提取到 `src/infer/hubert-infer/`，不在本 EXE 源码中。

---

## 3. 依赖关系

### 3.1 CMake 链接

```cmake
target_link_libraries(DatasetPipeline PRIVATE
    dstools-widgets          # PlayWidget(未使用), TaskWindow, GpuSelector, Theme
    audio-util               # 音频处理 (LyricFA/HubertFA 使用)
    FunAsr                   # Paraformer ASR (LyricFA)
    hubert-infer             # HuBERT 推理 (HubertFA)
    cpp-pinyin::cpp-pinyin   # G2P (LyricFA 中文拼音)
    SndFile::sndfile         # WAV I/O (Slicer 直接使用 sndfile)
    Qt6::Core Qt6::Widgets
)
```

### 3.2 间接依赖

- `dstools-widgets` → `dstools-audio` → `dstools-core` → Qt6
- `hubert-infer` → `dstools-infer-common` (OnnxEnv, ExecutionProvider) → onnxruntime
- `FunAsr` → `dstools-infer-common` → onnxruntime, fftw3
- `audio-util` → SndFile, soxr, mpg123, FLAC

### 3.3 运行时依赖

| DLL | 用途 |
|-----|------|
| `dstools-widgets.dll` | 共享样式/组件 |
| `audio-util.dll` | 音频处理 |
| `hubert-infer.dll` | HuBERT 推理 |
| `onnxruntime.dll` | ONNX 推理引擎 |
| Qt6 DLLs | UI 框架 |
| `sndfile.dll` | WAV I/O |
| `dict/` 目录 | cpp-pinyin 字典 |
| `model/` 目录 | ASR + HuBERT 模型文件 |

---

## 4. 关键设计决策

### 4.1 SlicerPage 不继承 TaskWindow

原因（与源码交叉验证确认）：

| 差异点 | AsyncTaskWindow (LyricFA/HubertFA) | AudioSlicer MainWindow |
|--------|-------------------------------------|------------------------|
| 基类 | `AsyncTask::AsyncTaskWindow : QMainWindow` | 独立 `QMainWindow` |
| 完成信号 | `slot_oneFinished(QString filename, QString msg)` | `slot_oneFinished(QString filename, int listIndex)` — **签名不同** |
| 失败信号 | `slot_oneFailed(QString filename, QString msg)` | `slot_oneFailed(QString errmsg, int listIndex)` — **签名不同** |
| 信息信号 | 无 | `slot_oneInfo(QString infomsg)` — **独有** |
| 日志组件 | `QPlainTextEdit *m_logOutput` | 通过 `Ui::MainWindow` 的 `txtLogs`（实际类型待确认） |
| UI 布局 | `m_rightPanel` + `m_topLayout` 框架内填充 | 完全自定义 UI (`Ui::MainWindow` 代码生成) |
| 任务栏进度 | 无 | `ITaskbarList3 *m_pTaskbarList3` + `nativeEvent` |
| 样式菜单 | 无 | `initStylesMenu()` |
| 状态管理 | `m_totalTasks` / `m_finishedTasks` / `m_errorTasks` | `m_workTotal` / `m_workFinished` / `m_workError` + `m_failIndex` (QStringList) |

### 4.2 nativeEvent 适配方案

原 AudioSlicer `MainWindow` 重写 `nativeEvent()` 处理 `WM_TASKBARCREATED` 消息以初始化 `ITaskbarList3`。迁移为 QWidget 后：

**方案 A（推荐）**: 使用 `QAbstractNativeEventFilter`

```cpp
// SlicerPage.cpp
class TaskbarEventFilter : public QAbstractNativeEventFilter {
public:
    explicit TaskbarEventFilter(SlicerPage *page) : m_page(page) {}
    bool nativeEventFilter(const QByteArray &, void *message, qintptr *) override {
        auto msg = static_cast<MSG *>(message);
        if (msg->message == m_page->m_taskbarCreatedMsg) {
            m_page->initTaskbar();
            return true;
        }
        return false;
    }
private:
    SlicerPage *m_page;
};

// 在 SlicerPage 构造函数中:
m_taskbarCreatedMsg = RegisterWindowMessageW(L"TaskbarButtonCreated");
m_eventFilter = new TaskbarEventFilter(this);
qApp->installNativeEventFilter(m_eventFilter);
```

**方案 B**: 将 taskbar 初始化移到 `showEvent` 中，通过 `window()->winId()` 获取 HWND：

```cpp
void SlicerPage::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    if (!m_taskbarInitialized) {
        m_hwnd = reinterpret_cast<HWND>(window()->winId());
        CoCreateInstance(CLSID_TaskbarList, ...);
        m_taskbarInitialized = true;
    }
}
```

> **注意**: 方案 B 无法处理 explorer 重启后 taskbar 重建的场景。推荐方案 A。

### 4.3 原 AsyncTaskWindow 菜单栏处理

原 `AsyncTaskWindow` 有自己的 `createMenus()`，创建 File (Add File/Add Folder) 和 Help (About/About Qt) 菜单。迁移为 TaskWindow (QWidget) 后：

- **File/Help 菜单功能移至 PipelineWindow** 的菜单栏
- TaskWindow 改为通过按钮提供 Add File/Add Folder 功能（已在 02-module-spec.md 中定义）
- PipelineWindow 菜单栏包含: File (退出) + Help (About/About Qt)

> **勘误**: 02-module-spec.md 的 TaskWindow 接口中缺少对原 `m_rightPanel` 和 `m_topLayout` 的替代说明。实际上子类通过 `addTopWidget()` 方法向 TaskWindow 布局中插入自定义控件，替代了原来直接操作 `m_rightPanel` 的模式。

### 4.4 LyricFAPage 特殊成员

原 `LyricFAWindow` 有以下成员需在迁移中正确处理：

| 成员 | 类型 | 处理 |
|------|------|------|
| `m_asr` | `Asr *` | 保留，指向 `pipeline/lyricfa/Asr` |
| `m_mandarin` | `QSharedPointer<Pinyin::Pinyin>` | 保留 |
| `m_match` | `MatchLyric *` | 保留 |
| `m_currentMode` | `enum Mode { Mode_Asr, Mode_MatchLyric }` | 保留（控制 runTask 行为） |
| `m_pinyinBox` | `QCheckBox *` | 保留（"ASR result saved as pinyin"） |
| `m_modelEdit` / `m_modelStatusLabel` | QLineEdit / QLabel | 保留 |

### 4.5 HubertFAPage 特殊成员

原 `HubertFAWindow` 有以下成员：

| 成员 | 类型 | 处理 |
|------|------|------|
| `m_hfa` | `HFA::HFA *` | 改为使用 `hubert-infer` 库接口 |
| `m_languageGroup` | `QButtonGroup *` | 保留（动态生成的语言 radio） |
| `m_nonSpeechPhLayout` | `QHBoxLayout *` | 保留（动态生成的非语音音素 checkbox） |
| `m_dynamicContainer` | `QWidget *` | 保留（模型加载后生成的动态 UI 容器） |
| `m_errorFormat` | `QTextCharFormat` | 保留（错误日志格式） |
| `m_modelLoadBtn` | `QPushButton *` | 保留 |

HubertFAPage 使用共享 `GpuSelector` 替代原 `DmlGpuUtils`，需在 `initCustomUI()` 中创建并添加到布局。

---

## 5. Bug 修复清单

以下 Bug 在 DatasetPipeline 构建过程中修复：

| Bug ID | 严重度 | 修复位置 | 简述 |
|--------|--------|----------|------|
| BUG-004 | Medium | `LyricFAPage.cpp`, `HubertFAPage.cpp` | 删除 4 处硬编码开发路径 |
| BUG-005 | Medium | `ChineseProcessor.cpp` | 中文正则 `[^[\\u4e00-\\u9fa5a]]` → `[^\\u4e00-\\u9fa5]` |
| BUG-006 | **Critical** | `WorkThread.cpp` | CSV `capturedView(0/1/2)` → `capturedView(1/2/3)` |
| BUG-009 | High | `AsrTask.cpp` | 工作线程 QMessageBox → emit signal |
| BUG-011 | Low | `DictionaryG2P.h`（在 hubert-infer 中） | `#endif DICTIONARY_G2P_H` → `#endif //` |
| BUG-021 | Medium | `HfaModel.cpp`（在 hubert-infer 中） | 空 session 守卫 |
| CQ-006 | Low | 文件名 + 类名 | `FaTread` → `LyricMatchTask` |

---

## 6. CMakeLists.txt

```cmake
# src/apps/pipeline/CMakeLists.txt

set(PIPELINE_VERSION 1.0.0.0)

# --- 源文件 ---
set(PIPELINE_SOURCES
    main.cpp
    PipelineWindow.h    PipelineWindow.cpp

    # Slicer Tab
    slicer/SlicerPage.h      slicer/SlicerPage.cpp
    slicer/Slicer.h           slicer/Slicer.cpp
    slicer/WorkThread.h       slicer/WorkThread.cpp
    slicer/MathUtils.h
    slicer/Enumerations.h

    # LyricFA Tab
    lyricfa/LyricFAPage.h     lyricfa/LyricFAPage.cpp
    lyricfa/AsrTask.h          lyricfa/AsrTask.cpp
    lyricfa/LyricMatchTask.h   lyricfa/LyricMatchTask.cpp
    lyricfa/Asr.h              lyricfa/Asr.cpp
    lyricfa/MatchLyric.h       lyricfa/MatchLyric.cpp
    lyricfa/LyricMatcher.h     lyricfa/LyricMatcher.cpp
    lyricfa/SequenceAligner.h  lyricfa/SequenceAligner.cpp
    lyricfa/ChineseProcessor.h lyricfa/ChineseProcessor.cpp
    lyricfa/SmartHighlighter.h lyricfa/SmartHighlighter.cpp
    lyricfa/LyricData.h
    lyricfa/Utils.h            lyricfa/Utils.cpp

    # HubertFA Tab
    hubertfa/HubertFAPage.h   hubertfa/HubertFAPage.cpp
    hubertfa/HfaTask.h         hubertfa/HfaTask.cpp
)

# --- 可执行文件 ---
add_executable(DatasetPipeline WIN32 ${PIPELINE_SOURCES})

if(APPLE)
    set_target_properties(DatasetPipeline PROPERTIES MACOSX_BUNDLE TRUE)
endif()

target_include_directories(DatasetPipeline PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/slicer
    ${CMAKE_CURRENT_SOURCE_DIR}/lyricfa
    ${CMAKE_CURRENT_SOURCE_DIR}/hubertfa
)

target_link_libraries(DatasetPipeline PRIVATE
    dstools-widgets
    audio-util
    FunAsr
    hubert-infer
    cpp-pinyin::cpp-pinyin
    SndFile::sndfile
    Qt6::Core Qt6::Widgets
)

# Windows 资源文件 (版本信息)
if(WIN32)
    include(${PROJECT_SOURCE_DIR}/cmake/winrc.cmake)
    generate_win_rc(DatasetPipeline
        VERSION ${PIPELINE_VERSION}
        DESCRIPTION "DiffSinger Dataset Pipeline"
    )
endif()

# 注册部署目标
add_dependencies(DeployedTargets DatasetPipeline)
```

> **勘误 vs 05-unified-app.md §4.1**: 原 CMake 示例遗漏了 `LyricData.h`、`Utils.h/.cpp`、头文件搜索路径配置、版本信息和 macOS bundle 设置。

---

## 7. 配置文件

文件: `config/DatasetPipeline.ini`

```ini
[General]
lastTab=0
theme=dark

[Slicer]
sourceDir=
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
saveAsPinyin=false

[HubertFA]
modelPath=model/HubertFA
language=mandarin
provider=CPU
deviceId=0
outputDir=
```

---

## 8. 验证清单

### 8.1 构建验证

- [ ] `DatasetPipeline.exe` 编译零 warning
- [ ] 链接 `dstools-widgets.dll` 成功
- [ ] windeployqt 成功

### 8.2 功能验证

**Tab 1 — AudioSlicer:**
- [ ] 拖放/浏览添加 WAV 文件
- [ ] 参数配置（阈值/最小长度/最小间隔/跳跃/最大静音）
- [ ] 批量切片 + WAV 输出
- [ ] CSV 标记导出/导入（BUG-006 修复验证）
- [ ] 输出格式选择（16/24/32-bit）
- [ ] 切片模式选择
- [ ] 文件名后缀位数配置
- [ ] 覆盖标记选项
- [ ] Windows 任务栏进度（通过 QAbstractNativeEventFilter）

**Tab 2 — LyricFA:**
- [ ] 加载 Paraformer ASR 模型
- [ ] 批量 ASR → .lab（BUG-009 验证：失败不弹对话框）
- [ ] 歌词匹配（Needleman-Wunsch）→ .json
- [ ] 差异高亮
- [ ] 中文文本清洗（BUG-005 验证）
- [ ] 无硬编码路径残留（BUG-004 验证）

**Tab 3 — HubertFA:**
- [ ] 加载 HuBERT ONNX 模型（CPU/DML/CUDA）
- [ ] 批量音素对齐 → TextGrid
- [ ] GPU 选择（GpuSelector 组件）
- [ ] 多语言 G2P + 动态 UI
- [ ] AP/SP 检测
- [ ] 无硬编码路径残留（BUG-004 验证）

**分页独立运行:**
- [ ] 三个 Tab 页各自独立配置和运行
- [ ] 每个 Tab 有独立的 Run/Stop/进度条/日志
- [ ] Tab 切换不影响正在运行的任务
- [ ] 启动时恢复上次选中的 Tab

### 8.3 回归验证

- [ ] AudioSlicer 输出与原独立 EXE 输出逐字节一致
- [ ] LyricFA .lab/.json 输出与原独立 EXE 一致
- [ ] HubertFA TextGrid 输出与原独立 EXE 一致

---

## 9. 迁移步骤

1. 创建 `src/apps/pipeline/` 目录结构
2. 新建 `PipelineWindow.h/.cpp`（骨架：QMainWindow + QTabWidget + 3 空 Tab）
3. 迁移 AudioSlicer → SlicerPage（含 BUG-006 + nativeEvent 适配）
4. 迁移 LyricFA → LyricFAPage（含 BUG-004/005/009 + CQ-006）
5. 迁移 HubertFA → HubertFAPage（含 BUG-004/011/021 + GpuSelector 接入）
6. 编写 `main.cpp`（AppInit + Theme）
7. 编写 CMakeLists.txt
8. 验证分页独立运行
9. 删除原 AudioSlicer/LyricFA/HubertFA 的 `main.cpp` 和独立 CMakeLists

---

## A. 项目规范

本节定义所有 4 个 EXE 重构文档遵循的统一规范。

### A.1 文档结构规范

每个 EXE 文档包含以下章节（按序）：

1. **概述** — 目标名、类型、版本、来源、窗口类型
2. **源文件清单** — 新建/迁移/删除文件的完整映射表
3. **依赖关系** — CMake 链接、间接依赖、运行时 DLL
4. **关键设计决策** — 该 EXE 特有的架构决策及理由
5. **Bug 修复清单** — 该 EXE 范围内的 Bug ID、严重度、修复位置
6. **CMakeLists.txt** — 完整 CMake 构建配置
7. **配置文件** — INI 格式及字段说明
8. **验证清单** — 构建/功能/回归验证 checkbox
9. **迁移步骤** — 有序步骤列表

### A.2 代码规范

| 规范项 | 要求 |
|--------|------|
| 命名空间 | 各 EXE 保留原命名空间（`Minlabel`, `SlurCutter`, 全局） |
| 共享层命名空间 | `dstools::` (core), `dstools::audio::` (audio), `dstools::widgets::` (widgets), `dstools::infer::` (infer) |
| 头文件守卫 | `#pragma once` |
| 指针 | 新代码使用 `std::unique_ptr`；Qt 父子树管理的用裸指针 |
| UI 字符串 | 全部用 `tr()` 包裹 |
| 错误处理 | 工作线程通过 signal 传递错误，主线程可弹 QMessageBox，禁止 `exit()` |
| 日志 | `qDebug()` / `qWarning()` / `qCritical()`，禁止 `printf`/`std::cout` |

### A.3 共享层依赖规范

所有 4 个 EXE 通过以下共享库获得统一能力：

| 库 | 类型 | 提供 |
|-----|------|------|
| `dstools-core` | STATIC | AppInit, Config, ErrorHandling, ThreadUtils, Theme |
| `dstools-audio` | STATIC | AudioDecoder (FFmpeg), AudioPlayback (SDL2), WaveFormat |
| `dstools-widgets` | **SHARED** | PlayWidget, TaskWindow, GpuSelector + QSS/图标资源 |
| `dstools-infer-common` | STATIC | OnnxEnv, ExecutionProvider |

每个 EXE 的 `main.cpp` 模板：

```cpp
#include <QApplication>
#include <dstools/AppInit.h>
#include <dstools/Theme.h>
#include "MainWindow.h"  // 或 PipelineWindow.h

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("<AppName>");
    app.setApplicationVersion("<Version>");
    app.setOrganizationName("Team OpenVPI");

    dstools::AppInit::init(app, /*initPinyin=*/false, /*initCrashHandler=*/false);
    dstools::Theme::apply(app, dstools::Theme::Dark);

    <Window> window;
    window.show();
    return app.exec();
}
```
