# 编译单元整合方案

**日期**: 2026-05-04
**目标**: 合并不必要的过度拆分，减少 ~25 个编译单元，提升可读性
**原则**: 同一概念的代码放同一文件；<40 行 .cpp 必须有理由独立存在；合并后单文件不超 400 行

---

## 现状

| 指标 | 数量 |
|---|---|
| .cpp 编译单元 (产品) | 193 |
| .cpp 编译单元 (测试) | 39 |
| <50 行的 .cpp 文件 | 51 |
| <30 行的 .cpp 文件 | 24 |

---

## 合并方案

### Merge 1: phoneme-editor/commands → BoundaryCommands

**理由**: 5 个 Command 类全部操作 boundary，职责相同，合并后 "打开一个文件就能看到所有边界操作"。

| 原文件 | 行数 | 合并后 |
|---|---|---|
| `InsertBoundaryCommand.h/cpp` | 30+40 | → `BoundaryCommands.h` |
| `RemoveBoundaryCommand.h/cpp` | 30+27 | → `BoundaryCommands.cpp` |
| `MoveBoundaryCommand.h/cpp` | 33+37 | |
| `LinkedMoveBoundaryCommand.h/cpp` | 24+26 | |
| `SetIntervalTextCommand.h/cpp` | 48+36 | |

**减少**: 4 个 .cpp，4 个 .h → 各 1 个（合并后 ~170 行 .cpp, ~150 行 .h）

---

### Merge 2: framework/core 注册表 + 微型类

| 合并组 | 原文件 | 行数 | 合并后 |
|---|---|---|---|
| 服务基础设施 | `ServiceLocator.cpp`(30) + `FormatAdapterRegistry.cpp`(30) | 60 | `ServiceLocator.cpp`（保留名，追加 Registry 实现） |
| 任务注册 | `TaskProcessorRegistry.cpp`(43) + `PassthroughProcessor.cpp`(25) | 68 | `TaskProcessorRegistry.cpp`（Passthrough 是自注册到 Registry 的，放一起） |
| 异步任务 | `AsyncTask.cpp`(19) | 19 | 方法内联到 `AsyncTask.h`（仅 3 个短方法） |

**减少**: 2 个 .cpp

---

### Merge 3: domain/src 微型文件

| 合并组 | 原文件 | 行数 | 合并后 |
|---|---|---|---|
| Pitch 数学 | `F0Curve.cpp`(39) + `PitchUtils.cpp`(84) | 123 | `PitchMath.cpp` |
| 路径解析 | `AudioFileResolver.cpp`(42) + `ProjectPaths.cpp`(48) | 90 | `PathResolvers.cpp` |
| Stub 适配器 | `DsDocumentAdapter.cpp`(24) + `ModelDownloader.cpp`(25) | 49 | `DomainAdapters.cpp` |
| 轻量格式适配器 | `FormatAdapterInit.cpp`(21) + `TextGridAdapter.cpp`(41) + `LabAdapter.cpp`(47) | 109 | `SimpleAdapters.cpp`（Init 注册函数 + 两个最简 adapter） |

**减少**: 5 个 .cpp

注: `IEditorDataSource.cpp`（3 行，仅触发 MOC）保留不动——这是 Qt AUTOMOC 对纯接口头文件的标准做法，删除会导致 MOC 缺失。

---

### Merge 4: libs/lyric-fa 按功能链归并

| 合并后文件 | 包含原文件 | 原行数 | 理由 |
|---|---|---|---|
| `AsrPipeline.cpp` | `Asr.cpp`(81) + `AsrTask.cpp`(31) + `FunAsrAdapter.cpp`(40) + `FunAsrProcessor.cpp`(55) | 207 | ASR 调用链：Adapter→Processor→Task→Asr |
| `AsrPipeline.h` | `Asr.h` + `AsrTask.h` + `FunAsrAdapter.h` + `FunAsrProcessor.h` | — | 对应头文件合并 |
| `LyricAlignment.cpp` | `LyricMatcher.cpp`(47) + `LyricMatchTask.cpp`(12) + `SmartHighlighter.cpp`(50) + `ChineseProcessor.cpp`(31) | 140 | 歌词匹配链：中文处理→匹配→高亮→Task |
| `LyricAlignment.h` | `LyricMatcher.h` + `LyricMatchTask.h` + `SmartHighlighter.h` + `ChineseProcessor.h` | — | 对应头文件合并 |
| `SequenceAligner.cpp/h` | 保持不变 (335 行) | — | 独立算法 |
| `MatchLyric.cpp/h` | 保持不变 (80 行) | — | 对外接口 |
| `Utils.cpp/h` | 保持不变 (66 行) | — | 工具函数 |

**减少**: 6 个 .cpp（11 → 5）

---

### Merge 5: framework/audio WaveFormat 内联

`WaveFormat.cpp`（12 行，6 个单行 getter）→ 全部 `inline` 到 `WaveFormat.h`。

**减少**: 1 个 .cpp

---

### Merge 6: framework/widgets 简单对话框

| 原文件 | 行数 | 合并后 |
|---|---|---|
| `SettingsDialog.cpp` | 35 | → `SimpleDialogs.cpp` |
| `ProgressDialog.cpp` | 45 | |
| `RunProgressRow.cpp` | 42 | |

**减少**: 2 个 .cpp（合并后 ~122 行）

对应头文件保持独立——它们声明不同的类，分开的头文件有利于按需 include。

---

### Merge 7: ~~waveform-panel PlaybackController 并入 WaveformPanel~~ ✅ 已完成

> waveform-panel 已从构建中删除，此合并项不再适用。

**减少**: 1 个 .cpp（随 waveform-panel 整体删除）

---

### Merge 8: data-sources 微型文件

| 原文件 | 行数 | 处理 |
|---|---|---|
| `AudioFileListPanel.cpp` | 16（仅构造函数设置 filter） | 内联到 `.h`，删除 .cpp |
| `ModelConfigHelper.cpp` | 19（仅一个自由函数） | 内联到 `.h`，删除 .cpp |
| `AudacityMarkerIO.cpp` | 49 | 保持不变（有 file I/O，不适合内联） |

**减少**: 2 个 .cpp

---

## 汇总

| 合并项 | 减少 .cpp | 改动复杂度 |
|---|---|---|
| Merge 1: phoneme-editor/commands | 4 | 低 |
| Merge 2: framework/core 注册表 | 2 | 低 |
| Merge 3: domain/src 微型文件 | 5 | 低 |
| Merge 4: libs/lyric-fa | 6 | 中 |
| Merge 5: framework/audio WaveFormat | 1 | 极低 |
| Merge 6: framework/widgets 对话框 | 2 | 低 |
| Merge 7: waveform-panel PlaybackController | 1 | 低 |
| Merge 8: data-sources 微型文件 | 2 | 低 |
| **合计** | **23** | — |

合并后产品编译单元: 193 → ~170
