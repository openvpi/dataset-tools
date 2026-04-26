# SlurCutter.exe — 重构实施文档

**Version**: 1.0  
**Date**: 2026-04-26  
**前置**: 01-architecture.md, 02-module-spec.md, app-DatasetPipeline.md §A「项目规范」

---

## 1. 概述

SlurCutter.exe 是**保留的独立 EXE**，用于 DiffSinger MIDI 句子编辑（钢琴卷帘 + F0 曲线）。重构重点是 F0Widget 拆分（CQ-002）、PlayWidget 替换和统一样式接入。

| 属性 | 值 |
|------|-----|
| 目标名 | `SlurCutter` |
| 类型 | GUI EXE (WIN32_EXECUTABLE) |
| 版本 | 0.1.0（从 0.0.1.4 升至 0.1.0） |
| 来源 | 原 `src/apps/SlurCutter/` 原地重构 |
| 窗口类型 | QMainWindow（不变） |
| 命名空间 | `SlurCutter`（保留） |

---

## 2. 源文件清单

### 2.1 保留文件（修改）

| 文件 | 变化 |
|------|------|
| `main.cpp` | 重写：AppInit + Theme（同 MinLabel 模式） |
| `gui/MainWindow.h/.cpp` | `PlayWidget *` → `dstools::widgets::PlayWidget *`；CQ-004: 2处 foreach → 范围 for (line 247, 356) |
| `gui/DsSentence.h` | 无变化 |

### 2.2 拆分文件（CQ-002: F0Widget 1097行 → 4 文件）

| 新文件 | 来源 | 内容 |
|--------|------|------|
| `gui/F0Widget/F0Widget.h/.cpp` | 原 `gui/F0Widget.h/.cpp` | 轻量组合器 + 鼠标/键盘输入处理（~160行） |
| `gui/F0Widget/PianoRollRenderer.h/.cpp` | 原 F0Widget `paintEvent` 部分 | QPainter 渲染逻辑（~320行），使用 `Theme::palette()` |
| `gui/F0Widget/NoteDataModel.h/.cpp` | 原 F0Widget 数据成员 + 操作方法 | 数据管理：加载/修改/查询音符和 F0（~250行） |
| `gui/F0Widget/MusicUtils.h` | 原 F0Widget 静态工具函数 | `noteNameToMidiNote`, `midiNoteToName` 等（~60行） |
| `gui/F0Widget/UndoCommands.h/.cpp` | **新建** | FEAT-001: QUndoCommand 子类（7 种编辑操作） |

> **注意**: 原 `gui/F0Widget.h/.cpp` 作为 2 个文件，拆分为 `gui/F0Widget/` 目录下的 8+1 个文件。

### 2.3 保留文件（不变）

| 文件 | 说明 |
|------|------|
| `gui/intervaltree.hpp` | 第三方头文件，原样保留（**原方案遗漏**，未在 01-architecture.md 目录结构中列出） |

### 2.4 删除文件

| 文件 | 原因 |
|------|------|
| `gui/PlayWidget.h` | 被 `dstools-widgets::PlayWidget` 替代 |
| `gui/PlayWidget.cpp` | 同上 |
| 原 `gui/F0Widget.h` | 拆分后删除 |
| 原 `gui/F0Widget.cpp` | 拆分后删除 |

---

## 3. 依赖关系

### 3.1 CMake 链接

```cmake
target_link_libraries(SlurCutter PRIVATE
    dstools-widgets          # PlayWidget (范围播放模式) + Theme
    Qt6::Core Qt6::Widgets
)
```

**对比原依赖变化**：

| 原依赖 | 变化 |
|--------|------|
| `SDLPlayback` (static) | 间接通过 dstools-widgets |
| `FFmpegDecoder` (static) | 间接通过 dstools-widgets |

SlurCutter 是依赖最少的 EXE——仅需 Qt + dstools-widgets。

### 3.2 运行时依赖

| DLL | 用途 |
|-----|------|
| `dstools-widgets.dll` | PlayWidget + Theme |
| Qt6 DLLs | UI |
| SDL2.dll | 音频播放 |
| FFmpeg DLLs | 音频解码 |

---

## 4. 关键设计决策

### 4.1 PlayWidget 范围播放模式

SlurCutter 的 PlayWidget 使用**范围播放**——用户在 F0Widget 中选择一段区域后，仅播放该区域的音频，同时 F0Widget 上显示播放头位置。

原 `SlurCutter::PlayWidget` 特有功能：
- `setRange(double start, double end)` — 设置播放范围
- `playheadChanged(double position)` signal — 播放头位置（通过 steady_clock 高精度跟踪）
- `estimatedTimeMs()` — 基于 steady_clock 的时间估算
- `reloadFinePlayheadStatus()` — 精细播放头更新
- `lastObtainedTimeMs`, `pauseAtTime`, `lastObtainedTimePoint` — 时间跟踪状态

**映射到共享 PlayWidget**：

| 原接口 | 共享 PlayWidget | 说明 |
|--------|----------------|------|
| `setRange(start, end)` | `setPlayRange(startSec, endSec)` | 重命名 |
| `playheadChanged(position)` | `playheadChanged(positionSec)` | 信号签名保持一致 |
| `estimatedTimeMs()` | 内部实现（`m_playStartTime` + steady_clock） | 不暴露为公共接口 |

**关键 connect**：

```cpp
// MainWindow 构造函数中
connect(m_playWidget, &dstools::widgets::PlayWidget::playheadChanged,
        m_f0Widget, &F0Widget::setPlayHeadPos);
```

### 4.2 F0Widget 拆分策略

原 F0Widget (1097行) 承担了 4 个职责：数据模型、渲染、输入处理、工具函数。拆分后各文件职责清晰：

```
F0Widget (QWidget, ~160行)
  ├── 持有 NoteDataModel + PianoRollRenderer
  ├── 鼠标事件 → 转发到 Model 操作
  ├── 键盘事件 → 转发到 Model 操作
  ├── paintEvent → 委托给 PianoRollRenderer
  └── QUndoStack (FEAT-001)

NoteDataModel (QObject, ~250行)
  ├── 加载 DsSentence JSON
  ├── 音符 CRUD
  ├── F0 数据
  └── signals: dataChanged, errorOccurred

PianoRollRenderer (~320行, 非 QObject)
  ├── render(QPainter &, QRect) — 主渲染
  ├── 使用 Theme::palette() 获取色板
  └── 坐标转换辅助

MusicUtils (~60行, 纯头文件)
  ├── noteNameToMidiNote / midiNoteToName
  └── 其他音乐理论工具

UndoCommands (QUndoCommand 子类, 新建)
  ├── SplitNoteCommand
  ├── MergeNoteCommand
  ├── TransposeCommand
  ├── SetGlideCommand
  ├── MoveNoteCommand
  ├── ResizeNoteCommand
  └── SetPhonemeCommand
```

### 4.3 PianoRollRenderer 主题适配

原 F0Widget 使用硬编码颜色（白色背景、红色 F0 曲线等）。重构后通过 `Theme::palette()` 获取色板：

```cpp
// PianoRollRenderer::render()
const auto &p = dstools::Theme::palette();
painter.fillRect(rect, p.bgPrimary);          // 原: Qt::white
// F0 曲线
QPen f0Pen(p.error, 2.0);                     // 原: Qt::red
// 音符
QColor noteColor = p.accent;                   // 原: QColor(80,160,255)
// 选中音符
QColor noteSelected = p.warning;               // 原: QColor(255,200,50)
```

完整色彩映射见 04-ui-theming.md §4.1。

---

## 5. Bug 修复清单

| Bug ID | 严重度 | 修复位置 | 简述 |
|--------|--------|----------|------|
| BUG-001 | **Critical** | 删除 `PlayWidget.cpp` | 原 1 处 exit 随删除消除；共享 PlayWidget 用 `m_valid` 守卫 |
| BUG-003 | Medium | `F0Widget/F0Widget.cpp` | switch case Glide 缺少 `break` |
| BUG-010 | Medium | 删除 `PlayWidget.h` | 原未初始化指针随删除消除 |
| BUG-013 | Medium | `F0Widget/NoteDataModel.cpp` | `f0Values` 空向量守卫 |
| BUG-014 | Medium | 删除 `PlayWidget.cpp` | 已知崩溃注释随删除消除 |
| BUG-024 | Medium | `F0Widget/NoteDataModel.cpp` | `splitPitch[1]` 越界守卫 |
| CQ-002 | Medium | `gui/F0Widget/` (拆分) | 1097行 → 4+1 文件 |
| CQ-004 | Low | `MainWindow.cpp` (2处) | `foreach` → 范围 for |
| CQ-004 | Low | `NoteDataModel.cpp` (1处) | `foreach` → 范围 for (原 F0Widget.cpp:147) |
| CQ-004 | Low | `PianoRollRenderer.cpp` (2处) | `foreach` → 范围 for (原 F0Widget.cpp:829,833) |
| CQ-005 | Low | `MainWindow.cpp` | 循环内 regex → `static const` |
| CQ-007 | Low | `F0Widget/F0Widget.cpp` | 不雅注释替换 |
| FEAT-001 | High | `F0Widget/UndoCommands.h/.cpp` (新建) | QUndoStack 撤销/重做 |

---

## 6. CMakeLists.txt

```cmake
# src/apps/SlurCutter/CMakeLists.txt

set(SLURCUTTER_VERSION 0.1.0.0)

add_executable(SlurCutter WIN32
    main.cpp
    gui/MainWindow.h        gui/MainWindow.cpp
    gui/DsSentence.h
    gui/intervaltree.hpp
    gui/F0Widget/F0Widget.h        gui/F0Widget/F0Widget.cpp
    gui/F0Widget/PianoRollRenderer.h gui/F0Widget/PianoRollRenderer.cpp
    gui/F0Widget/NoteDataModel.h   gui/F0Widget/NoteDataModel.cpp
    gui/F0Widget/MusicUtils.h
    gui/F0Widget/UndoCommands.h    gui/F0Widget/UndoCommands.cpp
    # 注意: gui/PlayWidget.h/.cpp 已删除
    # 注意: 原 gui/F0Widget.h/.cpp 拆分到 gui/F0Widget/ 目录
)

if(APPLE)
    set_target_properties(SlurCutter PROPERTIES MACOSX_BUNDLE TRUE)
endif()

target_include_directories(SlurCutter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/gui
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/F0Widget
)

target_link_libraries(SlurCutter PRIVATE
    dstools-widgets
    Qt6::Core Qt6::Widgets
)

if(WIN32)
    include(${PROJECT_SOURCE_DIR}/cmake/winrc.cmake)
    generate_win_rc(SlurCutter
        VERSION ${SLURCUTTER_VERSION}
        DESCRIPTION "SlurCutter - DiffSinger MIDI Editor"
    )
endif()

add_dependencies(DeployedTargets SlurCutter)
```

**对比原 CMakeLists (34行) 变化**：

| 变化 | 说明 |
|------|------|
| 删除 `gui/PlayWidget.cpp`, `gui/F0Widget.cpp` | 替换/拆分 |
| 新增 `gui/F0Widget/` 下 5 个 .cpp | F0Widget 拆分产物 |
| 新增 `gui/F0Widget/UndoCommands.cpp` | FEAT-001 |
| 删除 `SDLPlayback` / `FFmpegDecoder` 链接 | 间接通过 dstools-widgets |
| 新增 `dstools-widgets` | 统一样式/PlayWidget |
| 新增 `target_include_directories` | F0Widget 子目录 |

---

## 7. 配置文件

文件: `config/SlurCutter.ini`（与原格式兼容）

```ini
[General]
lastDir=
theme=dark          ; 新增

[View]
zoomLevel=100       ; 新增：缩放百分比持久化
```

---

## 8. 验证清单

### 8.1 构建验证

- [ ] `SlurCutter.exe` 编译零 warning
- [ ] F0Widget 拆分后编译通过
- [ ] 链接 `dstools-widgets.dll` 成功

### 8.2 功能验证

- [ ] .ds JSON 加载 + 句子导航
- [ ] F0 曲线显示正确（深色主题下）
- [ ] MIDI 音符显示正确（深色主题下）
- [ ] 分割音符（BUG-003 验证：Glide 操作后状态正确）
- [ ] 合并音符
- [ ] 移调
- [ ] 滑音设置
- [ ] 范围播放（共享 PlayWidget）
- [ ] 播放头同步（F0Widget 上显示绿色播放头线）
- [ ] 撤销/重做（FEAT-001）
- [ ] 编辑追踪（SlurEditedFiles.txt）
- [ ] 缩放/滚动
- [ ] 深色主题正确（F0Widget 自绘区域色彩正确）
- [ ] 拔掉音频设备 → 播放禁用，不崩溃
- [ ] 空 f0_seq 句子 → 错误提示，不崩溃（BUG-013）
- [ ] 纯音名 "C4"（无偏移）→ cents=0，不崩溃（BUG-024）

### 8.3 回归验证

- [ ] 编辑后保存的 .ds JSON 与原版格式完全一致
- [ ] SlurEditedFiles.txt 格式不变
- [ ] 原有快捷键全部可用

---

## 9. 迁移步骤

1. 创建 `gui/F0Widget/` 目录
2. 拆分 `gui/F0Widget.h/.cpp` → 4 文件 (F0Widget, PianoRollRenderer, NoteDataModel, MusicUtils)
3. 在拆分过程中修复 BUG-003/013/024、CQ-004/005/007
4. 新建 `gui/F0Widget/UndoCommands.h/.cpp`（FEAT-001）
5. 删除 `gui/PlayWidget.h/.cpp`
6. 修改 `MainWindow.h/.cpp`: 使用共享 PlayWidget（范围播放模式）
7. 修改 `MainWindow.cpp`: CQ-004 foreach 修复
8. 更新 PianoRollRenderer 使用 `Theme::palette()`
9. 重写 `main.cpp`（AppInit + Theme）
10. 更新 CMakeLists.txt
11. 编译验证 + 功能测试
