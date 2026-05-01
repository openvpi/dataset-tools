# 架构重构路线图

> 基于 2026-05-02 代码审计更新，仅保留待办事项。
>
> **原则**: 功能齐全、简单可靠、不过度设计。接口保持一致性和合理的扩展预留。
>
> **范围**: 在本仓库内完成。所有模块作为 CMake 子目录共存。
>
> **C++ 标准**: 允许 C++20（当前 C++20，已有 `std::filesystem` 使用）。

---

## 已完成阶段摘要

| Phase | 名称 | 主要成果 |
|-------|------|---------|
| 0 | 预备工作 | ModelType/DocumentFormat 泛化，Qt 依赖分离，pipeline 子模块库化 |
| 1 | 核心分离 | ModelManager→domain，dsfw-base 创建，OnnxModelBase，IInferenceEngine |
| 2 | 库边界固化 | 目录重排，版本化 API (DSFW_VERSION)，FunASR 适配器，pipeline 后端提取 |
| 3 | 框架增强 | Logger，Undo/Redo (ICommand+UndoStack)，EventBus，CLI 工具 (dstools-cli) |
| 4 | 完善与扩展 | PluginManager，CrashHandler，UpdateChecker，RecentFiles，WidgetGallery，新标准控件 |
| 5-6 | 深度优化 | Result\<T\> 统一，UI 推理解耦，Slicer 合并，MinLabelService 提取，StringUtils 提取，CI 矩阵构建，clang-tidy CI，CHANGELOG，线程安全验证，冗余依赖清理 |

> 所有已修复 Bug/架构债/遗留缺陷不再列出。

---

## 架构决策记录 (ADR)

| ADR | 决策 | 理由 |
|-----|------|------|
| ADR-1 | dsfw-base 为 Qt-free 静态库 | 支持 CLI 工具和非 Qt 消费者 |
| ADR-2 | ModelType 改为 int ID 注册表 | 框架层不感知业务模型类型 |
| ADR-3 | OnnxModelBase protected 继承 | 各推理 DLL 保持现有 API 稳定 |
| ADR-4 | audio-util 保持应用层 | 音频 DSP 过于专业化 |
| ADR-5 | dsfw-audio (FFmpeg/SDL2) 归框架 | 音频播放/解码是通用桌面能力 |
| ADR-6 | Undo/Redo 迁移 opt-in | 现有 app 按各自节奏迁移 |
| ADR-7 | FunASR 永远不直接修改 | vendor 代码只用适配器包装 |
| ADR-8 | 单仓库模式 | 降低维护复杂度 |
| ADR-9 | 允许 C++20 | 编译器均已支持，按需使用新特性，不强制全面迁移 |
| ADR-21 | 替换 QBreakpad，统一用 dsfw::CrashHandler | QBreakpad 为外部依赖且 dsfw::CrashHandler 已存在但未接入；修复后统一使用，减少依赖 |
| ADR-22 | config/logs/dumps 迁移到 QStandardPaths::AppDataLocation | applicationDirPath() 在 macOS .app bundle 和 Linux /usr/bin 安装下可能只读 |
| ADR-23 | 撤销重做直接使用 QUndoStack，不封装框架抽象 | Qt QUndoStack 已成熟，旧 dsfw::UndoStack 零消费者已删除；QUndoCommand 子类是纯业务逻辑无法泛化 |

---

## Phase B — 测试与质量 (P2)

### B.1 补齐领域模块单元测试 — P2, L (1-3d) — ✅ 已完成

24 个测试模块 (80+ 用例)：domain 10 + framework 12 + libs 1 + widgets 1。原待测的 CsvToDsConverter、TextGridToCsv、PitchProcessor、TranscriptionPipeline 均已补齐。

**验收标准**:
- [x] 4 个模块有单元测试
- [x] 测试样本数据放入 `src/tests/data/`

---

### B.2 TODO/FIXME 清理 — P3, S (<2h) — ✅ 已完成

应用代码中的 TODO/FIXME 已全部清理。

---

### B.3 文件操作错误处理补全 — P3, S (<2h)

搜索 `file.open()` 无 else 分支的代码点，补充错误处理。已有 `Result<T>` 基础设施，直接用即可。

---

## Phase C — 代码质量 (P3)

### C.1 大文件拆分 — P3, L (1-3d)

只拆真正有维护痛点的：

| 文件 | 行数 | 拆分方向 |
|------|------|---------|
| PitchLabelerPage.cpp | 755 | 文件 I/O → PitchFileService |
| PhonemeLabelerPage.cpp | 600 | 波形渲染 → WaveformRenderer |

其余 400-600 行的文件在 C++ 中属于正常范围，有痛点时再处理。

---

### C.2 魔法数字常量化 — P3, S (<2h)

重复出现的提取为常量，一次性出现的不动：

| 值 | 出现次数 | 目标 |
|------|---------|------|
| `4096` (音频 buffer) | 3 处 | `AudioConstants::kDefaultBufferSize` |
| 窗口尺寸 `1280×720` / `1200×800` | 2 处 | 各 app main.cpp 中就地定义 `constexpr` |

Blackman-Harris 系数、hop size 等仅一处使用的，就地加注释即可。

---

## Phase D — CI/CD (P2-P3)

### D.1 框架模块独立编译 CI 验证 — P2, M (2-8h) — ✅ 已完成

dsfw-core / dsfw-ui-core 已有 `find_package` guards，`.github/workflows/verify-modules.yml` 已创建。

---

### D.2 Doxygen CI — P3, S (<2h) — ✅ 已完成

Doxyfile 已配置，32 个框架头文件已有 Doxygen 注释。`.github/workflows/docs.yml` 已创建。

---

### D.3 跨平台包分发 — P3, L (1-3d) — ✅ 已完成

`.github/workflows/release.yml` 已创建，监听 `v*` tag：
- Windows: 便携版 ZIP
- macOS: DMG
- Linux: AppImage

---

## Phase H — 用户体验与可靠性 (P1.5)

> **框架 vs 应用边界**：
>
> - **H.1 撤销重做**：两个 app 均直接使用 `QUndoStack`（Qt 内置），不再封装框架层抽象（旧 `dsfw::UndoStack`/`dsfw::ICommand` 已在 G.1 中作为死代码删除）。`QUndoCommand` 子类是纯业务逻辑，归应用层。框架不介入。
> - **H.2 文件日志**：`FileLogSink` 归 `dsfw-core`（与 `Logger` 同层），`AppInit` 负责注册。通用能力，所有应用自动受益。
> - **H.3 批量 Checkpoint**：`BatchCheckpoint` 归 `dsfw-core`（与 `TaskTypes`/`ITaskProcessor` 同层），processBatch 默认实现自动接入。通用能力。
> - **H.4 CrashHandler**：已在 `dsfw-core`，修复后由 `AppInit` 统一激活。通用能力。
> - **H.5 AppPaths**：归 `dsfw-core`，集中管理跨平台数据路径。通用能力。

### H.1 PitchLabeler 撤销重做补全 — P1.5, M (4-8h)

> **不纳入框架**。`QUndoStack` 是 Qt 成熟的撤销/重做机制，PhonemeLabeler 和 PitchLabeler 均已直接使用。旧 `dsfw::UndoStack` 包装层在 G.1 中已删除（零消费者）。`QUndoCommand` 子类是纯领域逻辑（音符删除、滑音切换等），不存在可复用的框架抽象。

现状：PitchLabeler 已有 `QUndoStack` + `PitchMoveCommand`（音符音高拖动）+ `ModulationDriftCommand`（颤音/偏移 F0 调整）。但以下 5 个编辑操作直接修改 `DSFile` 并调用 `markModified()`，绕过了 UndoStack，不可撤销：

| 操作 | 信号来源 | 需要新建的 Command |
|------|---------|-------------------|
| 删除音符 | `noteDeleteRequested` | `DeleteNotesCommand` |
| 修改滑音类型 | `noteGlideChanged` | `SetNoteGlideCommand` |
| 切换连音标记 | `noteSlurToggled` | `ToggleNoteSlurCommand` |
| 切换休止符 | `noteRestToggled` | `ToggleNoteRestCommand` |
| 合并到左邻音符 | `noteMergeLeft` | `MergeNoteLeftCommand` |

另外 `PianoRollInputHandler.cpp:305` 和 `:385` 也有直接 `markModified()` 调用，需确认是否属于已有 Command 覆盖的 F0 编辑路径，如果不是则需补充。

**实现方式**：参照现有 `PhonemeLabeler/gui/ui/commands/` 模式（InsertBoundaryCommand 等），在 `PitchLabeler/gui/ui/commands/` 下新建 Command 类。每个 Command 在构造时快照受影响音符的旧状态，`redo()` 执行操作，`undo()` 恢复快照。

**注意事项**：
- `DeleteNotesCommand` 和 `MergeNoteLeftCommand` 涉及音符数量变化，undo 时需恢复音符并调用 `recomputeNoteStarts()`
- `ToggleNoteRestCommand` 的 undo 需要记录原始 `note.name`（因为转为 rest 后原 name 丢失）
- 所有 Command 的 `redo()`/`undo()` 末尾需调用 `markModified()` + 通知 PianoRollView 刷新

**验收标准**:
- [ ] 5+ 个操作均通过 `m_undoStack->push(new XxxCommand(...))` 执行
- [ ] 每个操作可 Ctrl+Z 撤销、Ctrl+Shift+Z 重做
- [ ] 撤销/重做后 PianoRoll 显示正确、文件脏标记正确
- [ ] PitchLabelerPage::connectSignals() 中的 5 个 lambda 替换为 push Command
- [ ] PianoRollInputHandler 中的直接 markModified() 调用已审计并补全

---

### H.2 全局文件日志 Sink — P1.5, S (2-4h) — `dsfw-core` 框架模块

> **纳入框架**。`FileLogSink` 是 `Logger` 的配套设施，与 `Log.h` 同属 `dsfw-core`。所有应用通过 `AppInit` 自动注册，无需各 app 重复代码。

现状：`Logger` 已有 pluggable sink 机制（`addSink()`），`AppInit::init()` 中未注册文件 sink。崩溃时只有 minidump，没有应用级别日志。

**需求**：所有应用启动时自动注册一个文件日志 sink，将日志写入 `<AppDataLocation>/logs/<app_name>_<date>.log`（参见 H.5 路径迁移）。

| # | 任务 | 涉及文件 |
|---|------|---------|
| 1 | 在 `dsfw-core` 新增 `FileLogSink` 工厂函数（接受路径，返回 `LogSink`） | `Log.h`, 新建 `FileLogSink.h/.cpp` 或直接加到 `Log.cpp` |
| 2 | `AppInit::init()` 中创建日志目录并注册 `FileLogSink` | `AppInit.cpp` |
| 3 | 日志自动轮转：保留最近 N 天（默认 7 天）的日志文件，启动时清理过期文件 | `AppInit.cpp` |
| 4 | 崩溃 callback 中将最后 N 条日志附加到 dump 目录 | `AppInit.cpp` CrashHandler 回调 |

**验收标准**:
- [ ] 任意应用启动后 logs 目录下生成日志文件
- [ ] 日志包含时间戳、级别、分类、消息
- [ ] 超过 7 天的日志文件在启动时自动删除
- [ ] 崩溃时 dump 目录下有对应的日志副本

---

### H.3 批量处理 Checkpoint — P1.5, M (4-8h) — `dsfw-core` 框架模块

> **纳入框架**。`BatchCheckpoint` 操作 `TaskTypes.h` 中的 `BatchOutput`/`BatchInput`，与 `ITaskProcessor` 同层。`processBatch()` 默认实现自动接入 checkpoint，各处理器无需额外代码。

现状：`BatchOutput` 记录 `processedCount`/`failedCount`，但无法知道具体哪些文件已处理。如果批量处理中途崩溃或用户中断，必须从头重新处理。

**需求**：批量处理支持 checkpoint 文件，记录已完成的文件列表，支持断点续处理。

| # | 任务 | 涉及文件 |
|---|------|---------|
| 1 | `BatchOutput` 新增 `QStringList processedFiles` 和 `QStringList failedFiles` 字段（failedFiles 包含错误原因） | `TaskTypes.h` |
| 2 | 新增 `BatchCheckpoint` 工具类：写入/读取 `<workingDir>/dstemp/<taskName>.checkpoint.json` | 新建 `BatchCheckpoint.h/.cpp` |
| 3 | `ITaskProcessor::processBatch()` 默认实现中接入 checkpoint：跳过已完成文件、每完成一个文件追加记录 | `ITaskProcessor.cpp` |
| 4 | 各 TaskWindow 页面添加"继续上次处理"按钮（检测 checkpoint 文件存在时显示） | `SlicerPage`, `LyricFAPage`, `HubertFAPage`, `GameAlignPage` |
| 5 | 处理完成后自动删除 checkpoint 文件 | `ITaskProcessor.cpp` |

**Checkpoint 文件格式**:
```json
{
  "taskName": "phoneme_alignment",
  "processorId": "hubert-fa",
  "startTime": "2026-05-01T10:30:00",
  "processedFiles": ["001.wav", "002.wav"],
  "failedFiles": [{"file": "003.wav", "error": "Model inference failed"}]
}
```

**验收标准**:
- [ ] 批量处理中每完成一个文件，checkpoint 文件实时更新
- [ ] 中断后重启，检测到 checkpoint 时弹出"继续/重新开始"选择
- [ ] 选择"继续"后跳过已处理文件，从断点继续
- [ ] 全部处理完成后 checkpoint 文件自动清理
- [ ] `failedFiles` 包含失败原因，用户可查看

---

### H.4 统一 CrashHandler（替换 QBreakpad）— P1.5, M (4-8h) — ✅ 已完成

> ADR-21: 替换 QBreakpad，统一使用 dsfw::CrashHandler

QBreakpad 已移除。CrashHandler 已重写，使用 `AppPaths::dumpDir()`，支持 Windows MiniDumpWriteDump 和 Unix 信号处理。

**残留清理**: `AppInit.h:22` 仍有 QBreakpad 注释残留待清理。

**验收标准**:
- [x] QBreakpad 依赖完全移除（vcpkg.json + CMakeLists.txt）
- [x] Windows 崩溃写 .dmp + 调用 callback
- [x] macOS/Linux 崩溃写上下文信息到文件
- [x] `m_callback` 在所有平台崩溃时被调用
- [x] 三平台 CI 编译通过

---

### H.5 数据路径迁移到 QStandardPaths — P1.5, S (2-4h) — ✅ 已完成

> ADR-22: config/logs/dumps 迁移到 QStandardPaths::AppDataLocation

`AppPaths` 工具类已创建于 dsfw-core，使用 `QStandardPaths::AppDataLocation`。config/logs/dumps 路径已迁移，支持 `migrateFromLegacyPaths()` 旧路径自动迁移。`applicationDirPath()` 仅剩用于 model/dict 等捆绑资源路径（合理）。

**验收标准**:
- [x] 三平台下 config/logs/dumps 写入用户数据目录
- [x] 旧路径存在时自动迁移
- [x] macOS .app bundle 场景正常工作

---

## Phase F — 按需改进 (P3, 有需求时再做)

### F.1 示例项目 — P3, M (2-8h)

在 `examples/` 创建最小非 DiffSinger 应用，演示 dsfw 框架独立使用。当有外部用户需要参考时再做。

---

## Phase G — 任务处理器架构 (P1)

> 详细设计见 [task-processor-design.md](task-processor-design.md)
>
> **目标**：每个处理阶段可接入 N 种模型实现，I/O 统一为 ds-format 的 layer 结构。
> 统一 ITaskProcessor 替代 4 个 per-domain 服务接口，所有模型通过 ModelManager 管理。

### G.1 死代码清理 — P1, S (2-4h)

删除零消费者的基础设施。互不依赖，可全部并行。

| 目标 | 涉及文件 |
|------|---------|
| 删除 EventBus | `EventBus.h`, `EventBus.cpp` |
| 删除 PluginManager + IStepPlugin | `PluginManager.h/.cpp`, `IStepPlugin.h` |
| 删除 UpdateChecker | `UpdateChecker.h/.cpp` |
| 删除 RecentFiles | `RecentFiles.h/.cpp` |
| 删除 dsfw::UndoStack + dsfw::ICommand | `UndoStack.h/.cpp`, `ICommand.h` |
| 删除 IDocumentFormat + IDocument::format() | `IDocumentFormat.h`, 修改 `IDocument.h` 和 `DsDocumentAdapter.h/.cpp` |
| 删除 IPageProgress | `IPageProgress.h` |
| IPageActions 删除死方法 | `IPageActions.h`: 删除 `editActions()`, `viewActions()`, `toolActions()`, `save()` 及所有 override |
| IPageLifecycle 激活 onShutdown/onWorkingDirectoryChanged | `AppShell.cpp`: 关闭时 dispatch onShutdown，setWorkingDirectory 后 dispatch onWorkingDirectoryChanged |
| IModelManager 瘦身 | `IModelManager.h`: `setMemoryLimit/memoryLimit/currentMemoryUsage/status/loadedModels/registerProvider` 降级为 ModelManager 非虚方法 |
| 更新 architecture.md 模块图 | 移除已删除组件 |

### G.2 任务处理器基础设施 — P1, M (1-2d)

新增文件，不破坏现有代码。

| # | 任务 | 涉及文件 | 并行 |
|---|------|---------|------|
| 1 | 定义 `LayerData`, `TaskInput`, `TaskOutput`, `TaskSpec`, `BatchInput`, `BatchOutput` 数据类型 | `src/framework/core/include/dsfw/TaskTypes.h` | ✅ |
| 2 | 定义 `ITaskProcessor` 接口（含 `process()` + `processBatch()` + `capabilities()`） | `src/framework/core/include/dsfw/ITaskProcessor.h` | ✅ |
| 3 | 实现 `TaskProcessorRegistry`（单例 + Essentia 式 `Registrar<T>` 自注册） | `src/framework/core/include/dsfw/TaskProcessorRegistry.h`, `.cpp` | 依赖 #2 |
| 4 | `DsProjectDefaults` 添加 `std::map<QString, TaskModelConfig> taskModels`（兼容旧字段解析） | `DsProject.h/.cpp` | ✅ |
| 5 | 单元测试：TaskProcessorRegistry 注册/创建/列表 | `src/tests/framework/TestTaskProcessorRegistry.cpp` | 依赖 #3 |

### G.3 处理器迁移 — P1, L (3-5d)

逐个迁移，每个可独立完成和提交。旧服务接口并存，UI 页面逐步切换。

| # | 任务 | 工作量 | 涉及文件 | 并行 | 前置 | 说明 |
|---|------|--------|---------|------|------|------|
| 1 | **RmvpePitchProcessor** — 最简单，验证设计 | S (2-4h) | 新建 `src/libs/rmvpepitch/RmvpePitchProcessor.h/.cpp` | ✅ | G.2 | 单模型，process() 返回 f0 数据；processBatch() 逐文件提取 |
| 2 | **FunAsrProcessor** — 简单，单模型 | S (2-4h) | 新建 `src/libs/lyricfa/FunAsrProcessor.h/.cpp` | ✅ | G.2 | process() 返回 sentence 层；同时修复 LyricFAPage 绕过服务层的问题 |
| 3 | **HubertAlignmentProcessor** — 中等，有 G2P 依赖和 capabilities | M (4-8h) | 新建 `src/libs/hubertfa/HubertAlignmentProcessor.h/.cpp` | ✅ | G.2 | capabilities() 返回 language/nonSpeechPh 配置；IG2PProvider 构造注入 |
| 4 | **GameMidiProcessor** — 最复杂，多功能 | L (1-2d) | 新建 `src/libs/gameinfer/GameMidiProcessor.h/.cpp` | ✅ | G.2 | capabilities() 返回 5 个调参项；processBatch() 用 Game::alignCSV() 原生批量接口；注册为单个 ModelTypeId（不拆子模型） |

每个处理器完成后需验证：
- [ ] `TaskProcessorRegistry::Registrar<T>` 自注册生效
- [ ] `capabilities()` 返回正确的参数描述
- [ ] `initialize()` 通过 ModelManager 加载模型
- [ ] `process()` 单条处理正确
- [ ] `processBatch()` 批量处理正确（如适用）
- [ ] 旧服务接口仍正常工作（并存期间）

### G.4 集成与清理 — P1, L (2-3d)

| # | 任务 | 工作量 | 涉及文件 | 前置 |
|---|------|--------|---------|------|
| 1 | CLI 接入 TaskProcessorRegistry（替代 5 个 ServiceLocator::get 调用）| M (4-8h) | `src/apps/cli/main.cpp` | G.3 全部 |
| 2 | DiffSingerLabeler 页面切换到 TaskProcessorRegistry | L (1-2d) | `GameAlignPage.cpp`, `BuildDsPage.cpp`, `HubertFAPage.cpp`, `LyricFAPage.cpp` | G.3 全部 |
| 3 | GameInfer app 切换（消除 dynamic_cast，用 capabilities + config） | M (4-8h) | `MainWidget.cpp`, `GameInferService.h/.cpp` | G.3.4 |
| 4 | 删除旧服务接口 | S (2-4h) | `IAlignmentService.h`, `IAsrService.h`, `IPitchService.h`, `ITranscriptionService.h` + 所有旧实现 | #1-3 全部 |
| 5 | 接线 IExportFormat 到 DiffSingerLabeler | S (2-4h) | `src/apps/labeler/` | 独立 |
| 6 | 接线 IQualityMetrics 到 PipelineRunner（可选） | S (2-4h) | `src/domain/` | 独立 |

---

## 执行优先级

```
P1 — 架构演进（核心价值）— ✅ 全部完成
  G.1  死代码清理 (S)              — ✅
  G.2  任务处理器基础设施 (M)      — ✅
  G.3  处理器迁移 (L)              — ✅
  G.4  集成与清理 (L)              — ✅

P1.5 — 用户体验与可靠性
  H.5  数据路径迁移 QStandardPaths (S) — ✅
  H.4  统一 CrashHandler (M)           — ✅
  H.2  全局文件日志 Sink (S)           — 📋 待执行（前置 H.5 已完成）
  H.1  PitchLabeler 撤销重做补全 (M)   — 📋 待执行（独立）
  H.3  批量处理 Checkpoint (M)         — 📋 待执行（独立）

P2 — 有实际价值 — ✅ 全部完成
  B.1  补齐领域测试 (L)           — ✅
  D.1  框架独立编译 CI 验证 (M)   — ✅

P3 — 按需拾取
  B.2  TODO/FIXME 清理 (S)        — ✅
  B.3  文件操作错误处理 (S)       — ⏳ 大部分完成
  C.1  大文件拆分 (L)             — 📋 按需
  C.2  魔法数字常量化 (S)         — 📋 按需
  D.2  Doxygen CI (S)             — ✅
  D.3  跨平台包分发 (L)           — ✅
  F.1  示例项目 (M)               — 📋 按需
```

## 建议执行顺序

```
下一批: H.2 (文件日志) + H.1 (PitchLabeler 撤销补全) + H.3 (批量 checkpoint) — 全部可并行
收尾:   B.3 (文件操作错误处理补全) + C.1/C.2 — 按需拾取
```

---

## 关联 Issue

| Issue # | 标题 | 路线图 | 状态 |
|---------|------|--------|------|
| #11 | 领域模块单元测试 | B.1 | ✅ 已完成 |
| #15 | 框架模块独立编译 | D.1 | ✅ 已完成 |
| #16 | API 文档 | D.2 | ✅ 已完成 |
| #28 | TranscriptionPipeline 可测试性 | B.1 | ✅ 已完成 |
| #39 | God class 拆分 | C.1 | 📋 按需 |
| #40 | 魔法数字 | C.2 | 📋 按需 |
| — | 任务处理器架构 | G.1-G.4 | ✅ 全部完成 |
| — | CrashHandler 统一 | H.4 | ✅ 已完成 |
| — | 数据路径迁移 | H.5 | ✅ 已完成 |

已关闭: #21 (CI 矩阵), #27 (clang-tidy), #37 (Slicer 合并), #38 (MinLabel 提取)

---

## 剩余技术债

| 编号 | 描述 | 严重性 |
|------|------|--------|
| TD-03 | ~~4 个领域模块缺测试~~ | ✅ 已补齐 |
| TD-04 | ~~部分文件操作缺错误分支~~ | ⏳ 大部分已补全 |
| TD-05 | 2 个文件超 600 行 (PitchLabelerPage 755, PhonemeLabelerPage 600) | 低 |
| TD-10 | ~~11 个死接口/死基础设施占用维护成本~~ | ✅ 已清理 |
| TD-11 | ~~4 个服务各自管理模型，ModelManager 闲置~~ | ✅ 已迁移至 TaskProcessor |
| TD-12 | ~~DsProjectDefaults 硬编码 4 个模型路径字段~~ | ✅ 已演进为 TaskModelConfig |
| TD-13 | ~~GameInfer UI 通过 dynamic_cast 绕过接口调用 5 个 setter~~ | ✅ 已消除 |
| TD-14 | ~~LyricFAPage / SlicerPage 绕过服务层直接创建引擎~~ | ✅ 已切换到 TaskProcessorRegistry |
| TD-15 | PitchLabeler 5+ 个编辑操作绕过 QUndoStack 不可撤销 | 中 |
| TD-16 | 所有应用无持久化日志文件（仅有 minidump） | 中 |
| TD-17 | 批量处理无 checkpoint，中断后须从头重来 | 中 |
| TD-18 | ~~config/logs/dumps 路径基于 applicationDirPath()，macOS/Linux 安装场景可能只读~~ | ✅ 已迁移至 AppPaths |
| TD-19 | ~~dsfw::CrashHandler 是死代码且有 4 个 bug（未调用 callback、Unix 无 dump、缺 include、与 QBreakpad 并存）~~ | ✅ 已重写 |

> 更新时间：2026-05-02

---

## 任务执行清单

> 按执行顺序排列。同一批次内的任务互不依赖，可并行。

### 批次 2 — 清理 + 质量保障

| # | 任务 | 工作量 | 涉及文件 | 并行 |关联 | 状态 |
|---|------|--------|---------|------|------|------|
| 5 | **G.1** 死代码清理（11 个死接口/基础设施） | S (2-4h) | 见 Phase G.1 详细清单 | ✅ | TD-10 | ✅ |
| 6 | **B.1** 补齐领域模块单元测试 | L (1-3d) | `src/tests/domain/` | ✅ | TD-03, #11, #28 | ✅ |
| 7 | **D.1** 框架模块独立编译 CI 验证 | M (2-8h) | `.github/workflows/verify-modules.yml` | ✅ | #15 | ✅ |

### 批次 3 — 任务处理器基础设施

| # | 任务 | 工作量 | 涉及文件 | 并行 | 关联 | 状态 |
|---|------|--------|---------|------|------|------|
| 8 | **G.2** 定义 TaskTypes + ITaskProcessor + TaskProcessorRegistry + DsProjectDefaults 演进 | M (1-2d) | 见 Phase G.2 详细清单 | — | TD-11, TD-12 | ✅ |

### 批次 4 — 处理器迁移（4 个任务全部可并行）

| # | 任务 | 工作量 | 涉及文件 | 并行 | 关联 | 状态 |
|---|------|--------|---------|------|------|------|
| 9 | **G.3.1** RmvpePitchProcessor | S (2-4h) | `src/libs/rmvpepitch/` | ✅ | TD-11 | ✅ |
| 10 | **G.3.2** FunAsrProcessor | S (2-4h) | `src/libs/lyricfa/` | ✅ | TD-11, TD-14 | ✅ |
| 11 | **G.3.3** HubertAlignmentProcessor | M (4-8h) | `src/libs/hubertfa/` | ✅ | TD-11 | ✅ |
| 12 | **G.3.4** GameMidiProcessor | L (1-2d) | `src/libs/gameinfer/` | ✅ | TD-11, TD-13 | ✅ |

### 批次 5 — 集成与清理

| # | 任务 | 工作量 | 涉及文件 | 并行 | 前置 | 关联 | 状态 |
|---|------|--------|---------|------|------|------|------|
| 13 | **G.4.1** CLI 接入 TaskProcessorRegistry | M (4-8h) | `src/apps/cli/main.cpp` | ✅ | 批次 4 | — | ✅ |
| 14 | **G.4.2** DiffSingerLabeler 页面切换 | L (1-2d) | `src/apps/labeler/pages/` | ✅ | 批次 4 | TD-14 | ✅ |
| 15 | **G.4.3** GameInfer app 切换（消除 dynamic_cast） | M (4-8h) | `src/apps/GameInfer/` | ✅ | G.3.4 | TD-13 | ✅ |
| 16 | **G.4.4** 删除旧服务接口 (IAlignmentService 等 4 个) | S (2-4h) | `src/framework/core/` + 旧实现 | — | #13-15 | TD-11 | ✅ |
| 17 | **G.4.5** 接线 IExportFormat / IQualityMetrics | S (2-4h) | `src/apps/labeler/` | ✅ | 独立 | — | ✅ |

### 批次 6 — 用户体验与可靠性

| # | 任务 | 工作量 | 涉及文件 | 层级 | 并行 | 前置 | 关联 | 状态 |
|---|------|--------|---------|------|------|------|------|------|
| 25 | **H.5** 数据路径迁移 QStandardPaths | S (2-4h) | `dsfw-core`: `AppPaths.h/.cpp`; `AppSettings.cpp`, `CrashHandler.cpp`, `AppInit.cpp` | 框架 | ✅ | — | TD-18 | ✅ |
| 26 | **H.4** 统一 CrashHandler（替换 QBreakpad） | M (4-8h) | `dsfw-core`: `CrashHandler.cpp`; `ui-core`: `AppInit.cpp`, `CMakeLists.txt` | 框架 | ✅ | — | TD-19 | ✅ |
| 27 | **H.2** 全局文件日志 Sink + 日志轮转 | S (2-4h) | `dsfw-core`: `Log.h/.cpp` 或新建 `FileLogSink.h/.cpp`; `ui-core`: `AppInit.cpp` | 框架 | — | H.5 | TD-16 | 📋 |
| 28 | **H.1** PitchLabeler 撤销重做补全（5+ 个操作） | M (4-8h) | `src/apps/PitchLabeler/gui/ui/commands/`, `PitchLabelerPage.cpp` | 应用 | ✅ | — | TD-15 | 📋 |
| 29 | **H.3** 批量处理 Checkpoint（断点续处理） | M (4-8h) | `dsfw-core`: `TaskTypes.h`, 新建 `BatchCheckpoint.h/.cpp`, `ITaskProcessor.cpp`; 应用: TaskWindow 页面 | 框架+应用 | ✅ | — | TD-17 | 📋 |

### 批次 7 — 小修小补

| # | 任务 | 工作量 | 并行 | 关联 | 状态 |
|---|------|--------|------|------|------|
| 18 | **B.2** TODO/FIXME 清理 (5 处) | S (<2h) | ✅ | TD-07 | ✅ |
| 19 | **B.3** 文件操作错误处理补全 | S (<2h) | ✅ | TD-04 | ✅ |
| 20 | **C.2** 魔法数字常量化 | S (<2h) | ✅ | TD-06 | ✅ |

### 批次 8 — 按需拾取

| # | 任务 | 工作量 | 关联 | 状态 |
|---|------|--------|------|------|
| 21 | **C.1** 大文件拆分 (PitchLabelerPage/PhonemeLabelerPage) | L (1-3d) | TD-05 | 📋 |
| 22 | **D.2** Doxygen CI | S (<2h) | #16 | ✅ |
| 23 | **D.3** 跨平台包分发 | L (1-3d) | — | ✅ |
| 24 | **F.1** 示例项目 | M (2-8h) | — | 📋 |
