# 架构重构路线图

> 基于 2026-05-01 代码审计重新生成，仅保留待办事项。
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

---

## Phase B — 测试与质量 (P2)

### B.1 补齐领域模块单元测试 — P2, L (1-3d)

已有 19 个测试模块 (80+ 用例)。以下模块仍缺测试：

| 模块 | 测试重点 | 测试数据 |
|------|---------|---------|
| CsvToDsConverter | 正常转换、格式错误输入、边界 CSV | 样本 CSV + 预期 .ds |
| TextGridToCsv | TextGrid 解析、多层级、空区间 | 样本 .TextGrid 文件 |
| PitchProcessor | DSP 编辑算法、边界条件 | 合成 F0 数据 |
| TranscriptionPipeline | 4 步骤独立测试 (Deps 已支持注入) | mock 回调 |

**验收标准**:
- [ ] 4 个模块有单元测试
- [ ] 测试样本数据放入 `src/tests/data/`

---

### B.2 TODO/FIXME 清理 — P3, S (<2h)

| 文件 | 行号 | 内容 | 行动 |
|------|------|------|------|
| `GameInferPage.cpp` | 60 | Forward file paths to MainWidget | 实现或删除 |
| `BuildDsPage.cpp` | 89 | RMVPE-based F0 extraction | 集成 IPitchService 或标记 won't-fix |
| `game-infer/tests/main.cpp` | 320 | Map language string to ID | 从 config.json 读取 |
| `DsItemManager.cpp` | 58, 69 | add timestamp | 实现或删除 |

---

### B.3 文件操作错误处理补全 — P3, S (<2h)

搜索 `file.open()` 无 else 分支的代码点，补充错误处理。已有 `Result<T>` 基础设施，直接用即可。

---

## Phase C — 代码质量 (P3)

### C.1 大文件拆分 — P3, L (1-3d)

只拆真正有维护痛点的：

| 文件 | 行数 | 拆分方向 |
|------|------|---------|
| PitchLabelerPage.cpp | 781 | 文件 I/O → PitchFileService |
| PhonemeLabelerPage.cpp | 630 | 波形渲染 → WaveformRenderer |

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

### D.1 框架模块独立编译 CI 验证 — P2, M (2-8h)

dsfw-core / dsfw-ui-core 已有 `find_package` guards，但没有 CI 验证。

创建 `.github/workflows/verify-modules.yml`，矩阵构建各框架模块，确认外部消费路径可用。

---

### D.2 Doxygen CI — P3, S (<2h)

Doxyfile 已配置，所有公共头文件已有 Doxygen 注释。添加 CI step 生成文档并发布到 GitHub Pages。

---

### D.3 跨平台包分发 — P3, L (1-3d)

创建 `.github/workflows/release.yml`，监听 `v*` tag：
- Windows: 便携版 ZIP
- macOS: DMG
- Linux: AppImage

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
P1 — 架构演进（核心价值）
  G.1  死代码清理 (S)              — 减少维护负担
  G.2  任务处理器基础设施 (M)      — ITaskProcessor + Registry + 数据类型
  G.3  处理器迁移 (L)              — 逐个迁移 4 个处理器
  G.4  集成与清理 (L)              — 切换消费者 + 删除旧接口

P2 — 有实际价值
  B.1  补齐领域测试 (L)           — 防止回归
  D.1  框架独立编译 CI 验证 (M)   — 确认外部可消费

P3 — 按需拾取
  B.2  TODO/FIXME 清理 (S)
  B.3  文件操作错误处理 (S)
  C.1  大文件拆分 (L)
  C.2  魔法数字常量化 (S)
  D.2  Doxygen CI (S)
  D.3  跨平台包分发 (L)
  F.1  示例项目 (M)
```

## 建议执行顺序

```
批次 2: G.1 (死代码清理) + B.1 (领域测试) + D.1 (模块 CI) — 清理 + 质量保障
批次 3: G.2 (任务处理器基础设施) — 新增，不破坏现有代码
批次 4: G.3 (处理器迁移: RMVPE → FunASR → HuBERT → GAME) — 逐个迁移
批次 5: G.4 (集成: CLI → Labeler → GameInfer → 删除旧接口)
批次 6: B.2 + B.3 + C.2 — 小修小补
批次 7: 其余按需
```

---

## 关联 Issue

| Issue # | 标题 | 路线图 | 状态 |
|---------|------|--------|------|
| #11 | 领域模块单元测试 | B.1 | ⏳ 部分完成 |
| #15 | 框架模块独立编译 | D.1 | ⏳ CI 未验证 |
| #16 | API 文档 | D.2 | ⏳ CI 未完成 |
| #28 | TranscriptionPipeline 可测试性 | B.1 | ⏳ 主要障碍已清除 |
| #39 | God class 拆分 | C.1 | 📋 按需 |
| #40 | 魔法数字 | C.2 | 📋 按需 |
| — | 任务处理器架构 | G.1-G.4 | 📋 设计完成，待执行 |

已关闭: #21 (CI 矩阵), #27 (clang-tidy), #37 (Slicer 合并), #38 (MinLabel 提取)

---

## 剩余技术债

| 编号 | 描述 | 严重性 |
|------|------|--------|
| TD-03 | 4 个领域模块缺测试 | 中 |
| TD-04 | 部分文件操作缺错误分支 | 低 |
| TD-05 | 2 个文件超 600 行 | 低 |
| TD-06 | `4096` buffer size 3 处重复 | 低 |
| TD-07 | 5 处 TODO/FIXME | 低 |
| TD-10 | 11 个死接口/死基础设施占用维护成本 | 中 |
| TD-11 | 4 个服务各自管理模型，ModelManager 闲置 | 高 |
| TD-12 | DsProjectDefaults 硬编码 4 个模型路径字段 | 中 |
| TD-13 | GameInfer UI 通过 dynamic_cast 绕过接口调用 5 个 setter | 中 |
| TD-14 | LyricFAPage / SlicerPage 绕过服务层直接创建引擎 | 中 |

> 更新时间：2026-05-01

---

## 任务执行清单

> 按执行顺序排列。同一批次内的任务互不依赖，可并行。

### 批次 2 — 清理 + 质量保障

| # | 任务 | 工作量 | 涉及文件 | 并行 |关联 |
|---|------|--------|---------|------|------|
| 5 | **G.1** 死代码清理（11 个死接口/基础设施） | S (2-4h) | 见 Phase G.1 详细清单 | ✅ | TD-10 |
| 6 | **B.1** 补齐领域模块单元测试 | L (1-3d) | `src/tests/domain/` | ✅ | TD-03, #11, #28 |
| 7 | **D.1** 框架模块独立编译 CI 验证 | M (2-8h) | `.github/workflows/verify-modules.yml` | ✅ | #15 |

### 批次 3 — 任务处理器基础设施

| # | 任务 | 工作量 | 涉及文件 | 并行 | 关联 |
|---|------|--------|---------|------|------|
| 8 | **G.2** 定义 TaskTypes + ITaskProcessor + TaskProcessorRegistry + DsProjectDefaults 演进 | M (1-2d) | 见 Phase G.2 详细清单 | — | TD-11, TD-12 |

### 批次 4 — 处理器迁移（4 个任务全部可并行）

| # | 任务 | 工作量 | 涉及文件 | 并行 | 关联 |
|---|------|--------|---------|------|------|
| 9 | **G.3.1** RmvpePitchProcessor | S (2-4h) | `src/libs/rmvpepitch/` | ✅ | TD-11 |
| 10 | **G.3.2** FunAsrProcessor | S (2-4h) | `src/libs/lyricfa/` | ✅ | TD-11, TD-14 |
| 11 | **G.3.3** HubertAlignmentProcessor | M (4-8h) | `src/libs/hubertfa/` | ✅ | TD-11 |
| 12 | **G.3.4** GameMidiProcessor | L (1-2d) | `src/libs/gameinfer/` | ✅ | TD-11, TD-13 |

### 批次 5 — 集成与清理

| # | 任务 | 工作量 | 涉及文件 | 并行 | 前置 | 关联 |
|---|------|--------|---------|------|------|------|
| 13 | **G.4.1** CLI 接入 TaskProcessorRegistry | M (4-8h) | `src/apps/cli/main.cpp` | ✅ | 批次 4 | — |
| 14 | **G.4.2** DiffSingerLabeler 页面切换 | L (1-2d) | `src/apps/labeler/pages/` | ✅ | 批次 4 | TD-14 |
| 15 | **G.4.3** GameInfer app 切换（消除 dynamic_cast） | M (4-8h) | `src/apps/GameInfer/` | ✅ | G.3.4 | TD-13 |
| 16 | **G.4.4** 删除旧服务接口 (IAlignmentService 等 4 个) | S (2-4h) | `src/framework/core/` + 旧实现 | — | #13-15 | TD-11 |
| 17 | **G.4.5** 接线 IExportFormat / IQualityMetrics | S (2-4h) | `src/apps/labeler/` | ✅ | 独立 | — |

### 批次 6 — 小修小补

| # | 任务 | 工作量 | 并行 | 关联 |
|---|------|--------|------|------|
| 18 | **B.2** TODO/FIXME 清理 (5 处) | S (<2h) | ✅ | TD-07 |
| 19 | **B.3** 文件操作错误处理补全 | S (<2h) | ✅ | TD-04 |
| 20 | **C.2** 魔法数字常量化 | S (<2h) | ✅ | TD-06 |

### 批次 7 — 按需拾取

| # | 任务 | 工作量 | 关联 |
|---|------|--------|------|
| 21 | **C.1** 大文件拆分 (PitchLabelerPage/PhonemeLabelerPage) | L (1-3d) | TD-05 |
| 22 | **D.2** Doxygen CI | S (<2h) | #16 |
| 23 | **D.3** 跨平台包分发 | L (1-3d) | — |
| 24 | **F.1** 示例项目 | M (2-8h) | — |
