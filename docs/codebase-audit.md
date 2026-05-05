# Codebase Audit: Issues, Tech Debt & Roadmap

**Date**: 2026-05-04  
**Scope**: `dataset-tools` full repository  
**Method**: Static analysis, pattern search, CI/build log review

---

## Summary

| Category | Severity | Count |
|---|---|---|
| Build Issues | Critical | 1 |
| Thread Safety | High | 7 |
| Memory Management | Medium | ~15 |
| Architecture Debt | Medium | 5 |
| Code Quality / Tech Debt | Low-Medium | ~20 |
| Test Coverage Gaps | Medium | 4 |
| CI/CD Gaps | Low | 3 |

---

## 1. Critical Issues (P0)

### 1.1 Broken Local Build: `dsfw-base.lib` Not Found

**File**: `build_test_log.txt`  
**Symptom**: `LNK1104: cannot open file "dsfw-base.lib"` when building `TestDsDocument` locally.  
**Root Cause**: Debug build dependency chain not correctly exposing `dsfw-base` as a link target, or build order issue with NMake generator (CI uses Ninja and works fine).  
**Impact**: Cannot run domain tests locally on Windows with NMake.

**Fix**: Ensure `dsfw-base` is built before test targets. Check `target_link_libraries` for TestDsDocument — may need explicit dependency on `dsfw-base` or the generator expression for its import lib path.

---

## 2. High Severity Issues (P1)

### 2.1 Thread Safety: `QtConcurrent::run` Captures `this`

**Files**:
- `src/apps/shared/data-sources/MinLabelPage.cpp` (lines 418, 471)
- `src/apps/shared/data-sources/PhonemeLabelerPage.cpp` (lines 386, 512)
- `src/apps/shared/data-sources/PitchLabelerPage.cpp` (lines 520, 562, 783)

**Problem**: Lambda captures `this` (a QWidget) and runs on a thread pool. If the user navigates away or the widget is destroyed while the task is running, `this` becomes a dangling pointer. Additionally, accessing widget members from a non-GUI thread violates Qt's thread affinity rules.

**Fix**:
- Use `QPointer<ThisClass>` or weak capture pattern with guard check before touching `this`.
- Marshal results back to GUI thread via `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` or signal/slot.
- Consider a cancellation token pattern so tasks abort on page destruction.

### 2.2 No Model Validation Before ONNX Load

**Files**: `src/infer/game-infer/`, `src/infer/hubert-infer/`, `src/infer/rmvpe-infer/`

**Problem**: ONNX model files are loaded from disk without validating file integrity, expected input/output shapes, or versioning. A corrupted or incompatible model will crash at inference time with an opaque ONNX Runtime error.

**Fix**: Add lightweight model validation (check file magic bytes, expected node names, input dimensions) before creating inference sessions. Surface user-friendly errors.

---

## 3. Medium Severity Issues (P2)

### 3.1 God Classes (500+ Lines)

| File | Lines | Issue |
|---|---|---|
| `DsSlicerPage.cpp` | 945 | Single class handles UI, slicing logic, file I/O, context menu, state management |
| `PitchLabelerPage.cpp` | 707 | Mixed concerns: UI + inference orchestration + data persistence |
| `ExportPage.cpp` | 637 | Export logic interleaved with UI updates |
| `SettingsPage.cpp` | 615 | All settings in one monolithic page |
| `GameModel.cpp` | 609 | Complex model logic could be split into encode/segment/estimate |
| `WaveformPanel.cpp` | 583 | Rendering + interaction + audio loading |
| `PianoRollView.cpp` | 581 | Rendering + input handling + selection state |

**Fix**: Extract business logic into service/controller classes. UI classes should only handle layout and user interaction, delegating to a service layer.

### 3.2 Hardcoded Path Conventions

**Files**: `DsSlicerPage.cpp`, `ExportPage.cpp`, `BuildCsvPage.cpp`, `BuildDsPage.cpp`, `ProjectDataSource.cpp`, `NewProjectDialog.cpp`

**Problem**: Path segments like `"/dstemp/dsitems"`, `"/dstemp/slices/"`, `"/wavs"`, `"/ds"` are scattered as string literals across multiple files. If the directory convention changes, all must be updated.

**Fix**: Centralize all path conventions into a single `ProjectPaths` utility class:
```cpp
class ProjectPaths {
public:
    static QString dsItemsDir(const QString &workingDir);
    static QString slicesDir(const QString &workingDir);
    // ...
};
```

### 3.3 Manual `delete` Without RAII

**Notable instances** (non-`= delete` pattern):
- `DsProject.cpp` — `delete project` (lines 104, 133, 180)
- `MinLabelPage.cpp` — `delete m_asr` / `delete m_match` (lines 33, 34, 214, 223)
- `SingleInstanceGuard.cpp` — `delete m_lockFile` / `delete m_server`
- `TaskWindow.cpp` — manual lifecycle management

**Problem**: Raw `new`/`delete` is error-prone for exception safety and early returns.

**Fix**: Replace with `std::unique_ptr` where ownership is exclusive, or use Qt's parent-child ownership model explicitly.

### 3.4 Excessive Singletons

**Instances**: `CrashHandler`, `FormatAdapterRegistry`, `Logger`, `TaskProcessorRegistry`, `OnnxEnv`, `Theme`

**Problem**: Singletons create hidden global state, make unit testing difficult (can't inject mocks), and introduce initialization-order issues.

**Fix**: The existing `ServiceLocator` pattern is already in the codebase — migrate remaining singletons to it. `OnnxEnv` and `Theme` are good candidates. Keep only `CrashHandler` and `Logger` as true singletons (justified by lifecycle needs).

### 3.5 Test Coverage Gaps

**Covered** (good): domain layer (14 test files), framework core (14 test files), libs (2 files), types (1 file), widgets (1 file)

**Not covered**:
- `src/apps/` — No unit tests for any application page logic
- `src/infer/` — Tests exist but require models (off by default), no mock-based unit tests
- `src/framework/ui-core/` — Only 1 integration test (`TestAppShellIntegration`)
- `src/framework/audio/` — No tests

**Fix**: 
1. Extract testable business logic from Page classes → test those service classes
2. Add mock ONNX sessions for inference unit tests
3. Add audio processing unit tests with fixture wav files

---

## 4. Low Severity / Tech Debt (P3)

### 4.1 No `using namespace` in Headers — CLEAN

No violations found. Good discipline.

### 4.2 No `#if 0` Dead Code — CLEAN

No violations found.

### 4.3 Minimal TODOs in Project Code

Only found in vendored ONNX Runtime headers (not project code). Clean.

### 4.4 Singleton `FileLogSink` Allocates `QMutex` on Heap

**File**: `FileLogSink.cpp:27` — `auto *mutex = new QMutex()`

**Fix**: Use `static QMutex` or `std::mutex` member.

### 4.5 `reinterpret_cast` Usage in Audio Code

**Files**: `AudioDecoder.cpp`, `AudioPlayback.cpp`, `Util.cpp`, DSP/FFTW code

**Assessment**: Mostly justified for audio buffer type-punning (float↔byte). Low risk but could benefit from `std::span<std::byte>` wrappers for clarity.

### 4.6 CI: clang-tidy Warnings Not Blocking

**File**: `.github/workflows/build.yml` (clang-tidy job)

**Problem**: clang-tidy results are reported as `::warning` but never fail the build. `WarningsAsErrors: ''` in `.clang-tidy` means no check fails CI.

**Fix**: Set `WarningsAsErrors` to match `Checks` (or a subset) to gate PRs on code quality.

### 4.7 CI: No Code Coverage Reporting

No lcov/gcov/coverage step in CI. No coverage badge.

**Fix**: Add `gcovr` or `llvm-cov` step post-test, upload to Codecov/Coveralls.

### 4.8 CI: Release Workflow Commented Out

**File**: `.github/workflows/build.yml` — Release archive creation and upload steps are all commented out.

**Fix**: Uncomment and test when ready for automated releases.

### 4.9 Build Verification Checks Stale App Names

**File**: `.github/workflows/build.yml` — `Verify build outputs` step checks for `DatasetPipeline`, `MinLabel`, `PhonemeLabeler`, `PitchLabeler`, `GameInfer`, `DiffSingerLabeler`. But README says outputs are `LabelSuite.exe`, `DsLabeler.exe`.

**Problem**: Either the verification step is outdated or there's a name mapping. Could cause CI false-passes if new names aren't checked.

**Fix**: Align verification with actual CMake target names / output binary names.

---

## 5. Architecture Observations

### 5.1 Layering — Generally Good

```
types (header-only) → framework (base→core→ui-core→widgets)
                    → domain → libs → infer → apps
```

CI verifies independent module builds (`verify-modules.yml`). `find_package(dsfw)` integration test exists. Well-structured.

### 5.2 Concern: App Layer is Monolithic

`src/apps/shared/` (81 files) contains data-sources, editors, waveform panel, pitch editor, phoneme editor. These are shared across apps but mix UI rendering, data orchestration, and inference invocation in single classes.

**Recommendation**: Introduce a thin "ViewModel" or "Presenter" layer between Page (UI) and the domain/infer services.

### 5.3 Concern: `src/infer/` Directly Exposes Implementation

Inference libraries (`game-infer`, `hubert-infer`, `rmvpe-infer`) are direct shared libraries. The `src/framework/infer/` contains some abstraction but the app layer calls concrete inference types.

**Recommendation**: The existing `src/libs/` wrapper layer (e.g., `game-infer-lib`, `hubert-fa`, `rmvpe-pitch`) already provides this. Ensure apps ONLY depend on `libs/`, never directly on `infer/`.

---

## 6. Phased Task Roadmap

### Phase 1: Stability & Safety (1-2 weeks)

| # | Task | Priority | Effort |
|---|---|---|---|
| 1.1 | ~~Fix local build issue (dsfw-base.lib link order)~~ ✅ | Critical | 2h |
| 1.2 | ~~Add `QPointer` guard to all `QtConcurrent::run` + `this` captures~~ ✅ | High | 4h |
| 1.3 | ~~Add ONNX model validation before session creation~~ ✅ | High | 4h |
| 1.4 | ~~Align CI build verification with actual binary names~~ ✅ | Medium | 1h |

### Phase 2: Code Quality (2-4 weeks)

| # | Task | Priority | Effort |
|---|---|---|---|
| 2.1 | ~~Extract `ProjectPaths` utility, replace all hardcoded path strings~~ ✅ | Medium | 3h |
| 2.2 | ~~Replace manual `delete` with `unique_ptr` in DsProject, MinLabelPage, SingleInstanceGuard~~ ✅ | Medium | 3h |
| 2.3 | ~~Enable `WarningsAsErrors` for key clang-tidy checks in CI~~ ✅ | Medium | 2h |
| 2.4 | ~~Migrate `OnnxEnv` and `FormatAdapterRegistry` singletons to ServiceLocator~~ ✅ | Low | 4h |
| 2.5 | ~~Fix heap-allocated QMutex in FileLogSink~~ ✅ | Low | 30min |

### Phase 3: Architecture Refactoring (4-8 weeks)

| # | Task | Priority | Effort |
|---|---|---|---|
| 3.1 | ~~Extract business logic from `DsSlicerPage` into `SlicerService`~~ ✅ | Medium | 1-2d |
| 3.2 | ~~Extract inference orchestration from `PitchLabelerPage` into `PitchExtractionService`~~ ✅ | Medium | 1d |
| 3.3 | ~~Extract export logic from `ExportPage` into `ExportService`~~ ✅ | Medium | 1d |
| 3.4 | ~~Introduce ViewModel/Presenter pattern for shared data-source pages~~ ✅ | Low | 2-3d |
| 3.5 | ~~Audit and enforce that apps → libs → infer layering (no direct infer usage from apps)~~ ✅ | Low | 4h |

### Phase 4: Testing & CI (ongoing)

****| # | Task | Priority | Effort |
|---|---|---|---|
| 4.1 | ~~Add unit tests for extracted service classes (Phase 3 output)~~ ✅ | Medium | 1d per service |
| 4.2 | ~~Add mock-based inference tests (mock ONNX session)~~ ✅ | Medium | 2d |
| 4.3 | ~~Add code coverage reporting to CI~~ ✅ | Low | 4h |
| 4.4 | ~~Add audio processing unit tests with fixture files~~ ✅ | Low | 1d |
| 4.5 | ~~Uncomment and enable automated release workflow~~ ✅ | Low | 2h |

---

---

## 7. 设计原则回顾（来自 unified-app-design.md）

在分析模块结构之前，先明确设计文档中已确立的两个应用的定位差异：

### 7.1 两个应用的本质区别

| | LabelSuite | DsLabeler |
|---|---|---|
| **定位** | 通用标注工具集，散装独立页面 | DiffSinger 一条龙，.dsproj 驱动 |
| **页面数** | 10（全部暴露，每步可独立操作） | 7（合并/隐藏可自动化步骤） |
| **文件格式** | FormatAdapter 兼容旧格式（TextGrid/.lab/.ds） | 仅 dstext，内部 PipelineContext |
| **中间文件** | 无工程目录管理 | **不产生用户可见中间文件**，导出时一次性生成 |
| **独有页面** | Align, CSV, MIDI, DS（显式控制每步） | Welcome, Export（工程管理+一次性导出） |
| **数据源** | `FileDataSource`（文件系统） | `ProjectDataSource`（.dsproj） |

### 7.2 核心复用原则（已落地 ✅）

设计文档已明确（ADR-52/66/68/69/70）：

1. **底层统一**：两个 app 共享 dstext/PipelineContext 数据模型
2. **编辑器共享**：MinLabelEditor, PhonemeEditor, PitchEditor 完全共享
3. **IEditorDataSource 抽象**：页面不感知数据来源（文件 vs 工程）
4. **ISettingsBackend 抽象**：Settings UI 共享，后端可切换 QSettings/.dsproj
5. **自动补全统一**：进入 Phone/Pitch 页时自动 FA/F0/MIDI，两个 app 行为一致

### 7.3 LabelSuite 独有页面的意义

LabelSuite 保留 Align/CSV/MIDI/DS 四个独立页面**不是冗余**，而是设计意图：

- 用户需要**显式控制每个步骤**（如：只跑 HubertFA 对齐，不做后续）
- 兼容第三方工具（MFA、SOFA）的中间产物
- 不需要工程文件管理，直接操作文件

---

## 8. 代码现状 vs 设计文档的差距

### 8.1 当前 CMake Target 全景

```
层级          目标名                         类型
──────────────────────────────────────────────────
Framework     dsfw-base                      STATIC
              dsfw-core                      STATIC/SHARED
              dsfw-ui-core                   STATIC
              dstools-audio                  STATIC
              dstools-infer-common           STATIC
              dsfw-widgets                   SHARED

Domain        dstools-domain                 STATIC

Infer         audio-util                     SHARED (DLL)
              FunAsr                         STATIC
              game-infer                     SHARED (DLL)
              hubert-infer                   SHARED (DLL)
              rmvpe-infer                    SHARED (DLL)

Libs          gameinfer-lib                  STATIC
              hubertfa-lib                   STATIC
              lyricfa-lib                    STATIC
              minlabel-lib                   STATIC
              rmvpepitch-lib                 STATIC
              slicer-lib                     STATIC
              textgrid                       STATIC

Apps/Shared   data-sources                   STATIC    ← 含 Page + Service
              settings-page                  STATIC
              min-label-editor               STATIC
              phoneme-editor                 STATIC
              pitch-editor                   STATIC
              audio-visualizer               STATIC    ← AudioVisualizerContainer + MiniMap + SliceTierLabel

Apps/旧Pages  ~~minlabel-page~~               已删除 (5B)
              ~~phonemelabeler-page~~         已删除 (5B)
              ~~pitchlabeler-page~~           已删除 (5B)
              ~~pipeline-slicer-page~~        已删除 (5B)
              ~~pipeline-lyricfa-page~~       已删除 (5B)
              ~~pipeline-hubertfa-page~~      已删除 (5B)
              ~~pipeline-pages~~              已删除 (5B)

Apps (exe)    LabelSuite, DsLabeler, dstools-cli, WidgetGallery, TestShell
```

**总计: ~40 个 CMake targets（不含测试）**

### 8.2 依赖关系（当前实际）

```
LabelSuite ─┬─ pipeline-pages ─┬─ pipeline-slicer-page ── slicer-lib
             │                  ├─ pipeline-lyricfa-page ── lyricfa-lib
             │                  └─ pipeline-hubertfa-page ── hubertfa-lib
             ├─ data-sources ──── (MinLabelPage, PhonemeLabelerPage, PitchLabelerPage...)
             ├─ game-infer, rmvpe-infer (直接依赖 DLL)
             └─ dstools-ui-core, dstools-widgets

DsLabeler ──┬─ data-sources (同上)
             ├─ min-label-editor, phoneme-editor, pitch-editor, settings-page
             └─ dstools-ui-core, dstools-widgets
```

### 8.3 关键差距分析

#### ❌ 差距1：LabelSuite 有两套实现路径

设计文档说 LabelSuite 和 DsLabeler **共享全部页面组件**（§4.3），区别仅在数据源。

但当前代码中 LabelSuite 的 Slice/ASR/Align 三个页面走**旧 pipeline-pages 体系**（独立实现），而非 data-sources 中的共享 Page：

```
设计意图:  LabelSuite Slice 页 → data-sources/SlicerService + FileDataSource
当前实际:  LabelSuite Slice 页 → pipeline-slicer-page → slicer-lib (独立实现)
```

这导致：
- 同一功能维护两份代码
- LabelSuite 独有页面（Align/CSV/MIDI/DS）中 BuildCsvPage、BuildDsPage、GameAlignPage 已在 LabelSuite 内部实现 ✅
- 但 Slice/ASR/Align 仍走旧路径 ❌

#### ❌ 差距2：6 个遗留 Page 库 + 1 个空目录

M.4.1 删除独立 exe 后留下的壳：
- `minlabel-page`, `phonemelabeler-page`, `pitchlabeler-page` — 旧独立 app 的 page 库
- `pipeline-slicer-page`, `pipeline-lyricfa-page`, `pipeline-hubertfa-page` — 旧 Pipeline app 的 page 库
- `pipeline-pages` INTERFACE — 聚合上面三个
- `game-infer-app/` — 空目录

按设计文档，这些应该被 `data-sources` 中的统一 Page 替代。

#### ✅ 做对的部分

- `data-sources` 已包含 SlicerService, ExportService, PitchExtractionService ✅
- 编辑器组件（phoneme-editor, pitch-editor, min-label-editor）已共享 ✅
- IEditorDataSource 抽象已实现 ✅
- DsLabeler 的 7 页结构和 .dsproj 驱动已实现 ✅
- FormatAdapter 旧格式兼容已实现 ✅

#### ⚠️ libs/ 层是否冗余？

设计文档 §3.11 的 CMake 示例直接链接 `hubert-infer`, `game-infer` 等 DLL，不经过 libs/ 层。但当前代码中 `data-sources` 依赖 `hubertfa-lib`, `gameinfer-lib` 等 wrapper。

这是**有意设计**还是**遗留**？需要看 libs 的实际内容：
- `hubertfa-lib`：包含 alignment decoding 逻辑（非薄壳），**应保留**
- `lyricfa-lib`：包含 FunASR + 歌词匹配逻辑（非薄壳），**应保留**
- `slicer-lib`：包含 RMS 切片算法（非薄壳），**应保留**
- `gameinfer-lib`：GAME model 的 Qt wrapper，需评估厚度
- `rmvpepitch-lib`：RMVPE 的 Qt wrapper，需评估厚度
- `minlabel-lib`：MinLabel 业务逻辑，**应保留**

libs/ 层对 CLI 工具（`dstools-cli`）也是必要的——CLI 直接用 libs 层而不依赖任何 UI。

---

## 9. 模块精简方案（基于设计原则）

### 原则约束

1. LabelSuite 保留全部 10 个散装页面，兼容旧格式
2. DsLabeler 一条龙，不产生中间文件，一次性导出
3. 底层不动（framework/infer 层已经很好）
4. libs/ 层为 CLI 提供无 UI 接口，**不能删**
5. 目标：消除**旧 page 壳**和**双路径**，不是无脑减 target 数

### Phase 5A: 统一 LabelSuite 到 data-sources（核心，2-3 周）

**目标**：LabelSuite 的 Slice/ASR/Align 三个页面也走 `data-sources` + `FileDataSource`，与 DsLabeler 共享同一套 Page 组件。

| # | Task | 说明 |
|---|---|---|
| 5A.1 | LabelSuite Slice 页 → `data-sources/SlicerService` + `FileDataSource` | 替代 `pipeline-slicer-page`。SlicerService 已有切片逻辑，FileDataSource 已有文件系统访问 |
| 5A.2 | LabelSuite ASR 页 → `data-sources/MinLabelPage` 的 ASR 功能 + `FileDataSource` | 替代 `pipeline-lyricfa-page`。MinLabelPage 已内置 ASR 按钮 |
| 5A.3 | LabelSuite Align 页 → `data-sources/PhonemeLabelerPage` 的 FA 功能 + `FileDataSource` | 替代 `pipeline-hubertfa-page`。PhonemeLabelerPage 已内置自动 FA |
| 5A.4 | LabelSuite 独有页面（CSV/DS/MIDI）保留在 `label-suite/pages/` 内部 | 这些是 LabelSuite 的专属页面，不需要共享，保留现状即可 |

**注意**：LabelSuite 的 Align/MIDI 页面与 DsLabeler 的"自动 FA/自动 MIDI"有区别——LabelSuite 需要**独立界面**让用户显式执行。实现方式是在共享 Page 上添加 LabelSuite 专属的"手动运行"按钮/菜单，通过数据源类型判断是否显示。

### Phase 5B: 清理遗留目标 ✅ 已完成

| # | Task | 说明 | 状态 |
|---|---|---|---|
| 5B.1 | 删除 `pipeline-slicer-page` | 被 data-sources SlicerService 替代 | ✅ |
| 5B.2 | 删除 `pipeline-lyricfa-page` | 被 data-sources MinLabelPage 替代 | ✅ |
| 5B.3 | 删除 `pipeline-hubertfa-page` | 被 data-sources PhonemeLabelerPage 替代 | ✅ |
| 5B.4 | 删除 `pipeline-pages` INTERFACE | 不再需要 | ✅ |
| 5B.5 | 删除 `minlabel-page` | data-sources 中已有 MinLabelPage | ✅ |
| 5B.6 | 删除 `phonemelabeler-page` | data-sources 中已有 PhonemeLabelerPage | ✅ |
| 5B.7 | 删除 `pitchlabeler-page` | data-sources 中已有 PitchLabelerPage | ✅ |
| 5B.8 | 删除 `game-infer-app/` 空目录 | 清理 | ✅ (已不存在) |
| 5B.9 | 删除 `src/apps/pipeline/` 目录 | 所有 page 已迁移 | ✅ |

**结果**: 减少 7 个 CMake targets，删除 1 个空目录，删除 1 个目录。PitchLabelerKeys.h 迁移到 pitch-editor。

### Phase 5C: 评估 libs 层薄壳 ✅ 已评估

| # | Task | 说明 | 结论 |
|---|---|---|---|
| 5C.1 | 审计 `gameinfer-lib` | ITaskProcessor 适配器 + TaskProcessorRegistry 自注册 | **保留** — 架构合理，infer 引擎保持框架无关 |
| 5C.2 | 审计 `rmvpepitch-lib` | ITaskProcessor 适配器 + TaskProcessorRegistry 自注册 | **保留** — 同上 |
| 5C.3 | `textgrid` 改为 INTERFACE target | header-only 第三方库 | **已是 INTERFACE** — 无需改动 |

**结论**: 三个库均非无意义薄壳。gameinfer-lib 和 rmvpepitch-lib 是有意设计的适配器层（将 infer 引擎适配到 ITaskProcessor 框架接口），textgrid 是完整的第三方 header-only 库。均应保留。

### Phase 5D: 构建系统优化 ✅ 已完成

| # | Task | 说明 | 状态 |
|---|---|---|---|
| 5D.1 | `dstools-infer-common` 的 `target_link_directories` → imported target | CMake 反模式修复 | ✅ 创建 INTERFACE IMPORTED onnxruntime target |
| 5D.2 | `FunAsr` C++17 → 继承项目 C++20 | 避免混合标准 | ✅ FunAsr + infer-target.cmake + test-find-package |
| 5D.3 | 评估 Unity Build | 对大量小 static libs 效果显著 | ❌ 不采用 — PCH + ccache 已覆盖 |
| 5D.4 | 评估 infer DLL → STATIC | 减少运行时依赖和部署复杂度 | ✅ 默认改为 STATIC，消除 dllimport 警告 |

---

## 10. 精简后的目标架构

```
精简后 (~33 targets，含测试目标):

Framework (6, 可独立 find_package 消费):
  dsfw-base → dsfw-core → dsfw-ui-core → dsfw-widgets
  dstools-audio
  dstools-infer-common

Domain (1):
  dstools-domain

Infer (5, DLL 独立可部署):
  audio-util, FunAsr, game-infer, hubert-infer, rmvpe-infer

Libs (7, CLI 和 GUI 共用的业务封装):
  slicer-lib, lyricfa-lib, hubertfa-lib
  gameinfer-lib, rmvpepitch-lib, minlabel-lib
  textgrid (→ INTERFACE)

App-Shared (6, 两个 app 共享):
  data-sources        ← Page + Service 统一入口
  min-label-editor    ← 歌词编辑 UI 组件
  phoneme-editor      ← 音素编辑 UI 组件 (含 TierLabelArea, PhonemeTextGridTierLabel)
  pitch-editor        ← 音高编辑 UI 组件
  audio-visualizer    ← 统一音频可视化容器 (AudioVisualizerContainer + MiniMap + SliceTierLabel)
  settings-page       ← 设置 UI

Apps (4):
  LabelSuite          ← 10 页散装，独有页面(CSV/DS/MIDI/GameAlign)在内部
  DsLabeler           ← 7 页一条龙，独有页面(Welcome/Export)在内部
  dstools-cli
  WidgetGallery (dev only)

已删除 (8 targets):
  pipeline-slicer-page, pipeline-lyricfa-page, pipeline-hubertfa-page
  pipeline-pages (INTERFACE)
  minlabel-page, phonemelabeler-page, pitchlabeler-page
  game-infer-app/ (空目录)
```

### 设计文档 §11.1 的澄清

设计文档 §11 已更新：`waveform-panel` 已删除，`AudioVisualizerContainer`（`src/apps/shared/audio-visualizer/`）为统一替代方案。Slicer 和 PhonemeLabeler 均已迁移到该架构。

---

## 11. 完整分阶段路线图总览

| Phase | 主题 | 周期 | 核心交付 |
|---|---|---|---|
| **1** | 稳定性 & 安全 | 1-2 周 | 修复 build, 线程安全, 模型校验 |
| **2** | 代码质量 | 2-4 周 | ProjectPaths, RAII, CI 强化 |
| **3** | 架构重构 | 4-8 周 | God class 拆分, Service 提取 |
| **4** | 测试 & CI | 持续 | 覆盖率, mock infer, release 自动化 |
| **5A** | 统一 LabelSuite 到 data-sources | 2-3 周 | LabelSuite Slice/ASR/Align 走共享 Page |
| **5B** | 清理遗留 targets | 1 周 | 删除 7 个旧 page 库 |
| **5C** | Libs 层评估 | 1 周 | 审计 thin wrapper，textgrid → INTERFACE |
| **5D** | 构建优化 | 1 周 | imported target, unity build |

---

## Positive Findings

- **设计文档质量极高**：unified-app-design.md 已明确两个 app 的边界、复用策略、ADR 记录完整
- **复用架构已落地**：IEditorDataSource, ISettingsBackend, FormatAdapter 等抽象已实现
- **M.4.1 迁移已在进行**：独立 exe 已删除，Page 库化已完成，仅差最后一步统一
- **Clean header hygiene**: No `using namespace` in headers
- **No dead code blocks**: No `#if 0` sections
- **No TODO rot**: Project code is clean of stale TODOs
- **Good CI**: Multi-platform build, test execution, module independence verification, clang-tidy on PRs
- **Good layering**: Framework is independently consumable via `find_package`
- **Strong test foundation**: 32 test files covering domain + framework core
