# 架构重构路线图

> 基于 2026-05-01 代码审计重新生成，删除已完成项，仅保留待办事项。
>
> **原则**: 功能齐全、简单可靠、不过度设计。接口保持一致性和合理的扩展预留。
>
> **范围**: 在本仓库内完成。所有模块作为 CMake 子目录共存。
>
> **C++ 标准**: 允许 C++20（当前 C++17，已有 `std::filesystem` 使用）。

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

## Phase A — 可靠性与接口一致性 (P2) ✅ 已完成

### A.1 ServiceLocator 改用 std::any — ✅ 已完成

已在 commit `aa0192a` 中完成。ServiceLocator 内部存储已改为 `std::any`，使用 `std::any_cast` 进行类型安全取出。

### A.2 服务接口一致性修复 — ✅ 已完成

IAsrService 已补齐 `loadModel()`/`isModelLoaded()`/`unloadModel()` 纯虚方法。IInferenceEngine::load() 已改为纯虚 `= 0`。

### A.3 CrashHandler 全量启用 — ✅ 已完成

所有 6 个 GUI app (GameInfer, DiffSingerLabeler, PhonemeLabeler, DatasetPipeline, PitchLabeler, TestShell) 的 `AppInit::init()` 已改为 `initCrashHandler=true`。

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

Doxyfile 已配置，23 个框架头文件已有 Doxygen 注释。添加 CI step 生成文档并发布到 GitHub Pages。

---

### D.3 跨平台包分发 — P3, L (1-3d)

创建 `.github/workflows/release.yml`，监听 `v*` tag：
- Windows: 便携版 ZIP
- macOS: DMG
- Linux: AppImage

---

## Phase E — C++20 升级 (P2) ✅ 已完成

### E.1 升级 C++ 标准 — ✅ 已完成

1. `CMakeLists.txt`: 已改为 `set(CMAKE_CXX_STANDARD 20)`
2. README.md 已更新编译器要求为 C++20
3. CI 验证待确认

升级后按需使用 C++20 特性，不搞全面迁移运动。现有代码工作正常的不改。

---

## Phase F — 按需改进 (P3, 有需求时再做)

### F.1 示例项目 — P3, M (2-8h)

在 `examples/` 创建最小非 DiffSinger 应用，演示 dsfw 框架独立使用。当有外部用户需要参考时再做。

### F.2 Undo/Redo 迁移 — P3, L (1-3d)

PhonemeLabeler / PitchLabeler 现有自定义实现工作正常。当需要重构这些 app 时顺便迁移到 dsfw UndoStack，不单独排期。

---

## 执行优先级

```
P2 — 有实际价值
  A.1  ServiceLocator 类型安全 (S) — ✅ 已完成
  A.2  服务接口一致性修复 (S)     — ✅ 已完成
  A.3  CrashHandler 全量启用 (S)  — ✅ 已完成
  B.1  补齐领域测试 (L)           — 防止回归
  D.1  框架独立编译 CI 验证 (M)   — 确认外部可消费
  E.1  升级 C++20 (S)             — ✅ 已完成

P3 — 按需拾取
  B.2  TODO/FIXME 清理 (S)
  B.3  文件操作错误处理 (S)
  C.1  大文件拆分 (L)
  C.2  魔法数字常量化 (S)
  D.2  Doxygen CI (S)
  D.3  跨平台包分发 (L)
  F.1  示例项目 (M)
  F.2  Undo/Redo 迁移 (L)
```

## 建议执行顺序

```
批次 1: A.1 + A.2 + A.3 + E.1 — ✅ 全部完成
批次 2: B.1 (领域测试) + D.1 (模块 CI) — 质量保障
批次 3: B.2 + B.3 + C.2 — 小修小补，随手做
批次 4: 其余按需
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

已关闭: #21 (CI 矩阵), #27 (clang-tidy), #37 (Slicer 合并), #38 (MinLabel 提取)

---

## 剩余技术债

| 编号 | 描述 | 严重性 |
|------|------|--------|
| ~~TD-01~~ | ~~ServiceLocator 使用 void* 存储，类型不匹配时 UB~~ | ✅ 已修复 |
| ~~TD-02~~ | ~~IAsrService 缺模型生命周期虚方法，与其他服务接口不一致~~ | ✅ 已修复 |
| TD-03 | 4 个领域模块缺测试 | 中 |
| TD-04 | 部分文件操作缺错误分支 | 低 |
| TD-05 | 2 个文件超 600 行 | 低 |
| TD-06 | `4096` buffer size 3 处重复 | 低 |
| TD-07 | 5 处 TODO/FIXME | 低 |
| ~~TD-08~~ | ~~IInferenceEngine::load() 非纯虚，子类可意外不实现~~ | ✅ 已修复 |
| ~~TD-09~~ | ~~CrashHandler 仅 1/6 app 启用~~ | ✅ 已修复 |

> 更新时间：2026-05-01

---

## 任务执行清单

> 按执行顺序排列。同一批次内的任务互不依赖，可并行。

### 批次 1 — 接口加固 + 基础设施 ✅ 已完成

| # | 任务 | 状态 | 关联 |
|---|------|------|------|
| 1 | **A.1** ServiceLocator void* → std::any | ✅ 已完成 | TD-01 |
| 2 | **A.2** IAsrService 补齐虚方法；IInferenceEngine::load() 改纯虚 | ✅ 已完成 | TD-02, TD-08 |
| 3 | **A.3** CrashHandler 全量启用 | ✅ 已完成 | TD-09 |
| 4 | **E.1** CMake 升级 C++20 + README 更新编译器要求 | ✅ 已完成 | — |

### 批次 2 — 质量保障

| # | 任务 | 工作量 | 涉及文件 | 并行 | 前置 | 关联 |
|---|------|--------|---------|------|------|------|
| 5 | **B.1** 补齐领域模块单元测试 (CsvToDsConverter, TextGridToCsv, PitchProcessor, TranscriptionPipeline) | L (1-3d) | `src/tests/domain/` 新增 4 个测试文件 + `src/tests/data/` 样本数据 | ✅ 可并行 | — | TD-03, #11, #28 |
| 6 | **D.1** 创建 `verify-modules.yml` CI 矩阵验证各框架模块独立编译 | M (2-8h) | `.github/workflows/verify-modules.yml` | ✅ 可并行 | — | #15 |

> 批次 2 两个任务互不依赖，可同时进行。不依赖批次 1（但建议批次 1 先合入以减少 CI 噪音）。

### 批次 3 — 小修小补

| # | 任务 | 工作量 | 涉及文件 | 并行 | 前置 | 关联 |
|---|------|--------|---------|------|------|------|
| 7 | **B.2** TODO/FIXME 清理 (5 处) | S (<2h) | `GameInferPage.cpp`, `BuildDsPage.cpp`, `game-infer/tests/main.cpp`, `DsItemManager.cpp` | ✅ 可并行 | — | TD-07 |
| 8 | **B.3** 文件操作错误处理补全 (file.open 缺 else 分支) | S (<2h) | 散布于 `src/domain/`, `src/apps/` | ✅ 可并行 | — | TD-04 |
| 9 | **C.2** 魔法数字常量化 (`4096` buffer 3 处重复) | S (<2h) | `WaveformWidget.cpp`, `PhonemeLabelerPage.cpp`, `AudioFileLoader.cpp` | ✅ 可并行 | — | TD-06, #40 |

> 批次 3 三个任务互不依赖，可同时进行。无前置依赖。

### 批次 4 — 按需拾取（无固定顺序）

| # | 任务 | 工作量 | 并行 | 前置 | 关联 |
|---|------|--------|------|------|------|
| 10 | **C.1** PitchLabelerPage (781行) / PhonemeLabelerPage (630行) 拆分 | L (1-3d) | ✅ 两个文件可并行拆 | — | TD-05, #39 |
| 11 | **D.2** Doxygen CI (生成文档 + 发布 GitHub Pages) | S (<2h) | ✅ 独立 | — | #16 |
| 12 | **D.3** 跨平台包分发 (release.yml: ZIP/DMG/AppImage) | L (1-3d) | ✅ 独立 | — | — |
| 13 | **F.1** 示例项目 (非 DiffSinger 应用) | M (2-8h) | ✅ 独立 | — | — |
| 14 | **F.2** Undo/Redo 迁移 (PitchLabeler → dsfw UndoStack) | L (1-3d) | ✅ 独立 | — | — |

> 批次 4 任务均独立，有需求时随时拾取，不阻塞其他工作。
