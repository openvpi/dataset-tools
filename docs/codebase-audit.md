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
| 3.1 | Extract business logic from `DsSlicerPage` into `SlicerService` | Medium | 1-2d |
| 3.2 | Extract inference orchestration from `PitchLabelerPage` into `PitchExtractionService` | Medium | 1d |
| 3.3 | Extract export logic from `ExportPage` into `ExportService` | Medium | 1d |
| 3.4 | Introduce ViewModel/Presenter pattern for shared data-source pages | Low | 2-3d |
| 3.5 | Audit and enforce that apps → libs → infer layering (no direct infer usage from apps) | Low | 4h |

### Phase 4: Testing & CI (ongoing)

| # | Task | Priority | Effort |
|---|---|---|---|
| 4.1 | Add unit tests for extracted service classes (Phase 3 output) | Medium | 1d per service |
| 4.2 | Add mock-based inference tests (mock ONNX session) | Medium | 2d |
| 4.3 | Add code coverage reporting to CI | Low | 4h |
| 4.4 | Add audio processing unit tests with fixture files | Low | 1d |
| 4.5 | Uncomment and enable automated release workflow | Low | 2h |

---

## Positive Findings

- **Clean header hygiene**: No `using namespace` in headers
- **No dead code blocks**: No `#if 0` sections
- **No TODO rot**: Project code is clean of stale TODOs
- **Good CI**: Multi-platform build, test execution, module independence verification, clang-tidy on PRs
- **Good layering**: Framework is independently consumable via `find_package`
- **Compiler cache**: sccache/ccache properly configured
- **Strong test foundation**: 32 test files covering domain + framework core
- **C++20 adoption**: Modern language features used appropriately
