# 架构重构路线图

> 基于 2026-04-30 架构分析，目标：通用框架分离、模块解耦、前后端分离、推理层统一
>
> **范围**: 在本仓库内完成库分离，不新建独立仓库。所有模块作为 CMake 子目录共存。

---

## 目标总览

| # | 目标 | 总工作量 | 优先级 |
|---|------|---------|--------|
| G1 | 通用框架分离改名（仓库内独立库） | L (3-4w) | P0 |
| G2 | 通用框架补充常用功能 | XL (4-6w) | P2 |
| G3 | 解耦模块 | M (1-2w) | P1 |
| G4 | 提供稳定可靠的标准控件 | L (3-4w) | P2 |
| G5 | Qt 和非 Qt 依赖分开 | M (1-2w) | P0 |
| G6 | 前后端分开 | L (2-3w) | P1 |
| G7 | DS 和非 DS 相关分开 | S (<1w) | P1 |
| G8 | 相似 infer 模块构建共用底层 | L (2-3w) | P1 |

---

## Phase 0 — 预备工作 (Week 1-2)

### T-0.1 泛化 ModelType 枚举 [G1+G7] ✅ 已完成

**现状**: `IModelProvider.h` 中 `ModelType` 枚举包含 DS 专有值 (`Asr`, `HuBERT`, `GAME`, `RMVPE`, `SOME`)，框架层不应感知业务领域模型类型。

**方案**:
1. 将 `ModelType` 改为 **整数 ID + 字符串注册表** 模式：
   ```cpp
   // dsfw-core: 通用模型类型标识
   class ModelTypeId {
       int m_id;
       // ...
   };
   // 框架提供注册 API
   ModelTypeId registerModelType(const std::string &name);
   ```
2. DS 专有类型在 `dstools-domain` 的初始化代码中注册：
   ```cpp
   // dstools-domain/ModelTypes.h
   inline const auto MT_ASR    = registerModelType("Asr");
   inline const auto MT_HUBERT = registerModelType("HuBERT");
   inline const auto MT_GAME   = registerModelType("GAME");
   ```
3. `IModelProvider::type()` 返回 `ModelTypeId` 而非枚举。
4. `ModelManager` 内部使用 `ModelTypeId` 作为 key，不再硬编码枚举。

**影响范围**: `IModelProvider.h`, `ModelManager.h/.cpp`, 所有 apps 中 `ModelType::GAME` 等引用点。

**风险**:
- ⚠️ **高** — 所有使用 `ModelType` 枚举的代码都需更新（约 6 个 app + widgets/ModelLoadPanel）
- 缓解：提供 `constexpr ModelTypeId` 别名在 domain 头文件中，消费者只需改 include 路径

**工作量**: M (2-8h)

---

### T-0.2 移除 dsfw-core 对 Qt::Gui 的依赖 [G5] ✅ 已完成

**现状**: dsfw-core 链接 `Qt::Gui`，CLI 场景被迫引入 GUI 运行时。已知使用点: `QColor`（AppSettings 中的颜色配置项）。

**方案**:
1. 审查 `dsfw-core/src/` 中所有 `#include <QColor>` / `<QImage>` / `<QIcon>`
2. 颜色值使用字符串表示 (`"#RRGGBB"`)，在 ui-core 层转换为 `QColor`
3. CMakeLists 中移除 `Qt::Gui`，仅保留 `Qt::Core` + `Qt::Network`
4. 编译验证 dsfw-core 在无 Gui 环境下可编译

**风险**: 低 — 影响面小

**工作量**: S (<2h)

> ✅ 已完成 — dsfw-core CMakeLists 现在仅链接 Qt::Core 和 Qt::Network，Qt::Gui 依赖已移除。

---

### T-0.3 推理公共层去 Qt 化 [G5] ✅ 已完成

**现状**: `OnnxEnv.h` 包含 `#include <QString>`，`createSession()` 使用 `QString *errorMsg` 参数。`dstools-infer-common` 因此链接 `Qt::Core`。

**方案**:
1. `errorMsg` 参数改为 `std::string *`
2. 移除 `OnnxEnv.h` 中的 `<QString>` include
3. CMakeLists 移除 `Qt::Core` 依赖
4. 各推理库如需 QString 转换，在自身层做适配

**影响范围**: `OnnxEnv.h`, `OnnxEnv.cpp`, 所有调用 `createSession` 的推理模块

**风险**: 低 — API 变更仅影响 infer 子系统内部

**工作量**: S (<2h)

---

### T-0.4 修复 pipeline-pages 相对路径源文件引用 [G3] ✅ 已完成

> **已完成** — slicer、lyricfa、hubertfa 已独立构建为静态库，pipeline-pages 已通过 target_link_libraries 链接。无需额外修改。

**现状**: `pipeline-pages` CMakeLists 通过 `../slicer/` 等相对路径直接引用源码。

**方案**:
1. 将 `slicer`、`lyricfa`、`hubertfa` 各自建为独立 STATIC 库（在 `src/libs/` 下各自有 `CMakeLists.txt` + `include/` 目录）
2. `pipeline-pages` 通过 `target_link_libraries` 链接
3. 验证 DatasetPipeline 和 DiffSingerLabeler 编译运行正常

**风险**: 中 — 构建顺序可能变化

**工作量**: M (2-8h)

---

### T-0.5 审计 dsfw 的 DS 业务泄漏 [G7] ✅ 已完成

**现状**: 框架层（dsfw-core, dsfw-ui-core）应不包含任何 DiffSinger 专有逻辑。

**方案**:
1. 扫描所有 `src/framework/` 头文件，查找 DS 特有术语 (DiffSinger, .ds, .dsproj, G2P, Pinyin, ASR, HuBERT, GAME 等)
2. 确认 `DocumentFormat` 如存在同样需要泛化（同 T-0.1 模式）
3. 记录所有泄漏点，纳入后续任务

**风险**: 低

**工作量**: S (<2h)

---

### T-0.5 审计结果 (completed)

> 扫描范围: `src/framework/` 全部 `.h` / `.cpp` 文件
> 搜索关键词: DiffSinger, .ds, .dsproj, Pinyin, ASR, HuBERT, GAME, RMVPE, SOME, DsFile, G2P

| 文件 | 泄漏点 | 严重程度 | 后续任务 |
|------|--------|---------|---------|
| `IModelProvider.h` | `ModelType` 枚举包含 DS 专有值 (`Asr`, `HuBERT`, `GAME`, `RMVPE`, `SOME`) | 高 | T-0.1 |
| `IDocument.h` | `DocumentFormat::DsFile` — "DiffSinger .ds JSON file" | 中 | T-0.1 一并处理（同模式泛化为注册表） |
| `IG2PProvider.h` | 整个文件 — G2P (Grapheme-to-Phoneme) 为语音合成领域特定概念，不属于通用桌面框架 | 中 | T-1.1 迁移至 domain 层；框架保留 `ITextProcessor` 或类似泛化接口 |
| `ModelManager.h/.cpp` | 使用 `ModelType` 枚举作为 key — 属于传递性泄漏，修复 `IModelProvider.h` 后自动解决 | 低（传递性） | T-0.1 自动修复 |

**未发现额外泄漏**: 框架内无 `Pinyin`、`.dsproj`、`DiffSinger` 字符串引用（`DsFile` 注释中的 "DiffSinger" 除外）。`dsfw-ui-core` 层干净，无 DS 业务渗透。

---

## Phase 1 — 核心分离 (Week 3-5)

### T-1.1 ModelManager / ModelDownloader 迁移到 domain [G1+G7] ✅ 已完成

**现状**: `ModelManager` 和 `ModelDownloader` 位于 `dsfw-core` 但强依赖 `ModelType` 枚举中的 DS 语义。

**方案**:
1. 在 dsfw-core 定义泛化的 `IModelManager` 接口
2. 将 `ModelManager` 具体实现移至 `dstools-domain`
3. 框架提供 `IModelDownloader` 接口 + `StubModelDownloader`（已存在），具体下载逻辑在 domain
4. Apps 通过 `ServiceLocator::get<IModelManager>()` 获取

**依赖**: T-0.1 (ModelType 泛化)

**风险**: 中 — apps 直接使用 `dsfw::ModelManager` 的代码需改 include 路径

**工作量**: M (2-8h)

---

### T-1.2 dsfw-core 拆分 Qt-free 基础层 [G5] ✅ 已完成

**现状**: `dsfw-core` 中 `Result<T>`, `JsonHelper`(使用 nlohmann/json), 纯虚接口 (`IDocument`, `IFileIOProvider`, `IExportFormat`) 均不需要 Qt。

**方案**:
1. 新建 `dsfw-base` 模块 (STATIC 或 INTERFACE)，包含：
   - `Result<T>` (从 dstools-types 整合)
   - `JsonHelper` (纯 nlohmann/json 封装)
   - 纯虚接口：`IDocument`, `IFileIOProvider`, `IExportFormat`, `IG2PProvider`, `IQualityMetrics`
2. `dsfw-core` 依赖 `dsfw-base`，保留 Qt 相关: `AppSettings`, `AsyncTask`, `ServiceLocator`
3. 头文件路径保持 `<dsfw/...>` 不变，通过 CMake INTERFACE include 目录转发

**依赖**: T-0.2

**风险**:
- ⚠️ 中 — 所有 `#include <dsfw/IDocument.h>` 的消费者 CMake 需链接 `dsfw-base`
- 缓解：`dsfw-core` PUBLIC 依赖 `dsfw-base`，现有消费者无感

**工作量**: M (2-8h)

> ✅ 已完成 — dsfw-base 已创建于 src/framework/base/，包含 JsonHelper（Qt-free）。dsfw-core 依赖 dsfw-base。

---

### T-1.3 丰富 IInferenceEngine 接口 [G8] ✅ 已完成

**现状**: `IInferenceEngine` 仅有 `isOpen()`, `terminate()`, `engineName()` — 太薄，无法统一模型生命周期。

**方案**:
1. 添加方法：
   ```cpp
   virtual bool load(const std::filesystem::path &modelPath,
                     ExecutionProvider provider, int deviceId,
                     std::string &errorMsg) = 0;
   virtual void unload() = 0;
   virtual int64_t estimatedMemoryBytes() const { return 0; }
   ```
2. 各引擎 (Game, Rmvpe, HFA) 实现 `load()`，将现有构造函数/`load_model()` 逻辑迁入
3. FunASR 通过适配器 (T-3.3) 实现

**风险**:
- ⚠️ **高** — 所有 3 个推理引擎 DLL 的公共 API 变更
- 缓解：可提供 `load()` 默认实现返回 false，让现有代码逐步迁移

**工作量**: M (2-8h)

---

### T-1.4 创建 OnnxModelBase 共享基类 [G8] ✅ 已完成

**现状**: `GameModel`, `RmvpeModel`, `HfaModel` 三者重复代码：
- `Ort::Session` 创建（平台 ifdef wstring vs c_str）
- `Ort::MemoryInfo` 成员 + `#ifdef _WIN_X86` 条件编译
- `is_open()` = `m_session != nullptr`
- `GameModel` 和 `RmvpeModel` 的 `terminate()`: `std::mutex m_runMutex` + `Ort::RunOptions *m_activeRunOptions`

**方案**:
1. 在 `dstools-infer-common` 新建 `OnnxModelBase`:
   ```cpp
   class OnnxModelBase {
   protected:
       std::unique_ptr<Ort::Session> m_session;
       Ort::MemoryInfo m_memoryInfo;

       // 单 session 创建
       bool loadSession(const std::filesystem::path &modelPath,
                        ExecutionProvider provider, int deviceId,
                        std::string &errorMsg);
   public:
       bool isOpen() const { return m_session != nullptr; }
       virtual ~OnnxModelBase();
   };
   ```
2. 新建 `CancellableRunMixin`（或直接作为 OnnxModelBase 的可选功能）:
   ```cpp
   class CancellableOnnxModel : public OnnxModelBase {
   protected:
       std::mutex m_runMutex;
       Ort::RunOptions *m_activeRunOptions = nullptr;
       void terminate();
   };
   ```
3. `RmvpeModel` 和 `GameModel` 继承 `CancellableOnnxModel`，`HfaModel` 继承 `OnnxModelBase`
4. `GameModel` 有多 session，`loadSession()` 调多次或提供 `loadSessionTo(unique_ptr<Session>&, path)` 辅助

**依赖**: T-1.3

**风险**:
- 中 — 各推理 DLL 内部实现变更，但公共 API 不变（基类是 protected 继承）
- `GameModel` 较特殊 (5 sessions)，需要灵活设计

**工作量**: L (1-3d)

> ✅ 已完成 — OnnxModelBase + CancellableOnnxModel 已实现。GameModel 和 RmvpeModel 继承 CancellableOnnxModel，HFA 直接实现 IInferenceEngine。loadSessionTo() 支持 GameModel 的多 session 场景。

---

### T-1.5 定义后端服务接口 [G6] ✅ 已完成

**现状**: 应用逻辑与 UI 混合。例如 pipeline-pages 中切片/对齐逻辑直接在 Qt widget 回调中执行。

**方案**:
1. 为每个核心功能定义无 Qt::Widgets 依赖的服务接口：
   - `ISlicerService` — 音频切片
   - `IAlignmentService` — 强制对齐 (HuBERT-FA)
   - `IAsrService` — 语音识别 (FunASR)
   - `IPitchService` — F0 提取 (RMVPE)
   - `ITranscriptionService` — Audio→MIDI (GAME)
2. 接口方法接受 domain 对象，返回 `Result<T>`
3. 实现类在 domain/libs 层，不依赖任何 Widget

**风险**: 低 — 新增接口，不影响现有代码

**工作量**: M (2-8h)

> ✅ 已完成 — ISlicerService、IAlignmentService、IAsrService、IPitchService、ITranscriptionService 已定义于 dsfw-core。

---

## Phase 2 — 库边界固化 (Week 6-8)

### T-2.1 整理 dsfw 目录结构与 CMake 导出 [G1]

**现状**: dsfw 各模块已在 `src/framework/` 下，但目录组织可进一步规范化，使其作为仓库内独立库清晰可辨。

**方案**:
1. 确保 `src/framework/` 下目录结构清晰：
   ```
   src/framework/
   ├── CMakeLists.txt          # 总入口，定义 DSFW_VERSION
   ├── types/                  # dsfw-types (header-only)
   ├── base/                   # dsfw-base (Qt-free, STATIC) — 若 T-1.2 引入
   ├── core/                   # dsfw-core (STATIC)
   ├── audio/                  # dsfw-audio (STATIC) — 从 src/audio/ 迁入
   ├── ui-core/                # dsfw-ui-core (STATIC)
   ├── widgets/                # dsfw-widgets (SHARED) — 通用控件，从 T-3.5 迁入
   ├── infer/                  # dsfw-infer (STATIC) — 从 src/infer/common/ 迁入
   └── tests/
   ```
2. 保留 `find_package(dsfw)` 支持（已有 `dsfwConfig.cmake.in`）
3. `src/` 顶层 CMakeLists 中 `add_subdirectory(framework)` 保持不变
4. 迁移 `src/audio/` → `src/framework/audio/`，迁移 `src/infer/common/` → `src/framework/infer/`

**依赖**: T-0.1 ~ T-1.2 完成

**风险**:
- 中 — 目录移动导致 git blame 丢失
- 缓解：使用 `git mv`，一次原子提交

**工作量**: M (2-8h)

---

### T-2.2 版本化 dsfw 公共 API [G1] ✅ 已完成

**方案**:
1. 在 `src/framework/CMakeLists.txt` 添加 `DSFW_VERSION` 宏 (`DSFW_VERSION_MAJOR.MINOR.PATCH`)
2. 所有公共符号使用 `DSFW_EXPORT` 宏（若选择 SHARED 构建）
3. 语义版本控制：breaking change = major bump

**工作量**: S (<2h)

---

### T-2.3 标准化模型配置加载 [G8] ✅ 已完成

**方案**:
在 `OnnxModelBase` 添加:
```cpp
Result<nlohmann::json> loadConfig(const std::filesystem::path &modelDir);
virtual void onConfigLoaded(const nlohmann::json &config) {}
```
各模型子类覆写 `onConfigLoaded` 解析自身参数（sample_rate、timestep 等）。

**依赖**: T-1.4

**工作量**: S (<2h)

---

### T-2.4 IInferenceEngine ↔ IModelProvider 桥接模板 [G8] ✅ 已完成

**现状**: `IInferenceEngine`（推理层）和 `IModelProvider`（框架层）是两套独立接口，应用层需手写粘合代码。

**方案**:
```cpp
template<typename Engine>
class InferenceModelProvider : public IModelProvider {
    Engine m_engine;
public:
    Result<void> load(const QString &path, int gpuIndex) override {
        std::string msg;
        auto ep = gpuIndex < 0 ? ExecutionProvider::CPU : defaultGpuProvider();
        if (!m_engine.load(path.toStdWString(), ep, gpuIndex, msg))
            return Err(QString::fromStdString(msg));
        return Ok();
    }
    // ... unload, status 转发
};
```

**依赖**: T-1.3, T-1.1

**工作量**: M (2-8h)

---

### T-2.5 FunASR 适配器 [G3+G8] ✅ 已完成

**现状**: FunASR 是 vendor 代码，有自己的 `Model` 基类，不实现 `IInferenceEngine`。

**方案**:
1. **不修改** FunASR 源码
2. 在 `src/libs/lyricfa/` 中创建 `FunAsrAdapter`:
   ```cpp
   class FunAsrAdapter : public dstools::infer::IInferenceEngine {
       std::unique_ptr<FunAsr::Model> m_model;
   public:
       bool load(...) override; // 内部调用 FunAsr::Model 构造
       bool isOpen() const override;
       const char *engineName() const override { return "FunASR"; }
   };
   ```

**风险**: 低 — 纯适配器，不触碰 vendor 代码

**工作量**: M (2-8h)

---

### T-2.6 提取 pipeline 后端逻辑 [G6] ✅ 已完成

**依赖**: T-0.4 (slicer 等独立库化), T-1.5 (服务接口)

**方案**:
1. `src/libs/slicer/` 实现 `ISlicerService`
2. `src/libs/lyricfa/` 实现 `IAsrService`
3. `src/libs/hubertfa/` 实现 `IAlignmentService`
4. pipeline-pages 变为纯 UI 层，调用 service 接口

**工作量**: L (1-3d)

> ✅ 已完成 — SlicerService（src/libs/slicer/）、AsrService（src/libs/lyricfa/）、AlignmentService（src/libs/hubertfa/）已实现，分别对应 ISlicerService、IAsrService、IAlignmentService 接口。

---

## Phase 3 — 框架增强 (Week 9-12)

### T-3.1 结构化日志系统 [G2] ✅ 已完成

**现状**: 项目使用 `qDebug()` / `qWarning()` 零散输出。

**方案**:
- `dsfw/Log.h`: severity levels (Trace/Debug/Info/Warning/Error/Fatal)
- Category 标签（如 `"infer"`, `"audio"`, `"ui"`）
- 可插拔 sink（console, file, Qt message handler 转发）
- 推荐**不引入新依赖**，用轻量自研（≤300 行）；或可选依赖 spdlog

**工作量**: M (2-8h)

> ✅ 已完成 — Logger 单例已实现，支持 6 级 severity、category 标签、可插拔 sink。

---

### T-3.2 Command / Undo-Redo 框架 [G2] ✅ 已完成

**现状**: PhonemeLabeler 和 PitchLabeler 已有自定义 undo/redo 实现。

**方案**:
- `dsfw/UndoStack.h`: `ICommand` 接口 (`execute()` / `undo()` / `text()`)
- `UndoStack` 管理器，支持合并、宏命令
- 与 `QUndoStack` 风格兼容但更轻量
- 现有 app 逐步迁移（opt-in）

**风险**: 中 — 迁移现有 undo 代码需谨慎

**工作量**: L (1-3d)

> ✅ 已完成 — ICommand + UndoStack 已实现于 dsfw-core。

---

### T-3.3 类型安全事件总线 [G2] ✅ 已完成

**方案**:
- `dsfw/EventBus.h`: 跨模块发布/订阅，不依赖 QObject
- 基于 `std::function` + type-erased event
- 用于模块间通信 (如: 推理完成通知 UI 刷新)

**工作量**: M (2-8h)

> ✅ 已完成 — EventBus 单例已实现，基于 std::any + std::type_index 的类型安全发布/订阅。

---

### T-3.4 控件稳定性审计 [G4]

**现状**: dstools-widgets 13 个控件，缺乏单元测试。

**方案**:
1. 逐个 review 13 个 widget 的 crash 路径、边界条件
2. 使用 QTest 编写关键路径测试
3. 确认导出符号正确（`DSTOOLS_WIDGETS_EXPORT` 宏）

**工作量**: M (2-8h)

---

### T-3.5 通用控件迁移到 dsfw [G4]

**现状**: 部分 widget 是通用的（如 PlayWidget、TaskWindow、ShortcutManager），可归入框架。

**方案**:
1. 分类：
   - **通用** (→ dsfw-widgets): PlayWidget, TaskWindow, ShortcutManager, ShortcutEditorWidget, PathSelector, ViewportController, RunProgressRow, FileProgressTracker
   - **DS 专有** (→ 保留 dstools-widgets): ModelLoadPanel, GpuSelector, BaseFileListPanel, FileStatusDelegate
2. 新建 `dsfw-widgets` (SHARED)，从 dstools-widgets 迁移通用控件
3. dstools-widgets 依赖 dsfw-widgets

**风险**:
- ⚠️ 中 — DLL 符号迁移影响二进制兼容
- 缓解：过渡期 dstools-widgets 转发 (re-export) 迁出的符号

**工作量**: M (2-8h)

---

### T-3.6 CLI 命令行工具 [G6]

**依赖**: T-1.2 (dsfw-base), T-2.6 (后端服务)

**方案**:
1. 新建 `src/apps/cli/` → `dstools-cli` 可执行文件
2. 链接: `dsfw-base` + `dstools-domain` + 推理库，**不链接** `Qt::Widgets`
3. 子命令: `dstools slice`, `dstools align`, `dstools extract-pitch` 等

**风险**: 低 — 新增 target，不影响现有 app

**工作量**: M (2-8h)

> ✅ 已完成 — FunAsrAdapter 已实现于 src/libs/lyricfa/，实现 IInferenceEngine 接口，通过 FunAsr::create_model() 管理模型生命周期，线程安全，不修改 vendor 源码。

---

## Phase 4 — 完善与扩展 (Week 13+)

### T-4.1 插件系统 [G2]

- `dsfw/PluginManager.h` — 基于 `QPluginLoader` 的运行时加载
- 定义 `IPlugin` 接口标准，版本化 ABI
- 用于: 模型插件、导出格式插件、G2P 插件

**工作量**: L (1-3d)

### T-4.2 崩溃收集 [G2]

- Windows: `SetUnhandledExceptionFilter` + minidump
- macOS/Linux: signal handler + stack trace
- 可选: 自动上传到 crash 收集服务

**工作量**: M (2-8h)

### T-4.3 自动更新检查 [G2] ✅ 已完成

- GitHub Releases API 轮询 + 语义版本比较
- UI 通知（不静默安装）

**工作量**: S (<2h)

> ✅ 已完成 — UpdateChecker 已实现，通过 GitHub Releases API 检查更新。

### T-4.4 MRU 最近文件列表 [G2] ✅ 已完成

- `dsfw/RecentFiles.h` — 持久化最近文件列表
- 基于 `QSettings` 后端

**工作量**: S (<2h) (trivial)

> ✅ 已完成 — RecentFiles 已实现，基于 QSettings 后端。

### T-4.5 新增标准控件 [G4]

按需实现:
- `PropertyEditor` — 属性面板
- `SettingsDialog` — 设置对话框脚手架
- `LogViewer` — 日志查看器
- `ProgressDialog` — 多任务进度对话框

**工作量**: L (1-3d)

### T-4.6 控件画廊 App [G4]

- `WidgetGallery` 测试应用，展示所有框架控件
- 附带代码示例

**工作量**: S (<2h)

### T-4.7 DI 强化：应用层模型注入 [G3]

- Apps 不再直接构造推理引擎，统一通过 `ServiceLocator` 或 typed factory 获取
- 解耦 app → 具体推理实现

**工作量**: M (2-8h)

---

## 执行顺序总览

```
Phase 0 (Week 1-2) — 预备
  T-0.1 泛化 ModelType 枚举 (✅)
  T-0.2 dsfw-core 去 Qt::Gui (✅)
  T-0.3 infer-common 去 Qt (✅)
  T-0.4 pipeline 相对路径修复 (✅)
  T-0.5 DS 泄漏审计 (✅)
      │
      ▼
Phase 1 (Week 3-5) — 核心分离
  T-1.1 ModelManager 迁移至 domain (✅)   ← depends T-0.1
  T-1.2 dsfw-core 拆分 dsfw-base (✅)     ← depends T-0.2
  T-1.3 丰富 IInferenceEngine (✅)        ← independent
  T-1.4 创建 OnnxModelBase (✅)           ← depends T-1.3
  T-1.5 定义后端服务接口 (✅)              ← independent
      │
      ▼
Phase 2 (Week 6-8) — 库边界固化
  T-2.1 整理 dsfw 目录结构与导出       ← depends Phase 1
  T-2.2 版本化 API (✅)                     ← depends T-2.1
  T-2.3 标准化模型配置加载 (✅)              ← depends T-1.4
  T-2.4 IInferenceEngine↔IModelProvider (✅) ← depends T-1.3, T-1.1
  T-2.5 FunASR 适配器 (✅)                    ← depends T-1.3
  T-2.6 提取 pipeline 后端 (✅)              ← depends T-0.4, T-1.5
      │
      ▼
Phase 3 (Week 9-12) — 增强
  T-3.1 日志系统 (✅)  T-3.2 Undo/Redo (✅)  T-3.3 事件总线 (✅)
  T-3.4 控件审计       T-3.5 控件迁移         T-3.6 CLI 工具
      │
      ▼
Phase 4 (Week 13+) — 完善
  T-4.1 插件系统    T-4.2 崩溃收集    T-4.3 更新检查 (✅)
  T-4.4 MRU 列表 (✅)  T-4.5 新增控件  T-4.6 控件画廊
  T-4.7 DI 强化
```

---

## 关键风险矩阵

| 风险 | 影响 | 概率 | 缓解措施 |
|------|------|------|---------|
| **DLL 符号迁移** — 从 dstools-widgets 移出控件到 dsfw-widgets 导致运行时链接失败 | 高 | 中 | 过渡期保留转发 stub；bump SO version；CI 链接检查全部 6 个 app |
| **头文件路径变更** — 消费者编译失败 | 高 | 中 | CMake INTERFACE include 同时暴露新旧路径；旧路径 `#warning deprecated` |
| **ModelType 泛化** — 所有 app 的模型注册代码需改动 | 高 | 高 | 提供 constexpr 别名 + 一键 sed 脚本 |
| **IInferenceEngine 接口扩展** — 3 个推理 DLL 公共 API 变更 | 中 | 高 | 新增方法提供默认实现，逐步迁移 |
| **FunASR vendor 修改** | 中 | 低 | **绝不修改** vendor 源码，仅用适配器包装 |
| **6 个 app 持续编译** — 重构过程中 app 构建中断 | 高 | 中 | 每个 task 原子提交；CI 矩阵构建全部 app；feature branch 策略 |
| **目录迁移** — git mv 导致 blame/history 不连续 | 低 | 高 | 使用 `git mv` + 原子提交；IDE 均支持 follow rename |
| **OnnxModelBase 侵入性** — GameModel 有 5 个 session，不适合单 session 基类 | 低 | 中 | `loadSessionTo(ptr, path)` 辅助方法支持多 session；基类不强制单 session |

---

## 架构决策记录 (ADR)

| ADR | 决策 | 理由 |
|-----|------|------|
| ADR-1 | dsfw-base 为 Qt-free 静态库/header-only | 支持 CLI 工具和非 Qt 消费者 |
| ADR-2 | ModelType 改为 int ID 注册表，非枚举 | 框架层不应感知业务模型类型 |
| ADR-3 | OnnxModelBase 为 protected 继承，不影响公共 API | 各推理 DLL 保持现有 API 稳定 |
| ADR-4 | audio-util 保持应用层，不入框架 | 音频 DSP 过于专业化，不属于通用桌面框架 |
| ADR-5 | dsfw-audio (FFmpeg/SDL2 封装) 归入框架 | 音频播放/解码是通用桌面能力，非 DS 专有 |
| ADR-6 | Undo/Redo 迁移 opt-in | 现有 app 按各自节奏迁移 |
| ADR-7 | FunASR 永远不直接修改 | vendor 代码只用适配器包装 |
| ADR-8 | 所有模块在本仓库内共存，不拆分独立仓库 | 降低维护复杂度，避免双仓库协调开销 |
