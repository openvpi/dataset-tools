# MinLabel.exe — 重构实施文档

**Version**: 1.0  
**Date**: 2026-04-26  
**前置**: 01-architecture.md, 02-module-spec.md, app-DatasetPipeline.md §A「项目规范」

---

## 1. 概述

MinLabel.exe 是**保留的独立 EXE**，用于手工逐文件音频标注。重构变化最小——保持原有 QMainWindow 架构，仅替换重复组件和接入统一样式。

| 属性 | 值 |
|------|-----|
| 目标名 | `MinLabel` |
| 类型 | GUI EXE (WIN32_EXECUTABLE) |
| 版本 | 0.1.0（从 0.0.1.8 升至 0.1.0，反映重构） |
| 来源 | 原 `src/apps/MinLabel/` 原地重构 |
| 窗口类型 | QMainWindow（不变） |
| 命名空间 | `Minlabel`（保留原拼写） |

---

## 2. 源文件清单

### 2.1 保留文件（原地修改）

| 文件 | 变化 |
|------|------|
| `src/apps/MinLabel/main.cpp` | 重写：删除手动字体/root/crash/pinyin 初始化（~50行），替换为 `AppInit::init()` + `Theme::apply()`（~10行） |
| `src/apps/MinLabel/gui/MainWindow.h` | `covertAction` → `convertAction`（CQ-006）；`saveFile` 返回 `void` → `bool`（BUG-001）；`PlayWidget *` → `dstools::widgets::PlayWidget *` |
| `src/apps/MinLabel/gui/MainWindow.cpp` | 修复 BUG-001 (saveFile/labToJson exit→return)；修复 BUG-008 (QDir::count)；CQ-006 拼写 |
| `src/apps/MinLabel/gui/TextWidget.h/.cpp` | CQ-006: `covertNum` → `convertNum`（成员变量 + 所有引用） |
| `src/apps/MinLabel/gui/ExportDialog.h/.cpp` | 无变化（或 `tr()` 包裹） |
| `src/apps/MinLabel/gui/Common.h/.cpp` | CQ-004: 2处 `foreach` → 范围 for（line 67, 205） |

### 2.2 删除文件

| 文件 | 原因 |
|------|------|
| `src/apps/MinLabel/gui/PlayWidget.h` | 被 `dstools-widgets::PlayWidget` 替代 |
| `src/apps/MinLabel/gui/PlayWidget.cpp` | 同上 |

### 2.3 新增文件

无。MinLabel 不新建文件，仅修改现有文件。

---

## 3. 依赖关系

### 3.1 CMake 链接

```cmake
target_link_libraries(MinLabel PRIVATE
    dstools-widgets          # PlayWidget + Theme + AppInit
    cpp-pinyin::cpp-pinyin   # 中文拼音 G2P
    cpp-kana::cpp-kana       # 日语假名 G2P
    Qt6::Core Qt6::Widgets
)
```

**对比原依赖变化**：

| 原依赖 | 新依赖 | 变化 |
|--------|--------|------|
| `SDLPlayback` (static lib) | `dstools-widgets` (→ `dstools-audio` → SDL2) | 间接依赖，无需直接链接 |
| `FFmpegDecoder` (static lib) | `dstools-widgets` (→ `dstools-audio` → FFmpeg) | 间接依赖 |
| `qBreakpad::qBreakpad` | `dstools-core` (通过 `dstools-widgets`) | AppInit 内部条件初始化 |
| 无 | `dstools-widgets` | **新增**：统一样式/组件 |

> **勘误**: 原 02-module-spec.md §5.2 MinLabel 接口中未提及 `qBreakpad` 的处理方式。实际上 `AppInit::init(app, initPinyin=true, initCrashHandler=true)` 第三个参数控制 qBreakpad 初始化，MinLabel 是唯一需要 `initCrashHandler=true` 的 EXE。`dstools-core` 需要条件链接 `qBreakpad`（仅当 `HAS_QBREAKPAD` 宏定义时）。

### 3.2 运行时依赖

| DLL | 用途 |
|-----|------|
| `dstools-widgets.dll` | 共享样式/PlayWidget |
| Qt6 DLLs | UI 框架 |
| SDL2.dll | 音频播放（通过 dstools-audio） |
| FFmpeg DLLs | 音频解码（通过 dstools-audio） |
| `dict/` 目录 | cpp-pinyin 字典 |

---

## 4. 关键设计决策

### 4.1 PlayWidget 替换

原 `Minlabel::PlayWidget`（72行头文件，~342行实现）替换为 `dstools::widgets::PlayWidget`。

**接口映射**：

| 原接口 (Minlabel::PlayWidget) | 新接口 (dstools::widgets::PlayWidget) | 差异 |
|------|------|------|
| `openFile(filename)` | `openFile(path)` | 参数名变化，语义相同 |
| `isPlaying()` | `isPlaying()` | 相同 |
| `setPlaying(bool)` | `setPlaying(bool)` | 相同 |
| 无 | `setPlayRange(start, end)` | 新增（MinLabel 不使用） |
| 无 | `clearPlayRange()` | 新增（MinLabel 不使用） |
| 无 | `playheadChanged` signal | 新增（MinLabel 不使用） |

MinLabel 使用 PlayWidget 的**基本模式**（openFile + play/stop），不使用范围播放功能。

**connect 变化**：

```cpp
// 原: 无外部 connect（MinLabel 的 PlayWidget 内部自管理）
// 新: 同样无需外部 connect（共享 PlayWidget 也是自管理的）
// MainWindow 仅调用 m_playWidget->openFile(path) 和 setPlaying(bool)
```

### 4.2 saveFile 签名变更

```cpp
// 原:
void saveFile(const QString &filename);

// 新:
bool saveFile(const QString &filename);
```

所有调用者需处理返回值或显式忽略：

| 调用位置 | 处理方式 |
|----------|----------|
| `onTreeCurrentChanged` (切换文件时自动保存) | 忽略返回值：`saveFile(lastFile);` — 保存失败不阻止导航 |
| 析构函数 (关闭时保存) | 忽略返回值：尽力保存 |
| 手动保存快捷键 | 检查返回值，失败时状态栏提示 |

### 4.3 保持 QMainWindow

MinLabel 有完整的菜单栏（File/Edit/Help）、状态栏、QSplitter 布局（文件树 + 编辑区 + 播放器），不适合改为 QWidget Page。保持 QMainWindow 独立进程。

---

## 5. Bug 修复清单

| Bug ID | 严重度 | 修复位置 | 简述 |
|--------|--------|----------|------|
| BUG-001 | **Critical** | `MainWindow.cpp` (3处 exit) | `saveFile` → `bool` + `labToJson` 中 `exit(-1)` → `continue` |
| BUG-001 | **Critical** | 删除 `PlayWidget.cpp` | 原 PlayWidget 的 2 处 exit 随删除消除；共享 PlayWidget 用 `m_valid` 守卫 |
| BUG-008 | Low | `MainWindow.cpp` | `dir.count()` → `dir.entryList(filters, QDir::Files).count()` |
| CQ-004 | Low | `Common.cpp` (2处) | `foreach` → 范围 for |
| CQ-006 | Low | `MainWindow.h/.cpp` (4处), `TextWidget.h` (2处) | `covertAction` → `convertAction`, `covertNum` → `convertNum` |

---

## 6. CMakeLists.txt

```cmake
# src/apps/MinLabel/CMakeLists.txt

set(MINLABEL_VERSION 0.1.0.0)

add_executable(MinLabel WIN32
    main.cpp
    gui/MainWindow.h    gui/MainWindow.cpp
    gui/TextWidget.h    gui/TextWidget.cpp
    gui/ExportDialog.h  gui/ExportDialog.cpp
    gui/Common.h        gui/Common.cpp
    # 注意: gui/PlayWidget.h/.cpp 已删除
)

if(APPLE)
    set_target_properties(MinLabel PROPERTIES MACOSX_BUNDLE TRUE)
endif()

target_link_libraries(MinLabel PRIVATE
    dstools-widgets
    cpp-pinyin::cpp-pinyin
    cpp-kana::cpp-kana
    Qt6::Core Qt6::Widgets
)

if(WIN32)
    include(${PROJECT_SOURCE_DIR}/cmake/winrc.cmake)
    generate_win_rc(MinLabel
        VERSION ${MINLABEL_VERSION}
        DESCRIPTION "MinLabel - Audio Labeling Tool"
    )
endif()

add_dependencies(DeployedTargets MinLabel)
```

**对比原 CMakeLists 变化**：

| 变化 | 说明 |
|------|------|
| 删除 `gui/PlayWidget.cpp` | 被共享 PlayWidget 替代 |
| 删除 `SDLPlayback` / `FFmpegDecoder` 链接 | 通过 dstools-widgets 间接获得 |
| 删除 `qBreakpad::qBreakpad` 直接链接 | 通过 dstools-core 间接获得 |
| 删除 pinyin dict 拷贝逻辑 | 移至全局部署脚本 |
| 新增 `dstools-widgets` | 统一样式/组件 |

---

## 7. 配置文件

文件: `config/MinLabel.ini`（**与原格式完全兼容**，用户已有配置不丢失）

```ini
[General]
lastDir=
theme=dark          ; 新增：主题偏好

[Shortcuts]
Open=Ctrl+O
Save=Ctrl+S
Export=Ctrl+E
NextFile=Ctrl+Right
PrevFile=Ctrl+Left
TogglePlay=Space
```

---

## 8. 验证清单

### 8.1 构建验证

- [ ] `MinLabel.exe` 编译零 warning
- [ ] 链接 `dstools-widgets.dll` 成功
- [ ] windeployqt 成功

### 8.2 功能验证

- [ ] 目录浏览 + 文件树
- [ ] 音频播放（共享 PlayWidget）— 播放/暂停/停止/设备切换
- [ ] JSON 标注编辑 + 自动保存
- [ ] G2P（普通话/粤语/日语）
- [ ] Lab→JSON 批量转换（BUG-001 验证：转换失败不退出）
- [ ] 导出功能
- [ ] 快捷键
- [ ] 深色主题正确应用
- [ ] 拔掉音频设备启动 → 播放按钮禁用，不崩溃（BUG-001）
- [ ] 保存失败 → 提示但不退出（BUG-001）
- [ ] 进度统计准确（BUG-008）

### 8.3 回归验证

- [ ] 打开原有 MinLabel.ini 配置文件正常加载
- [ ] JSON 标注文件格式与原版完全一致
- [ ] .lab 文件输出与原版一致

---

## 9. 迁移步骤

1. 删除 `gui/PlayWidget.h/.cpp`
2. 修改 `MainWindow.h`: include 改为 `<dstools/PlayWidget.h>`，成员类型改为 `dstools::widgets::PlayWidget *`
3. 修改 `MainWindow.cpp`: 构造函数中 `new dstools::widgets::PlayWidget(this)` 替代 `new PlayWidget(this)`
4. 修复 `saveFile()` 返回类型 + 3 处 exit → return false / continue
5. 修复 CQ-006 拼写（4+2 处）
6. 修复 BUG-008 QDir::count
7. 修复 CQ-004 foreach (Common.cpp 2处)
8. 重写 `main.cpp`（AppInit + Theme）
9. 更新 CMakeLists.txt
10. 编译验证 + 功能测试
