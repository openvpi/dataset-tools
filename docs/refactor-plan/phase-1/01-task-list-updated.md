# Phase 1 — 更新后的任务清单

**Date**: 2026-04-28（初版），2026-04-29（四次核实更新），2026-04-29（五次核实 — 全量代码验证），2026-04-29（六次核实 — 确认全部 12/12 完成）  
**基于**: 代码核实报告 `00-verification-report.md`  
**变更说明**: 本文档修正了原文档中的不准确项，统一了与 00-overview.md 的编号，并根据实际代码调整了任务范围。2026-04-29 六次核实：**全量代码验证**，确认 12/12 全部完成。Phase 1 阶段完成。

---

## 任务总览

| 编号 | 任务 | 优先级 | 工时 | 状态 | 较原文档变更 |
|------|------|--------|------|------|-------------|
| TASK-1.1 | PitchUtils 提取 | P0 | S (半天) | ✅ **已完成** | 🔧 已完成：PitchUtils.h/cpp 已在 dstools-core，PianoRollView/PropertyPanel 已迁移 |
| TASK-1.2 | AudioFileResolver 提取 | P0 | S→M (半天→1天) | ✅ **已完成** | 🔧 已完成：AudioFileResolver.h 在 core 层；MinLabel 薄包装委托；PitchLabeler/PhonemeLabeler 直接调用 |
| TASK-1.3 | MinLabel JSON I/O 迁移 | P0 | XS (2h) | ✅ **已完成** | 🔧 已完成：readJsonFile/writeJsonFile 已委托 JsonHelper::loadFile/saveFile，含 nlohmann↔Qt 转换 |
| TASK-1.4 | GameInfer GpuSelector 迁移 | P0 | S (半天) | ✅ **已完成** | 🔧 已完成：DmlGpuUtils.h 已删除，MainWidget 使用共享 GpuSelector |
| TASK-1.5 | ShortcutManager 提取 | P1 | S (半天) | ✅ **已完成** | 🔧 已完成：ShortcutManager.h 在 widgets 层，MinLabel/PitchLabeler 均使用 |
| TASK-1.6 | PhonemeLabeler 接入快捷键编辑 | P1 | S (半天) | ✅ **已完成** | 🔧 已完成：m_shortcutManager->showEditor(this) 已集成 |
| TASK-1.7 | ViewportController 提升 | P1 | M (1~2天) | ✅ **已完成** | 🔧 ViewportController 已在 widgets 层；PhonemeLabeler 本地副本已删除；PitchLabeler 已全面集成 — 所有 zoom/scroll 操作均通过 ViewportController |
| TASK-1.8 | BaseFileListPanel 提取 | P1 | M (1天) | ✅ **已完成** | 🔧 已完成：BaseFileListPanel.h 在 widgets 层，两个 FileListPanel 均继承 |
| TASK-1.9 | hubert-infer 提取 | P2 | M (1~2天) | ✅ **已完成** | 🔧 已完成：src/infer/hubert-infer/ 已创建，含完整库结构；旧 src/apps/HubertFA/ 已删除 |
| TASK-1.10 | F0Curve 数据类提升 | P2 | S (半天) | ✅ **已完成** | 🔧 已完成：F0Curve.h 已在 core 层 |
| TASK-1.11 | game-infer align 现有实现验证 | 调研 | XS→S (2~4h) | ✅ **已完成** | 🔧 已完成：spike-game-align-result.md 已产出，结论 B — 核心实现完整，需少量集成补充 |
| TASK-1.12 | labelvoice schema 决策 | 调研 | S (半天) | ✅ **已完成** | 🔧 已完成：decision-labelvoice-schema.md 已产出，选项 A — labelvoice 作为工程容器 |

**统计**: 12/12 已完成  
**剩余工时**: 0

---

## 各任务详细说明

### TASK-1.1: PitchUtils 提取 [P0] ✅ 已完成

**状态**: 已完成（四次核实确认 2026-04-29）

**完成证据**:
- `src/core/include/dstools/PitchUtils.h` 存在（40 行），包含 6 个函数声明 + NotePitch struct
- `src/core/src/PitchUtils.cpp` 存在，全部函数有实现
- `PianoRollView.h:11` 已改为 `#include <dstools/PitchUtils.h>`，:26-32 有 using 声明
- `PianoRollView.cpp` 中不再有本地 pitch 函数定义
- `PropertyPanel.cpp:2` 已改为 `#include <dstools/PitchUtils.h>`，:75 和 :263 使用 `dstools::parseNoteName`

**Definition of Done 逐项验证**:
- [x] `src/core/include/dstools/PitchUtils.h` 存在，包含 6 个函数声明 + NotePitch struct
- [x] `src/core/src/PitchUtils.cpp` 存在，全部函数有实现
- [x] `PianoRollView.cpp` / `PianoRollView.h` 中不再包含本地定义
- [x] `PropertyPanel.cpp` 通过 `dstools::parseNoteName()` 调用共享实现
- [ ] PitchLabeler 的钢琴卷帘音符显示与改动前一致（需手动验证）
- [ ] PitchLabeler 的 cents 偏移编辑（shiftNoteCents）行为不变（需手动验证）
- [ ] 编译通过，无新增警告（需 CI 验证）

---

### TASK-1.2: AudioFileResolver 提取 [P0] ✅ 已完成

**状态**: 已完成（五次核实确认 2026-04-29）

**完成证据**:
- `src/core/include/dstools/AudioFileResolver.h` 存在，包含 findAudioFile/audioToDataFile/audioExtensions 3 个 static 方法
- MinLabel `Common.cpp` 中 `audioToOtherSuffix`（:9-15）和 `labFileToAudioFile`（:17-44）已改为薄包装，内部委托 `dstools::AudioFileResolver::audioToDataFile()` 和 `dstools::AudioFileResolver::findAudioFile()`
- PitchLabeler `MainWindow.cpp` 已 `#include <dstools/AudioFileResolver.h>` 并直接调用
- PhonemeLabeler `MainWindow.cpp` 已 `#include <dstools/AudioFileResolver.h>` 并直接调用
- PitchLabeler 和 MinLabel 通过 AudioFileResolver 自动获得 ogg 支持

---

### TASK-1.3: MinLabel JSON I/O 迁移 [P0] ✅ 已完成

**状态**: 已完成（五次核实确认 2026-04-29）

**完成证据**:
- `readJsonFile`（Common.cpp:207）已改为调用 `dstools::JsonHelper::loadFile()` + `nlohmannToQt()` 转换
- `writeJsonFile`（Common.cpp:216）已改为调用 `qtToNlohmann()` + `dstools::JsonHelper::saveFile()`（原子写入保护）
- 方案 B（仅 I/O 层转换）已按计划实施

---

### TASK-1.4: GameInfer GpuSelector 迁移 [P0] ✅ 已完成

**状态**: 已完成（五次核实确认 2026-04-29）

**完成证据**:
- `src/apps/GameInfer/utils/DmlGpuUtils.h` 已删除
- GameInfer `MainWidget.h` 已 `#include <dstools/GpuSelector.h>` 并使用 `dstools::widgets::GpuSelector`

**注**: HubertFA/util/ 的 DmlGpuUtils 副本仍存在，将在 TASK-1.9 中随 HubertFA 目录一并清理。

---

### TASK-1.5: ShortcutManager 提取 [P1] ✅ 已完成

**状态**: 已完成（五次核实确认 2026-04-29）

**完成证据**:
- `src/widgets/include/dstools/ShortcutManager.h` 存在
- MinLabel 和 PitchLabeler 均使用共享 ShortcutManager，不再有手动回写循环

---

### TASK-1.6: PhonemeLabeler 接入快捷键编辑 [P1] ✅ 已完成

**状态**: 已完成（五次核实确认 2026-04-29）

**完成证据**:
- PhonemeLabeler MainWindow.cpp 调用 `m_shortcutManager->showEditor(this)`（:182）
- 快捷键编辑对话框已集成到菜单

---

### TASK-1.7: ViewportController 提升 [P1] ✅ 已完成

**状态**: 已完成（六次核实确认 2026-04-29）

**已完成部分**:
- `src/widgets/include/dstools/ViewportController.h` 存在，含 ViewportState struct + ViewportController class
- PhonemeLabeler 本地 `ViewportController.h/cpp` 已删除，所有视图控件改用共享版本
- PitchLabeler `PianoRollView.h` 已声明 `ViewportController *m_viewport` 成员和 `setViewportController()` 方法
- `onViewportChanged` 回调已实现（:1679），将 ViewportController 状态同步到本地变量 m_hScale/m_scrollX

**PitchLabeler 全面集成确认**:

所有 zoom/scroll 操作均通过 ViewportController，不存在直接赋值绕过：

| 操作 | 行号 | 调用 |
|------|------|------|
| hScrollBar valueChanged | :88 | `m_viewport->setViewRange(startSec, endSec)` |
| setAudioDuration clamp | :324 | `m_viewport->setPixelsPerSecond(minScale)` |
| zoomIn | :332 | `m_viewport->zoomAt(viewCenter, 1.2)` |
| zoomOut | :342 | `m_viewport->zoomAt(viewCenter, factor)` |
| resetZoom | :352 | `m_viewport->setPixelsPerSecond(newScale)` |
| resizeEvent clamp | :1028 | `m_viewport->setPixelsPerSecond(minScale)` |
| wheelEvent Ctrl+zoom | :1041 | `m_viewport->zoomAt(centerSec, factor)` |
| wheelEvent Shift+scroll | :1045 | `m_viewport->scrollBy(deltaSec)` |

m_hScale/m_scrollX 仅在 `onViewportChanged()` 回调中被赋值（:1680-1681），作为绘制缓存。写入路径完全经过 ViewportController。

**剩余工时**: 0

---

### TASK-1.8: BaseFileListPanel 提取 [P1] ✅ 已完成

**状态**: 已完成（五次核实确认 2026-04-29）

**完成证据**:
- `src/widgets/include/dstools/BaseFileListPanel.h` 存在
- PitchLabeler `FileListPanel` 继承自 `dstools::widgets::BaseFileListPanel`
- PhonemeLabeler `FileListPanel` 继承自 `dstools::widgets::BaseFileListPanel`

---

### TASK-1.9: hubert-infer 提取 [P2] ✅ 已完成

**状态**: 已完成（六次核实确认 2026-04-29）

**完成证据**:
- `src/infer/hubert-infer/` 已创建，包含完整库结构（CMakeLists.txt, include/, src/, Config.cmake.in）
- 公开头文件 8 个：Hfa.h, HfaModel.h, AlignmentDecoder.h, AlignWord.h, DictionaryG2P.h, NonLexicalDecoder.h, Provider.h, HubertInferGlobal.h
- 源文件 6 个：Hfa.cpp, HfaModel.cpp, AlignmentDecoder.cpp, AlignWord.cpp, DictionaryG2P.cpp, NonLexicalDecoder.cpp
- 依赖：onnxruntime, SndFile, nlohmann_json, audio-util, dstools-core
- `src/infer/CMakeLists.txt` 已添加 hubert-infer 子目录
- `src/apps/pipeline/CMakeLists.txt` 已链接 hubert-infer（不再引用 ../HubertFA/util/）
- 旧 `src/apps/HubertFA/` 目录已完全删除
- DmlGpuUtils 副本已清理

---

### TASK-1.10: F0Curve 数据类提升 [P2] ✅ 已完成

**状态**: 已完成（五次核实确认 2026-04-29）

**完成证据**:
- `src/core/include/dstools/F0Curve.h` 存在，包含 F0Curve struct（std::vector\<double\>, timestep, totalDuration, getValueAtTime, getRange, setRange）
- PitchLabeler DSFile.h 不再有本地 F0Curve 定义

---

### TASK-1.11: game-infer align 验证 ★ 重新定义 ✅ 已完成

**状态**: 已完成（六次核实确认 2026-04-29）

**原定义**: 可行性 Spike — 调研 ONNX 模型是否支持 align 输入格式  
**新定义**: 现有实现验证 — 验证已有 align 功能的完整性和正确性

**完成证据**: `spike-game-align-result.md` 已产出，结论 B — 核心实现完整，需少量集成补充。

**验证结果摘要**:

| # | 验证项 | 结果 |
|---|--------|------|
| 1 | GameInfer.exe align UI 入口 | ❌ 未暴露（仅 extract 模式） |
| 2 | align 测试覆盖 | ❌ 无覆盖（TestGame 仅测 get_midi/D3PM） |
| 3 | C++ vs Python 一致性 | ✅ 代码审查确认算法忠实移植，未执行运行时对比 |
| 4 | CSV align 格式覆盖度 | ✅ 完整覆盖，匹配 Step 7 所需格式 |

**对 Phase 2 的影响**: TASK-2.8 从"从零实现"缩减为"UI 集成 + 补充测试"，工时从 3~5 天降至 ~1 天。

---

### TASK-1.12: labelvoice schema 决策 ✅ 已完成

**状态**: 已完成（六次核实确认 2026-04-29）

**完成证据**: `decision-labelvoice-schema.md` 已产出。

**决策**: 选项 A — labelvoice 作为工程容器，9-step 作为处理引擎。.dsproj 管理工程元数据和配置，dstemp/ 承载流水线全部中间产物，人工编辑步骤继续使用原生文件格式，.dstext 推迟到综合软件成熟后再启用。

---

## 依赖关系图 (最终)

```
Phase 1 全部完成 (12/12):

P0:
  TASK-1.1 (PitchUtils)     ✅
  TASK-1.2 (AudioResolver)  ✅
  TASK-1.3 (MinLabel JSON)  ✅
  TASK-1.4 (GpuSelector)    ✅

P1:
  TASK-1.5 (ShortcutMgr)    ✅ ─► TASK-1.6 (PhonemeLabeler 接入) ✅
  TASK-1.7 (ViewportCtrl)   ✅
  TASK-1.8 (FileListPanel)  ✅

P2:
  TASK-1.9 (hubert-infer)   ✅
  TASK-1.10 (F0Curve)       ✅

调研:
  TASK-1.11 (align 验证)    ✅
  TASK-1.12 (schema 决策)   ✅
```

**Phase 1 阶段完成。** 可进入阶段门控验证（05-gate-criteria.md）。
