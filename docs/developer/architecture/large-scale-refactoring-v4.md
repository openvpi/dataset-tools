# dataset-tools 大型重构方案 v4.1

> 版本：4.1.0
> 日期：2026-06-06
> 状态：已完成 (Phase A-F)
> 复核：全项目代码逐文件核实（v4.0 多处错误已修正）

---

## 0. 核心原则（不变）

### 0.1 功能完全不变

所有对外行为保持一致。重构仅改变内部实现方式，不改变可观测行为。

### 0.2 数据安全零妥协

所有文件 I/O 走统一安全通道（AtomicFileWriter）。关键数据结构（.dsproj、.dstext）保存时自动备份。不可变配置快照（INFRA-16）在流水线执行期间全程生效。

### 0.3 设计准则优先

所有重构方案必须通过 human-decisions.md 速查表逐条复核。冲突时以设计准则为准。

### 0.4 流水线兼容

不改变 10 步标注流水线的步骤定义、层依赖 DAG 和脏数据传播规则。

### 0.5 无历史包袱

废弃接口、残留引用、过渡 alias 全部清理。空目录、空文件、无意义注释一并清理。

### 0.6 统一底层接口

跨平台路径与编码由 `dsfw::PathUtils` 统一负责。文件路径选择由 `dsfw::widgets::PathSelector` 统一封装。音频 I/O 由 `dsfw-audio` 统一后端。

### 0.7 规范严谨

不使用 emoji。所有面向用户的消息经 `tr()` 国际化处理。日志消息使用英文。

---

## 1. 现状总览（经核实）

### 1.1 已完成重构（v3.0.0 Phase 1~5）

| 完成项 | 状态 |
|--------|------|
| 消除所有 path.string() 调用 | 已完成 |
| JsonHelper::saveFile 迁移到 AtomicFileWriter | 已完成 |
| 删除 DsDocument deprecated 方法 | 已完成 |
| curve_tools.cpp 迁移到 dsfw-signal | 已完成 |
| 删除空目录 | 已完成 |
| ExportService/AutoCompleteSlice 解耦具体引擎 | 已完成 |
| PitchLabelerPage 封装改进 | 已完成 |
| 统一路径选择控件行为 | 已完成 |
| IInferenceEngine 命名空间迁移到 dsfw::infer | 已完成 |
| 删除所有 namespace alias | 已完成 |
| 统一 include 路径 | 已完成 |
| SlicerPage 冗余 rename 清理 | 已完成 |
| 校验和验证扩展 | 已完成 |

### 1.2 核实后的待解决架构问题

| 编号 | 问题 | 实际状态 | 严重度 |
|------|------|---------|--------|
| A1 | dsfw-audio 已实现，但部分调用者仍使用旧路径 | 已实现，38 处引用，但 WaveformRenderer 等尚未迁移 | 中 |
| A2 | 三种文档模型并行，DsTextDocBridge 仅做 DsTextDocument↔DsPitchDocument 双向转换 | 桥接器职责清晰，无需大幅改动 | 低 |
| A3 | DsProject 私有成员 `nlohmann::json m_extraFields` 暴露在头文件中 | 编译所有包含 DsProject.h 的 TU 需解析 nlohmann/json.hpp | 中 |
| A4 | PitchLabelerPage 仍公开 `rmvpe()`/`game()` 访问器返回原始指针 | 已被 PitchLabelerPageHelpers 使用，但可通过 EnginePool 替代 | 中 |
| A5 | data-sources/ 扁平模块，类职责混杂但耦合可接受 | 当前结构可工作，拆分收益不明确 | 低 |
| A6 | 测试覆盖率不均衡 | 框架层测试较完善，领域层和应用层测试偏少 | 中 |
| A7 | CMake 缺少 install 规则和导出配置 | 影响可分发性 | 低 |
| A8 | 推理引擎生命周期由 EnginePool 管理，返回原始指针 | 已有 aliveToken 保护，但仍有改进空间 | 中 |
| A9 | 配置键分散在 CommonKeys（框架）/ DsKeys（领域）/ settings/Keys.h（应用） | 三级结构合理，但需确认无跨层引用 | 低 |
| A10 | 翻译文件不完整 | 部分页面缺翻译 | 低 |

---

## 2. 重构目标架构（经核实）

### 2.1 实际模块依赖图

```
                        ┌──────────────────────────────────────────────┐
                        │              应用层 (src/apps/)               │
                        │  ds-labeler · label-suite · dstools-cli · wg  │
                        │                                              │
                        │  shared/ (跨应用共享组件)                      │
                        │  ├── data-sources/   (扁平)                   │
                        │  │     EditorPageBase, EnginePool             │
                        │  │     SlicerPage, MinLabelPage               │
                        │  │     PhonemeLabelerPage, PitchLabelerPage   │
                        │  │     ExportService, SliceExportService       │
                        │  │     PitchExtractionService                 │
                        │  │     CompositeInferenceService              │
                        │  │     SliceListModel, SliceListPanel          │
                        │  │     PageFactory, PageDescriptors            │
                        │  │     BatchProcessDialog, etc.               │
                        │  ├── audio-visualizer/  AudioVisualizerContainer│
                        │  │     MiniMapScrollBar, PlayCursorOverlay    │
                        │  │     SliceTierLabel, DataAreaWidget         │
                        │  ├── chart-framework/   ChartPanelBase        │
                        │  │     IChartPanel, ChartCoordinate           │
                        │  │     BoundaryDragController, IBoundaryModel │
                        │  │     WaveformChartPanel, SpectrogramChartPanel│
                        │  │     PowerChartPanel, ChartConfigRegistry    │
                        │  ├── phoneme-editor/     PhonemeEditor        │
                        │  │     ui/ (TierEditWidget, TierLabelArea,    │
                        │  │          TextGridDocument, WaveformRenderer)│
                        │  ├── pitch-editor/       PitchEditor          │
                        │  │     ui/ (PianoRollChartPanel, PianoRollView,│
                        │  │          PitchProcessor, PropertyPanel)    │
                        │  ├── min-label-editor/   MinLabelEditor       │
                        │  ├── settings/           SettingsPage, Keys.h │
                        │  ├── bridges/            DsTextDocBridge      │
                        │  ├── log-page/           LogPage              │
                        │  └── mouth-curve-chart/  MouthCurveChartPanel │
                        └──────────────────┬───────────────────────────┘
                                           │
                        ┌──────────────────┴───────────────────────────┐
                        │         dstools-domain (STATIC) — 领域层      │
                        │                                              │
                        │  DsDocument (PIMPL'd), DsTextDocument (struct)│
                        │  DsPitchDocument, DsProject (NOT PIMPL'd)    │
                        │  DsTextTypes, LayerTypes, LayerSerialization  │
                        │  DsFileAdapter, CsvAdapter, TextGridAdapter  │
                        │  LabAdapter, AudacityMarkerAdapter           │
                        │  ModelManager, PinyinG2PProvider              │
                        │  PhNumCalculator, PitchUtils, F0Curve         │
                        │  ExportFormats, TextGridToCsv, CsvToDsConverter│
                        │  ProjectBackupManager, ProjectPaths           │
                        │  AudioFileResolver, DsKeys, Constants        │
                        └──────────────────┬───────────────────────────┘
                                           │
                ┌──────────────────────────┼──────────────────────────┐
                │                          │                          │
        ┌───────┴───────┐  ┌───────┴───────┐  ┌──────────────┴──────┐
        │ dsfw-widgets  │  │  dsfw-audio   │  │   dsfw-infer       │
        │ (SHARED DLL)  │  │  (STATIC)     │  │   (STATIC)         │
        │               │  │               │  │                    │
        │ PlayWidget    │  │ AudioBuffer   │  │ OnnxEnv            │
        │ PathSelector  │  │ FfmpegAudio   │  │ OnnxModelBase      │
        │ ViewportCtrl  │  │     Decoder   │  │ IInferenceEngine   │
        │ TimeRulerWgt  │  │ Swresample    │  │ IInferenceService  │
        │ ShortcutMgr   │  │     Resampler │  │ InferenceModel     │
        │ LogViewer     │  │ AudioPipeline │  │     Provider       │
        │ ToastNotif    │  │ AudioPlayback │  │                    │
        │ ProgressDlg   │  │     Adapter   │  │ 依赖: ONNX RT      │
        │ PropertyEdit  │  │ AudioPlayer   │  │                    │
        │ TaskWindow    │  │     Adapter   │  │                    │
        │               │  │ AudioFileWrtr │  │                    │
        │               │  │               │  │                    │
        │ 依赖: Qt6     │  │ 依赖: FFmpeg  │  │                    │
        └───────┬───────┘  └───────┬───────┘  └──────────┬─────────┘
                │                  │                      │
                └──────────┬───────┴──────────┬───────────┘
                           │                  │
                ┌──────────┴──────────────────┴───────────┐
                │            dsfw-core (STATIC)            │
                │                                         │
                │ AppSettings, ServiceLocator, Logger      │
                │ AsyncTask, PipelineRunner, Pipeline      │
                │ IFormatAdapter, ITaskProcessor           │
                │ IG2PProvider, IModelProvider             │
                │ AtomicFileWriter, CrashSafeGuard         │
                │ FormatAdapterRegistry                    │
                │ TaskProcessorRegistry                    │
                │ CommonKeys, ConfigTypes                  │
                │ PathUtils, JsonHelper                    │
                └──────────────────┬──────────────────────┘
                                   │
                ┌──────────────────┴──────────────────────┐
                │          dsfw-types (HEADER-ONLY)        │
                │                                         │
                │ Result<T>, ErrorCode, TimePos            │
                │ ExecutionProvider                        │
                └─────────────────────────────────────────┘

                ┌──────────────┐
                │ dsfw-signal  │
                │ (STATIC)     │
                │              │
                │ CurveTools   │
                │ MathUtils    │
                │ Slicer       │
                └──────────────┘
```

### 2.2 关键架构决策（经核实修正）

| 决策编号 | 决策内容 | 对照准则 | 依据 |
|---------|---------|---------|------|
| AD-01 | dsfw-audio 已实现，仅需迁移剩余调用者 | ARCH-01 | 38 处已有引用，WaveformRenderer 等仍在用旧路径 |
| AD-02 | 三种文档模型保持独立，DsTextDocBridge 仅做双向转换（已实现） | ARCH-04 | 职责不同，<30% 相同代码 |
| AD-03 | DsProject 私有成员 PIMPL 化，隐藏 nlohmann::json | INFRA-13 | 减少编译依赖传播 |
| AD-04 | 移除 PitchLabelerPage 的 rmvpe()/game() 公开访问器 | ARCH-06 | 通过 EnginePool 替代，依赖抽象 |
| AD-05 | 推理引擎生命周期由 EnginePool 管理，增加 shared_ptr 支持 | CONCUR-02 | 替代原始指针 + aliveToken 模式 |
| AD-06 | CMake 增加 install 规则和导出配置 | — | 支持外部项目集成 |
| AD-07 | 测试覆盖率目标：framework 90%+，domain 80%+，shared 70%+ | INFRA-17 | 框架层较完善，领域层和应用层偏少 |
| AD-08 | 翻译文件补齐 | — | 部分页面缺翻译 |

---

## 3. Phase A：DsProject 私有成员 PIMPL 化

> **目标**：将 DsProject 头文件中暴露的 `nlohmann::json m_extraFields` 通过 PIMPL 隔离，减少编译依赖传播。
> **前置条件**：无
> **风险**：低 — 纯重构，DsDocument 已有相同模式可参考

### A.1 现状

DsProject 当前定义（[DsProject.h](file:///d:/projects/dataset-tools/src/domain/include/dstools/DsProject.h)）：

```cpp
class DsProject {
public:
    DsProject() = default;
    // ... 公共方法 ...
private:
    QString m_filePath;
    QString m_workingDirectory;
    std::vector<Item> m_items;
    SlicerState m_slicerState;
    ExportConfig m_exportConfig;
    nlohmann::json m_extraFields; // 暴露在头文件中
};
```

对比 DsDocument 已完成的 PIMPL 化（[DsDocument.h](file:///d:/projects/dataset-tools/src/domain/include/dstools/DsDocument.h)）：

```cpp
class DsDocument {
    // ... 公共方法 ...
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
```

问题：所有包含 DsProject.h 的编译单元都需要解析 `nlohmann/json.hpp`（约 15000 行），增加编译时间。

### A.2 方案

参考 DsDocument 的 PIMPL 模式，将 DsProject 的所有私有数据成员移至 `DsProject::Impl`：

```cpp
// DsProject.h (修改后)
class DsProject {
public:
    DsProject();
    ~DsProject();
    DsProject(DsProject&&) noexcept;
    DsProject& operator=(DsProject&&) noexcept;
    DsProject(const DsProject&) = delete;
    DsProject& operator=(const DsProject&) = delete;

    // 公共方法不变...

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
```

```cpp
// DsProject.cpp (修改后)
struct DsProject::Impl {
    QString m_filePath;
    QString m_workingDirectory;
    std::vector<Item> m_items;
    SlicerState m_slicerState;
    ExportConfig m_exportConfig;
    nlohmann::json m_extraFields;
};
```

### A.3 修改文件清单

| 操作 | 文件 |
|------|------|
| 修改 | `src/domain/include/dstools/DsProject.h` — 改为 PIMPL |
| 修改 | `src/domain/src/DsProject.cpp` — 实现 Impl，所有 `m_` 成员改为 `m_impl->` |

### A.4 验收标准

- DsProject.h 不再包含 `nlohmann/json.hpp`
- 编译通过
- 所有现有单元测试通过
- DsProject 的 load/save round-trip 正常

---

## 4. Phase B：PitchLabelerPage 封装改进（移除公开引擎访问器）

> **目标**：移除 `rmvpe()`/`game()` 公开访问器，通过 EnginePool 获取引擎。
> **前置条件**：无
> **风险**：中 — 涉及 PitchLabelerPageHelpers 和调用者

### B.1 现状

[PitchLabelerPage.h](file:///d:/projects/dataset-tools/src/apps/shared/data-sources/PitchLabelerPage.h) 当前公开暴露：

```cpp
// 行 84-87
Rmvpe::Rmvpe *rmvpe() const { return m_rmvpe; }
Game::Game *game() const { return m_game; }
```

这些访问器被 `PitchLabelerPageHelpers.cpp` 使用（通过 `dstools::autoCompleteSlice` 自由函数），违反了 ARCH-06（依赖抽象接口）。

### B.2 方案

1. 将 `rmvpe()`/`game()` 改为 private
2. PitchLabelerPageHelpers 中通过 EnginePool 获取引擎，不再依赖 PitchLabelerPage 的公开访问器
3. 或者：如果 PitchLabelerPageHelpers 的函数需要引擎，改为通过 EnginePool 参数传递

### B.3 修改文件清单

| 操作 | 文件 |
|------|------|
| 修改 | `src/apps/shared/data-sources/PitchLabelerPage.h` — `rmvpe()`/`game()` 改为 private |
| 修改 | `src/apps/shared/data-sources/PitchLabelerPageHelpers.cpp` — 通过 EnginePool 获取引擎 |

### B.4 验收标准

- PitchLabelerPage.h 不再公开 `rmvpe()`/`game()`
- 编译通过
- 音高提取和 MIDI 提取功能正常

---

## 5. Phase C：EnginePool 改用 shared_ptr 管理引擎

> **目标**：将 EnginePool 的原始指针 + aliveToken 模式改为 shared_ptr 管理，简化并发安全机制。
> **前置条件**：Phase B 完成
> **风险**：高 — 涉及并发安全

### C.1 现状

当前 [EnginePool](file:///d:/projects/dataset-tools/src/apps/shared/data-sources/EnginePool.h) 使用以下模式：

```cpp
template <typename EngineType>
EngineType *acquire(const QString &modelKey);  // 返回原始指针

std::shared_ptr<std::atomic<bool>> aliveToken(const QString &modelKey);  // 存活令牌
```

调用者需要：
1. 通过 `acquire()` 获取原始指针
2. 通过 `aliveToken()` 获取并检查存活令牌
3. 在异步任务中检查 `aliveToken->load()` 确认引擎仍存活

问题：容易出错，调用者可能忘记检查存活令牌，导致 use-after-free。

### C.2 方案

将 EnginePool 改为返回 `shared_ptr`：

```cpp
template <typename EngineType>
std::shared_ptr<EngineType> acquire(const QString &modelKey);

// aliveToken 变为内部实现细节，不再对外暴露
```

调用者通过 `weak_ptr` 检查引擎是否仍存活：

```cpp
auto engine = m_enginePool->acquire<Rmvpe::Rmvpe>(taskKey);
std::weak_ptr<Rmvpe::Rmvpe> weakEngine = engine;

// 在异步任务中
if (auto e = weakEngine.lock()) {
    e->extract(...);
}
```

### C.3 修改文件清单

| 操作 | 文件 |
|------|------|
| 修改 | `src/apps/shared/data-sources/EnginePool.h` — 返回 shared_ptr |
| 修改 | 所有 `acquire()` 调用者 — 使用 shared_ptr/weak_ptr |
| 修改 | `EditorPageBase.h` — `ensureEngine` 返回 shared_ptr |

### C.4 验收标准

- EnginePool 不再对外暴露 aliveToken
- 所有调用者使用 shared_ptr/weak_ptr
- 编译通过，推理功能正常
- 并发场景下引擎访问安全

---

## 6. Phase D：音频管线调用者迁移

> **目标**：将仍使用旧 AudioDecoder 路径的调用者迁移到 dsfw-audio。
> **前置条件**：无
> **风险**：中 — 涉及核心音频处理路径

### D.1 现状

dsfw-audio 已实现（[AudioPipeline](file:///d:/projects/dataset-tools/src/framework/audio/dsfw-audio/include/dsfw/audio/AudioPipeline.h)）：

```cpp
class AudioPipeline {
public:
    static AudioPipeline create();
    Result<AudioFormatInfo> probe(const std::string &path);
    Result<AudioBuffer> decodeAndResample(const std::string &path, const ResampleConfig &config);
    Result<AudioBuffer> decodeSegmentAndResample(const std::string &path, double startSec, double endSec,
                                                 const ResampleConfig &config);
    Result<AudioBuffer> decodeToMonoFloat(const std::string &path, int targetSampleRate = 16000);
};
```

已有 38 处文件引用 `dsfw/audio/`，但 `WaveformRenderer` 等文件仍使用旧路径（直接通过 AudioDecoder 或手动处理）。

### D.2 方案

1. 迁移 `WaveformRenderer` 使用 `AudioPipeline::decodeToMonoFloat()`
2. 迁移各推理引擎的音频加载逻辑使用 `AudioPipeline`（HFA、RMVPE、GAME 已部分迁移）
3. 确认所有推理引擎（hubert-infer、rmvpe-infer、game-infer）统一使用 `AudioPipeline`
4. 删除任何残留的旧 AudioDecoder 直接调用

### D.3 修改文件清单

| 操作 | 文件 |
|------|------|
| 修改 | `src/apps/shared/phoneme-editor/ui/WaveformRenderer.cpp` — 使用 AudioPipeline |
| 核实 | `src/infer/hubert-infer/src/Hfa.cpp` — 已使用 dsfw/audio |
| 核实 | `src/infer/rmvpe-infer/src/Rmvpe.cpp` — 已使用 dsfw/audio |
| 核实 | `src/infer/game-infer/src/Game.cpp` — 已使用 dsfw/audio |
| 核实 | `src/apps/shared/data-sources/SlicerPage.cpp` — 已使用 dsfw/audio |
| 核实 | `src/apps/shared/data-sources/SliceExportService.cpp` — 已使用 dsfw/audio |

### D.4 验收标准

- 所有音频处理走 dsfw-audio 统一路径
- 编译通过
- 音频波形渲染、切片、推理功能正常

---

## 7. Phase E：CMake 构建系统完善

> **目标**：增加 install 规则和导出配置。
> **前置条件**：Phase A~D 完成
> **风险**：低

### E.1 方案

为每个库增加 install 规则：

```cmake
# 每个 target 增加
install(TARGETS dsfw-core
    EXPORT dsfw-targets
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY include/dsfw DESTINATION include)

# 根 CMakeLists.txt 增加
install(EXPORT dsfw-targets
    FILE dsfw-targets.cmake
    NAMESPACE dsfw::
    DESTINATION lib/cmake/dsfw
)
```

### E.2 验收标准

- `cmake --install build --prefix install` 正确安装所有头文件和库
- `find_package(dsfw)` 可供外部项目使用

---

## 8. Phase F：质量加固

> **目标**：补充测试、完善翻译、强化静态分析。
> **前置条件**：Phase A~E 完成
> **风险**：低

### F.1 测试补充

| 模块 | 当前状态 | 目标 | 需补充 |
|------|---------|------|--------|
| dsfw-audio | 4 个测试文件 | 85% | 边界条件、错误路径 |
| dstools-domain | DsDocument 测试 | 80% | DsProject save/load、ExportFormats、PhNumCalculator |
| shared/ | 部分测试 | 70% | EditorPageBase、ExportService、ChartConfigRegistry |

### F.2 翻译补齐

| 页面 | 当前状态 |
|------|---------|
| DsLabeler 全部页面 | 部分翻译 |
| LabelSuite 全部页面 | 部分翻译 |
| dsfw-widgets | 有翻译 |
| dsfw-ui-core | 有翻译 |

### F.3 静态分析强化

- 启用 clang-tidy 全部检查项
- 逐文件修复到 0 warning
- CI 中启用 `-Werror`

---

## 9. 执行依赖与顺序

```
Phase A (DsProject PIMPL 化)         ← 无前置依赖
Phase B (PitchLabelerPage 封装改进)   ← 无前置依赖
Phase C (EnginePool shared_ptr)      ← 依赖 Phase B
Phase D (音频管线调用者迁移)          ← 无前置依赖
Phase E (CMake 构建系统完善)         ← 依赖 Phase A~D
Phase F (质量加固)                   ← 依赖 Phase A~E
```

Phase A、B、D 可并行执行。Phase C 依赖 Phase B。Phase E、F 依赖前面的所有 Phase。

---

## 10. 风险矩阵

| 风险 | 严重度 | 缓解措施 |
|------|--------|---------|
| EnginePool shared_ptr 迁移可能引入循环引用 | 高 | 使用 weak_ptr 打破循环，ASan/TSan 验证 |
| PitchLabelerPage 访问器移除影响 Helpers | 中 | 先确认所有调用者，每步编译验证 |
| DsProject PIMPL 化可能遗漏成员访问 | 低 | 编译器会捕获所有遗漏 |
| WaveformRenderer 迁移可能影响渲染行为 | 中 | 端到端波形渲染对比测试 |

---

## 11. 补充准则（不变）

以下准则从 Qt、KDE Frameworks、LLVM、Google Chromium 等优秀 C++/Qt 项目实践中提炼：

### 11.1 编译隔离

- **PIMPL 必须化**：所有包含第三方库头文件的公共头文件必须使用 PIMPL（INFRA-13）
- **前向声明优先**：公共头文件中使用前向声明替代 include
- **include 顺序**：对应头文件（自测）→ 第三方库头文件 → 标准库头文件 → 项目头文件，每组间空行分隔
- **IWYU 合规**：每个头文件包含其直接使用的符号声明，不依赖间接包含

### 11.2 线程安全

- **信号槽连接类型显式化**：跨线程连接必须显式指定 `Qt::QueuedConnection`
- **QPointer 守卫**：所有从非 UI 线程访问 QWidget 的场景必须使用 `QPointer` 守卫 + `QMetaObject::invokeMethod`
- **shared_ptr 引擎所有权**：引擎实例通过 `shared_ptr` 管理，weak_ptr 替代存活令牌
- **不可变快照**：流水线执行期间使用的配置从 AppSettings 快照为不可变对象（INFRA-16）
- **锁的 RAII 包装**：使用 `std::lock_guard`/`std::scoped_lock`，禁止手动 lock/unlock（INFRA-06）

### 11.3 资源管理

- **RAII 严格化**：禁止裸 `new`/`delete`，使用 `std::make_unique`/`std::make_shared`（INFRA-06）
- **QObject 父子关系**：所有 QObject 派生类实例必须指定 parent 或通过 `std::unique_ptr` 管理
- **文件句柄即用即关**：禁止持有文件句柄超过函数作用域，使用 `AtomicFileWriter` 替代手动文件操作

### 11.4 API 设计

- **不可变参数 const 引用**：所有只读参数使用 `const &` 传递，禁止值传递大对象
- **返回值语义明确**：`Result<T>` 表示可能失败，`T` 表示不会失败，`std::optional<T>` 表示可能无值
- **接口最小化**：公共 API 仅暴露必要方法，内部方法使用 `private` + PIMPL 隐藏
- **接口版本化**：所有公共接口声明 `kInterfaceVersion`（INFRA-14）
- **信号参数规范**：小类型值传递，大对象 const ref 传递（INFRA-15）

### 11.5 构建系统

- **CMake target 可见性**：所有 `target_link_libraries` 使用 `PUBLIC`/`PRIVATE`/`INTERFACE` 显式标注
- **头文件搜索路径**：禁止使用 `include_directories()`，全部使用 `target_include_directories()`
- **编译警告即错误**：CI 中启用 `-Werror` / `/WX`，本地开发可关闭
- **install 规则**：每个库有 install 规则，支持 `find_package` 消费

### 11.6 国际化

- **所有用户可见字符串**：必须使用 `tr()` 包裹，禁止硬编码
- **日志消息**：使用英文，不使用 `tr()`
- **配置键**：使用英文路径格式，不使用中文
- **翻译文件**：每个 CMake target 有独立的 `.ts` 文件

### 11.7 错误处理

- **Result\<T\> 传播**：应用层逻辑使用 `Result<T>` 传播错误（ROBUST-02）
- **try-catch 仅限第三方边界**：nlohmann/json、ONNX Runtime、FFmpeg 边界（ROBUST-02）
- **错误信息追溯到根因**：每一层不忽略错误，直接传播（ROBUST-01）
- **禁止静默吞掉错误**：每个 catch 块必须记录日志或返回错误

### 11.8 代码卫生

- **无未使用 include**：每个 include 都有被使用的符号
- **无注释掉的代码**：删除而非注释
- **无 TODO/FIXME 残留**：要么修复，要么创建 issue
- **无魔术数字**：所有常量定义为 named constant

---

## 12. 重构检查清单

每个 Phase 完成后，必须通过以下检查：

- [ ] `cmake --build --preset release` 编译通过（0 error, 0 warning）
- [ ] 所有现有单元测试通过
- [ ] `clang-format` 格式检查通过
- [ ] 手动回归测试：该 Phase 涉及的功能正常
- [ ] 设计文档更新（如涉及）
- [ ] Memory MCP 更新
- [ ] 无 `path.string()` 新增调用
- [ ] 无 `processEvents()` 新增调用
- [ ] 无裸 `new`/`delete` 新增
- [ ] 所有新增文件头文件 include 路径正确
- [ ] 所有新增 API 有 `Result<T>` 或明确返回值语义
- [ ] 所有新增用户可见字符串走 `tr()`
- [ ] 无新增 `using namespace` 声明

---

## 13. 版本变更记录

### v4.0.0 → v4.1.0 (2026-06-06) — 全量核实修正

**核实发现并修正的错误**：

| 编号 | 错误内容 | 修正 |
|------|---------|------|
| E1 | "dsfw-audio 仅存在于设计文档，未实现" | 已实现：7 个源文件、10 个公共头文件、4 个测试文件、38 处引用 |
| E2 | 头文件命名错误：AudioFormat.h、FfmpegDecoder.cpp、AudioPlayback.cpp | 修正为 AudioFormatInfo.h、FfmpegAudioDecoder.cpp、AudioPlaybackAdapter.cpp |
| E3 | AudioPipeline API 设计错误（静态 decodeToFloat） | 修正为实例方法 create()/decodeAndResample()/decodeToMonoFloat() |
| E4 | AudioBuffer API 设计错误（mixToMono/extractChannel） | 修正为 SampleFormat 枚举 + 工厂方法 + span 视图 |
| E5 | "DsTextDocBridge 做三向转换" | 实际仅做 DsTextDocument↔DsPitchDocument 双向转换 |
| E6 | DsTextDocBridge API 设计错误（fromDsText/toDsText） | 修正为 extractIntervalLayers/mergeIntervalLayers/toPitchDoc/fromPitchDoc |
| E7 | "LayerDataVariant 有未使用类型" | 全部 7 种类型在 LayerSerialization.cpp 中活跃使用 |
| E8 | "data-sources/ 需要拆分为 5 个子目录" | 已为扁平模块，拆分无收益 |
| E9 | "需要新增 IPhonemeEditor/IPitchEditor 接口" | IChartPanel 接口已存在于 ChartPanelTypes.h |
| E10 | "AudioVisualizerContainer 需要 registerChartPanel" | 已有 addChart() 方法 + ChartEntry 结构 |
| E11 | "IInferenceService 需要新建" | 已存在于 dsfw/infer/IInferenceService.h |
| E12 | "CompositeInferenceService 需要新建" | 已存在于 data-sources/CompositeInferenceService.h |
| E13 | "IInferenceEngine 需要迁移到 dsfw::infer" | 已完成迁移 |
| E14 | "DsDocument 需要 PIMPL 化" | 已 PIMPL'd（struct Impl; unique_ptr<Impl> m_impl） |
| E15 | "EngineManager 需要新建" | EnginePool 已存在，仅需改进为 shared_ptr |
| E16 | "ExportService 依赖具体引擎指针" | 已使用 dsfw::infer::IInferenceService* |

**重构后的 Phase 列表**（6 个 Phase，从 7 个精简）：

| Phase | 目标 | 风险 |
|-------|------|------|
| A | DsProject 私有成员 PIMPL 化 | 低 |
| B | PitchLabelerPage 移除公开引擎访问器 | 中 |
| C | EnginePool 改用 shared_ptr 管理引擎 | 高 |
| D | 音频管线调用者迁移 | 中 |
| E | CMake 构建系统完善 | 低 |
| F | 质量加固 | 低 |