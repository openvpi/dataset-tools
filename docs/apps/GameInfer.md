# GameInfer.exe — 重构实施文档

**Version**: 1.0  
**Date**: 2026-04-26  
**前置**: architecture.md, module-spec.md, apps/DatasetPipeline.md §A「项目规范」

---

## 1. 概述

GameInfer.exe 是**保留的独立 EXE**，用于 GAME 音频转 MIDI 推理。重构变化最小——替换 DmlGpuUtils 为共享 GpuSelector，接入统一样式，修复错误提示。

| 属性 | 值 |
|------|-----|
| 目标名 | `GameInfer` |
| 类型 | GUI EXE (WIN32_EXECUTABLE) |
| 版本 | 0.1.0（从 0.0.0.1 升至 0.1.0） |
| 来源 | 原 `src/apps/GameInfer/` 原地重构 |
| 窗口类型 | QMainWindow（不变） |
| 命名空间 | 无（全局命名空间，保留） |

---

## 2. 源文件清单

### 2.1 保留文件（修改）

| 文件 | 变化 |
|------|------|
| `main.cpp` | 重写：AppInit + Theme（删除手动字体初始化） |
| `gui/MainWindow.h/.cpp` | 无重大变化（或 `tr()` 包裹） |
| `gui/MainWidget.h/.cpp` | 使用 `dstools::widgets::GpuSelector` 替代自有 GPU 枚举逻辑；修复 BUG-012 (3处错误提示) |

### 2.2 删除文件

| 文件 | 原因 |
|------|------|
| `utils/DmlGpuUtils.h` | 被 `dstools-widgets::GpuSelector` 替代 |
| `utils/DmlGpuUtils.cpp` | 同上 |
| `utils/GpuInfo.h` | 被 `dstools::widgets::GpuInfo` 替代 |

> **勘误**: 原 architecture.md 仅提到删除 `DmlGpuUtils.*`，遗漏了 `GpuInfo.h`。该文件定义了 `GpuInfo` 结构体（`index`, `name`, `dedicatedMemory`），已在共享 `GpuSelector.h` 中以 `dstools::widgets::GpuInfo` 重新定义。

### 2.3 新增文件

无。

---

## 3. 依赖关系

### 3.1 CMake 链接

```cmake
target_link_libraries(GameInfer PRIVATE
    dstools-widgets          # GpuSelector + Theme
    game-infer::game-infer   # GAME 推理引擎
    Qt6::Core Qt6::Widgets
)
```

**对比原依赖变化**：

| 原依赖 | 变化 |
|--------|------|
| `game-infer::game-infer` | 保留 |
| `dxgi` (Windows SDK) | **移至 `dstools-widgets`**——GpuSelector 内部使用 DXGI 枚举 GPU，GameInfer 不再直接链接 dxgi |
| `SndFile` (find_package) | **移除**——GameInfer 不直接使用 SndFile（通过 game-infer 间接使用）；原 CMake 中 `find_package(SndFile)` 是冗余的 |

> **勘误**: 原 GameInfer CMakeLists.txt 有 `find_package(SndFile CONFIG REQUIRED)` 但未 link SndFile，仅 `game-infer::game-infer` 间接携带了 SndFile。重构后应删除该冗余 find_package。

### 3.2 Windows SDK 依赖变化

原 CMakeLists.txt 有复杂的 Windows SDK 路径解析逻辑（`$ENV{WindowsSDKVersion}` 正则匹配 + include/lib 目录设置）用于链接 `dxgi`。重构后这些逻辑**移至 `dstools-widgets` 的 CMakeLists.txt**（GpuSelector 编译时需要），GameInfer 自身不再需要。

### 3.3 间接依赖

- `game-infer` → `audio-util` + `wolf-midi` + `nlohmann_json` + `dstools-infer-common` → `onnxruntime`
- `dstools-widgets` → `dstools-audio` → `dstools-core` → Qt6

### 3.4 运行时依赖

| DLL | 用途 |
|-----|------|
| `dstools-widgets.dll` | GpuSelector + Theme |
| `game-infer.dll` | GAME 推理 |
| `audio-util.dll` | 音频处理 |
| `onnxruntime.dll` | ONNX 推理 |
| Qt6 DLLs | UI |
| `sndfile.dll` | WAV I/O（通过 game-infer） |

---

## 4. 关键设计决策

### 4.1 GpuSelector 替换

原 `MainWidget.cpp` 中直接调用 `DmlGpuUtils::enumerateGpus()` 填充 QComboBox。替换为：

```cpp
// 原代码 (MainWidget.cpp):
#include "utils/DmlGpuUtils.h"
// ... 构造函数中:
auto gpus = DmlGpuUtils::enumerateGpus();
for (const auto &gpu : gpus) {
    m_gpuComboBox->addItem(
        QString("%1 (%2 MB)").arg(gpu.name).arg(gpu.dedicatedMemory / 1024 / 1024),
        gpu.index);
}

// 新代码:
#include <dstools/GpuSelector.h>
// ... 构造函数中:
m_gpuSelector = new dstools::widgets::GpuSelector(this);
// GpuSelector 自动枚举并填充，无需手动添加 item

// 获取选中设备 ID:
int deviceId = m_gpuSelector->selectedDeviceId();
```

**接口映射**:

| 原用法 | 新用法 |
|--------|--------|
| `DmlGpuUtils::enumerateGpus()` → 手动填充 QComboBox | `GpuSelector` 自动填充 |
| `comboBox->currentData().toInt()` 获取 device ID | `m_gpuSelector->selectedDeviceId()` |
| `GpuInfo { index, name, dedicatedMemory }` (utils/GpuInfo.h) | `dstools::widgets::GpuInfo` (相同字段) |

### 4.2 ExecutionProvider 适配

原 GameInfer 使用 `Game::ExecutionProvider`（定义在 game-infer 库中）。重构后 game-infer 统一使用 `dstools::infer::ExecutionProvider`。

MainWidget 中的适配：

```cpp
// 原:
Game::ExecutionProvider provider;
if (m_cpuRadio->isChecked()) provider = Game::ExecutionProvider::CPU;
else if (m_dmlRadio->isChecked()) provider = Game::ExecutionProvider::DML;

// 新（若 game-infer 内 using dstools::infer::ExecutionProvider）:
dstools::infer::ExecutionProvider provider;
if (m_cpuRadio->isChecked()) provider = dstools::infer::ExecutionProvider::CPU;
else if (m_dmlRadio->isChecked()) provider = dstools::infer::ExecutionProvider::DML;
```

> 若 game-infer 内部已做 `using`，则 MainWidget 无需改动。具体取决于 game-infer 库的重构方式。

### 4.3 保持 QMainWindow

GameInfer 界面简单（单文件选择 + 参数 + 推理按钮 + 输出），但有自己的菜单栏（About/About Qt），保持 QMainWindow。

---

## 5. Bug 修复清单

| Bug ID | 严重度 | 修复位置 | 简述 |
|--------|--------|----------|------|
| BUG-012 | Low | `gui/MainWidget.cpp` | 3 处修复：(1) "is not exists" → "does not exist" (2) `.arg(text.toLocal8Bit().toStdString())` → `.arg(text)` (3) `QMessageBox::information` → `QMessageBox::critical` |

GameInfer 范围内的 Bug 很少，因为它是最简单的 EXE。

---

## 6. CMakeLists.txt

```cmake
# src/apps/GameInfer/CMakeLists.txt

set(GAMEINFER_VERSION 0.1.0.0)

add_executable(GameInfer WIN32
    main.cpp
    gui/MainWindow.h    gui/MainWindow.cpp
    gui/MainWidget.h    gui/MainWidget.cpp
    # 注意: utils/DmlGpuUtils.* 和 utils/GpuInfo.h 已删除
)

if(APPLE)
    set_target_properties(GameInfer PROPERTIES MACOSX_BUNDLE TRUE)
endif()

target_link_libraries(GameInfer PRIVATE
    dstools-widgets
    game-infer::game-infer
    Qt6::Core Qt6::Widgets
)

# 注意: 不再需要 Windows SDK dxgi 链接（已移至 dstools-widgets）
# 注意: 不再需要 find_package(SndFile)（原冗余依赖）

target_compile_definitions(GameInfer PRIVATE
    APP_VERSION="${GAMEINFER_VERSION}"
)

if(WIN32)
    include(${PROJECT_SOURCE_DIR}/cmake/winrc.cmake)
    generate_win_rc(GameInfer
        VERSION ${GAMEINFER_VERSION}
        DESCRIPTION "GameInfer - Audio to MIDI Inference"
    )
endif()

add_dependencies(DeployedTargets GameInfer)
```

**对比原 CMakeLists (53行) 变化**：

| 变化 | 说明 |
|------|------|
| 删除 `GLOB_RECURSE` | 改为显式列出源文件（避免构建意外） |
| 删除 `find_package(SndFile)` | 冗余，GameInfer 不直接使用 |
| 删除 Windows SDK 解析逻辑 (15行) | 移至 dstools-widgets |
| 删除 `dxgi` 链接 | 移至 dstools-widgets |
| 新增 `dstools-widgets` | GpuSelector + Theme |
| 删除 `utils/` 源文件 | DmlGpuUtils/GpuInfo 已被共享组件替代 |

---

## 7. 配置文件

文件: `config/GameInfer.ini`（**新建**——原 GameInfer 无持久化配置）

```ini
[General]
theme=dark

[Inference]
modelPath=
provider=CPU
deviceId=0
lastWavPath=
lastOutputDir=
```

> **勘误**: 原方案中 architecture.md 提到 GameInfer.ini 但未说明这是新建的。原 GameInfer 代码中没有任何 QSettings 使用。此配置文件是重构新增的便利功能。

---

## 8. 验证清单

### 8.1 构建验证

- [ ] `GameInfer.exe` 编译零 warning
- [ ] 不再直接链接 dxgi
- [ ] 链接 `dstools-widgets.dll` + `game-infer.dll` 成功

### 8.2 功能验证

- [ ] GAME 模型加载
- [ ] 音频文件选择
- [ ] CPU 推理 → MIDI 输出正确
- [ ] DML/CUDA GPU 推理（如可用）
- [ ] GPU 选择（GpuSelector 自动枚举）
- [ ] 错误提示正确（BUG-012：语法/类型/级别全部修复）
- [ ] 深色主题正确应用
- [ ] 不存在的音频路径 → "Audio file does not exist" (critical 对话框)

### 8.3 回归验证

- [ ] MIDI 输出与原版逐字节一致（相同输入）
- [ ] GPU 枚举结果与原版一致

---

## 9. 迁移步骤

1. 删除 `utils/DmlGpuUtils.h/.cpp` 和 `utils/GpuInfo.h`
2. 修改 `gui/MainWidget.h/.cpp`: include 改为 `<dstools/GpuSelector.h>`，用 GpuSelector 替代手动 GPU 枚举
3. 修复 BUG-012 (MainWidget.cpp 3处)
4. 如需适配 `dstools::infer::ExecutionProvider`，修改枚举引用
5. 重写 `main.cpp`（AppInit + Theme）
6. 更新 CMakeLists.txt（删除 dxgi/SndFile/GLOB_RECURSE，添加 dstools-widgets）
7. 编译验证 + 功能测试
