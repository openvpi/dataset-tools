# 任务处理器架构设计 v2

> 基于 ds-format 规范、实际代码审计和 OSS 项目参考的多模型可插拔流水线架构

---

## 1 问题陈述

### 1.1 核心矛盾

1. **规范可扩展 vs 代码硬编码**：ds-format 的 `tasks[]` 允许声明任意任务及其 I/O 绑定，但代码用 5 个固定接口 (IAlignmentService, IAsrService, ...) 和 `DsProjectDefaults` 的 4 个硬编码路径字段实现。

2. **基础设施闲置**：`ModelManager` / `IModelProvider` / `InferenceModelProvider<Engine>` 完整实现了 LRU 淘汰和类型安全注册，但所有服务都绕过它，各自用 `unique_ptr<Engine>` + `mutex` 管理模型。

3. **双数据模型并存**：交互式 UI 使用 `.dstext` 的 `LayerData` (boundaries)，批处理流水线使用 `TranscriptionRow` (CSV 行)。两者间无转换桥梁。

### 1.2 实际代码现状（审计结论）

| 服务 | 实现 | 关键发现 |
|------|------|---------|
| AlignmentService (HuBERT) | `src/libs/hubertfa/` | `alignCSV()` 返回 "not supported"（死方法）；有非接口方法 `setLanguage()`、`setNonSpeechPhonemes()` |
| GameTranscriptionService | `src/libs/gameinfer/` | 同时实现 ITranscriptionService + IAlignmentService 两个接口；`exportMidi()` 直接写 MIDI 文件 |
| GameInferService (app) | `src/apps/GameInfer/` | 额外 5 个配置方法 (segThreshold, segRadius, estThreshold, d3pmTimesteps, language)，UI 通过 `dynamic_cast` 调用 |
| RmvpePitchService | `src/libs/rmvpepitch/` | 两个方法返回不同数据形状：`extractPitch()` 返回扁平 f0，`extractPitchWithProgress()` 返回分块 f0 + 输出参数 |
| AsrService (FunASR) | `src/libs/lyricfa/` | `AsrResult.segments` 从未填充，只返回 text |
| SlicerService | `src/libs/slicer/` | 纯算法，无状态，无模型 |

**UI 耦合现状**：

| 消费者 | 使用方式 | 问题 |
|--------|---------|------|
| GameAlignPage | `ServiceLocator::get<IAlignmentService>()` → `alignCSV()` | 正常 |
| HubertFAPage | `ServiceLocator::get<IAlignmentService>()` → `loadModel()`, `align()`, `vocabInfo()` | `vocabInfo()` 用于动态构建 UI |
| GameInfer/MainWidget | `dynamic_cast<GameInferService*>` 调用 5 个非接口 setter | 类型系统被绕过 |
| LyricFAPage | **绕过 IAsrService**，直接创建 `LyricFA::Asr` 引擎 | 服务层未接入 |
| SlicerPage | **绕过 ISlicerService**，直接用 WorkThread/SliceJob | 服务层未接入 |
| BuildCsvPage, BuildDsPage | 直接调用 domain 类，无服务层 | 这些步骤无模型，不需要服务抽象 |
| CLI | 全部通过 ServiceLocator，但只注册了 SlicerService | 4/5 命令运行时失败 |

---

## 2 OSS 参考架构

| 项目 | 语言 | 关键模式 | 取用点 |
|------|------|---------|--------|
| **Essentia** | C++ | Algorithm 基类 + 命名 I/O 端口 + `Configurable` 参数系统 + 单例工厂自注册 | I/O 端口模型、参数与计算分离 |
| **ESPnet** | Python | `ClassChoices` 注册表 + `Abs*` 抽象基类 + config 驱动选择 | 后端选择绑定配置文件 |
| **DiffSinger** | Python | `filter_kwargs` 适配异构构造参数 + 双层可插拔 (算法×骨干) | 处理异构引擎配置 |
| **SOFA** | Python | 模板方法：基类 `__call__` 校验不变量，子类实现 `_impl` | I/O 契约校验 |
| **MFA** | Python | Mixin 组合能力 + `MfaModel.get_pretrained_path()` 模型发现 | 模型生命周期管理 |

**核心经验**：分离三个关注点 — **注册**（如何发现后端）、**选择**（配置如何指定后端）、**I/O 契约**（阶段间如何连接）。

---

## 3 修订设计

### 3.1 放弃纯 LayerData I/O — 引入双模式处理

v1 设计假设所有处理器都是 `LayerData → LayerData` 变换。实际代码表明这不成立：

- `alignCSV()` 和 `exportMidi()` 是**文件到文件**操作
- `TranscriptionPipeline` 操作 `TranscriptionRow`，不是 `LayerData`
- F0 提取的输出是浮点数组，不是 boundary 序列（需等 `curve` 层类型支持）
- GAME 的 `alignCSV()` 直接由引擎处理 CSV → CSV

**修正方案**：处理器支持两种调用模式 — **单条处理** (process) 和**批量处理** (processBatch)。单条模式用于交互式 UI，批量模式用于流水线。

### 3.2 核心类型

```cpp
/// 处理器的引擎特定配置（替代 dynamic_cast + 非接口 setter）
/// 参考 Essentia 的 Configurable + DiffSinger 的 filter_kwargs
using ProcessorConfig = nlohmann::json;

/// 单条处理的输入（交互式模式）
struct TaskInput {
    QString audioPath;
    std::map<QString, LayerData> layers;   // slot → layer data
    ProcessorConfig config;                // 引擎特定参数
};

/// 单条处理的输出
struct TaskOutput {
    std::map<QString, LayerData> layers;   // slot → layer data
};

/// 批量处理的输入（流水线模式）
struct BatchInput {
    QString workingDir;                    // 工程根目录
    ProcessorConfig config;                // 引擎特定参数
    // 批量模式下，处理器自行读取所需文件
};

/// 批量处理的输出
struct BatchOutput {
    int processedCount = 0;
    int failedCount = 0;
    QString outputPath;                    // 产出文件路径
};

/// 进度回调（参考 SOFA 的 tqdm 模式）
using ProgressCallback = std::function<void(int current, int total, const QString &item)>;
```

### 3.3 ITaskProcessor — 修订版

```cpp
/// 参考 Essentia Algorithm + SOFA BaseG2P 的模板方法
class ITaskProcessor {
public:
    virtual ~ITaskProcessor() = default;

    // ── 元数据 ──────────────────────────────────────────────

    /// 处理器唯一标识（如 "hubert-fa", "sofa", "rmvpe"）
    virtual QString processorId() const = 0;

    /// 人类可读名称
    virtual QString displayName() const = 0;

    /// I/O 规格声明（与 .dsproj tasks[] 对应）
    virtual TaskSpec taskSpec() const = 0;

    /// 处理器能力声明
    /// 返回 JSON 描述：支持的参数及其类型/范围/默认值
    /// UI 据此动态生成配置控件（替代 vocabInfo() 等专用查询方法）
    virtual ProcessorConfig capabilities() const { return {}; }

    // ── 模型生命周期 ────────────────────────────────────────

    /// 初始化：通过 ModelManager 注册并加载模型
    virtual Result<void> initialize(IModelManager &mm,
                                    const ProcessorConfig &modelConfig) = 0;

    /// 释放处理器内部状态（模型由 ModelManager 统一管理卸载）
    virtual void release() = 0;

    // ── 处理 ────────────────────────────────────────────────

    /// 单条处理（交互式模式）
    /// 输入输出为 LayerData（对应 .dstext 的层结构）
    virtual Result<TaskOutput> process(const TaskInput &input) = 0;

    /// 批量处理（流水线模式）
    /// 默认实现：逐条调用 process()。引擎可覆盖以使用原生批量接口。
    /// 例如 GAME 的 alignCSV() 直接操作 CSV 文件，比逐条处理高效。
    virtual Result<BatchOutput> processBatch(const BatchInput &input,
                                             ProgressCallback progress = nullptr);
};
```

`capabilities()` 解决了 `vocabInfo()` 和 `dynamic_cast` 问题 — 处理器声明自己支持的参数，UI 据此动态构建控件：

```json
// HubertAlignmentProcessor::capabilities() 返回：
{
    "language": {"type": "enum", "values": ["zh", "ja", "en"], "default": "zh"},
    "nonSpeechPhonemes": {"type": "stringList", "default": ["AP", "SP"]}
}

// GameMidiProcessor::capabilities() 返回：
{
    "segThreshold": {"type": "float", "min": 0.0, "max": 1.0, "default": 0.3},
    "segRadiusFrames": {"type": "float", "min": 0, "max": 50, "default": 3.0},
    "estThreshold": {"type": "float", "min": 0.0, "max": 1.0, "default": 0.5},
    "d3pmTimesteps": {"type": "int", "min": 1, "max": 1000, "default": 100},
    "language": {"type": "enum", "values": ["auto"], "dynamic": true}
}
```

参数通过 `TaskInput.config` / `BatchInput.config` 传入，处理器在 `process()` / `processBatch()` 中读取。

### 3.4 TaskProcessorRegistry — 自注册工厂

参考 Essentia 的 `EssentiaFactory::Registrar` 模式：

```cpp
using ProcessorFactory = std::function<std::unique_ptr<ITaskProcessor>()>;

class TaskProcessorRegistry {
public:
    void registerProcessor(const QString &taskName,
                           const QString &processorId,
                           ProcessorFactory factory);

    std::unique_ptr<ITaskProcessor> create(const QString &taskName,
                                            const QString &processorId) const;

    QStringList availableProcessors(const QString &taskName) const;

    static TaskProcessorRegistry &instance();

    /// 自注册辅助（参考 Essentia Registrar）
    /// 用法：在 .cpp 文件中放置静态实例即可自动注册
    template <typename ProcessorType>
    struct Registrar {
        Registrar(const QString &taskName, const QString &processorId) {
            TaskProcessorRegistry::instance().registerProcessor(
                taskName, processorId,
                []{ return std::make_unique<ProcessorType>(); });
        }
    };

private:
    std::map<QString, std::map<QString, ProcessorFactory>> m_registry;
};

// 用法 — 在 HubertAlignmentProcessor.cpp 末尾：
static TaskProcessorRegistry::Registrar<HubertAlignmentProcessor>
    s_reg("phoneme_alignment", "hubert-fa");
```

### 3.5 DsProjectDefaults 演进

```cpp
struct TaskModelConfig {
    QString processorId;        // "hubert-fa", "rmvpe", "game", "funasr"
    QString modelPath;
    QString provider = "cpu";   // cpu / dml / cuda
    int deviceId = 0;
    ProcessorConfig extra;      // 引擎特定参数
};

struct DsProjectDefaults {
    std::map<QString, TaskModelConfig> taskModels;  // key = task name
    int hopSize = 512;
    int sampleRate = 44100;
};
```

.dsproj JSON 格式：

```json
{
    "defaults": {
        "models": {
            "phoneme_alignment": {
                "processor": "hubert-fa",
                "path": "models/hubert_fa.onnx",
                "provider": "dml",
                "deviceId": 0,
                "language": "zh",
                "nonSpeechPhonemes": ["AP", "SP"]
            },
            "midi_transcription": {
                "processor": "game",
                "path": "models/game/",
                "segThreshold": 0.3,
                "d3pmTimesteps": 100
            }
        }
    }
}
```

**向后兼容**：解析时旧 key (`asr`/`alignment`/`midi`/`build_ds`) 自动映射到新 task name，`processor` 缺省时按旧引擎名推断。

### 3.6 处理器实现修订

#### RmvpePitchProcessor

```cpp
class RmvpePitchProcessor : public ITaskProcessor {
    ModelTypeId m_typeId;
    IModelManager *m_mm = nullptr;

public:
    QString processorId() const override { return "rmvpe"; }
    QString displayName() const override { return "RMVPE F0 Extraction"; }

    TaskSpec taskSpec() const override {
        // F0 输出暂用 pitch category，待 curve 层类型实现后切换
        return {"pitch_analysis", {}, {{"pitch", "pitch"}}};
    }

    Result<void> initialize(IModelManager &mm, const ProcessorConfig &config) override {
        m_mm = &mm;
        m_typeId = registerModelType("rmvpe");

        if (!mm.provider(m_typeId)) {
            mm.registerProvider(m_typeId,
                std::make_unique<InferenceModelProvider<Rmvpe::Rmvpe>>(
                    m_typeId, "RMVPE"));
        }

        QString path = QString::fromStdString(config.value("path", ""));
        int gpu = config.value("deviceId", -1);
        if (config.value("provider", "cpu") == "cpu") gpu = -1;
        return mm.ensureLoaded(m_typeId, path, gpu);
    }

    Result<TaskOutput> process(const TaskInput &input) override {
        auto *prov = dynamic_cast<InferenceModelProvider<Rmvpe::Rmvpe>*>(
            m_mm->provider(m_typeId));
        if (!prov || !prov->isReady())
            return Err<TaskOutput>("RMVPE model not loaded");

        // 调用引擎
        std::vector<Rmvpe::RmvpeRes> res;
        auto r = prov->engine().get_f0(
            input.audioPath.toStdWString(), 0.03f, res, nullptr);
        if (!r) return Err<TaskOutput>(r.error());

        // 将 f0 转为 pitch layer（临时方案：每帧一个 boundary）
        TaskOutput out;
        LayerData layer;
        layer.category = "pitch";
        // ... F0 → boundary 转换逻辑
        out.layers["pitch"] = std::move(layer);
        return Ok(std::move(out));
    }

    /// 批量模式：逐文件提取 F0，写入 dstemp/f0/
    Result<BatchOutput> processBatch(const BatchInput &input,
                                     ProgressCallback progress) override {
        // 遍历 wavsDir，逐文件调用引擎，写入 f0 缓存
        // 返回处理计数
    }

    void release() override {}
};
```

#### GameMidiProcessor — 关键修订

GAME 引擎用 `shared_ptr<Game::Game>` 封装，**不**拆成 4 个独立 ModelTypeId — 因为 `Game::Game` 内部管理子模型的加载/协调，外部拆分会破坏其内部不变量。

```cpp
class GameMidiProcessor : public ITaskProcessor {
    ModelTypeId m_typeId;
    IModelManager *m_mm = nullptr;

public:
    QString processorId() const override { return "game"; }
    QString displayName() const override { return "GAME Audio-to-MIDI"; }

    TaskSpec taskSpec() const override {
        return {"midi_transcription",
                {{"grapheme", "grapheme"}},
                {{"pitch", "pitch"}}};
    }

    ProcessorConfig capabilities() const override {
        return {
            {"segThreshold",   {{"type", "float"}, {"min", 0}, {"max", 1}, {"default", 0.3}}},
            {"segRadiusFrames",{{"type", "float"}, {"min", 0}, {"max", 50}, {"default", 3}}},
            {"estThreshold",   {{"type", "float"}, {"min", 0}, {"max", 1}, {"default", 0.5}}},
            {"d3pmTimesteps",  {{"type", "int"}, {"min", 1}, {"max", 1000}, {"default", 100}}},
            {"language",       {{"type", "int"}, {"default", 0}}}
        };
    }

    Result<void> initialize(IModelManager &mm, const ProcessorConfig &config) override {
        // 注册为单个 ModelTypeId
        // Game::Game 内部管理多个子模型，不需要外部拆分
        m_typeId = registerModelType("game");
        // ... 注册 InferenceModelProvider 或自定义 GameModelProvider
    }

    Result<TaskOutput> process(const TaskInput &input) override {
        // 1. 从 config 读取引擎参数（替代 dynamic_cast setter）
        // 2. 从 input.layers["grapheme"] 读取音节/音素序列
        // 3. 调用 Game::Game::align() 或 get_notes()
        // 4. 转换为 pitch layer 输出
    }

    /// 批量模式：直接使用 Game::alignCSV()（原生批量接口）
    Result<BatchOutput> processBatch(const BatchInput &input,
                                     ProgressCallback progress) override {
        // 直接调用 m_game->alignCSV() — 比逐条 process() 高效
        // Game 引擎原生支持 CSV 批量处理
    }
};
```

#### HubertAlignmentProcessor — 带 capabilities

```cpp
class HubertAlignmentProcessor : public ITaskProcessor {
    ModelTypeId m_typeId;
    IModelManager *m_mm = nullptr;
    std::string m_language = "zh";
    std::vector<std::string> m_nonSpeechPh = {"AP", "SP"};

public:
    QString processorId() const override { return "hubert-fa"; }

    ProcessorConfig capabilities() const override {
        // 替代 vocabInfo() — UI 据此生成语言选择器和音素复选框
        return {
            {"language", {{"type", "enum"}, {"values", {"zh", "ja", "en"}}, {"default", "zh"}}},
            {"nonSpeechPhonemes", {{"type", "stringList"}, {"default", {"AP", "SP"}}}}
        };
    }

    Result<void> initialize(IModelManager &mm, const ProcessorConfig &config) override {
        // 从 config 读取 language 和 nonSpeechPhonemes
        if (config.contains("language"))
            m_language = config["language"].get<std::string>();
        if (config.contains("nonSpeechPhonemes"))
            for (auto &ph : config["nonSpeechPhonemes"])
                m_nonSpeechPh.push_back(ph.get<std::string>());
        // ... 注册模型并加载
    }

    Result<TaskOutput> process(const TaskInput &input) override {
        // 从 input.layers["grapheme"] 提取音素序列
        // 调用 HFA::HFA::recognize() with m_language, m_nonSpeechPh
        // 输出 phoneme layer (带时间戳的音素序列)
    }

    /// HuBERT 无原生批量接口 — 用默认逐条实现
    // processBatch() 不覆盖，使用基类默认
};
```

### 3.7 TranscriptionPipeline 集成

现有的 `TranscriptionPipeline` 操作 `TranscriptionRow`，这是批量模式的内部表示。与 `ITaskProcessor` 的关系：

```
PipelineRunner (新)
  │
  ├─ 切片步骤: ISlicerService (保留独立)
  ├─ ASR 步骤:  ITaskProcessor("asr").processBatch()
  ├─ 对齐步骤:  ITaskProcessor("phoneme_alignment").processBatch()
  ├─ CSV 步骤:  TranscriptionPipeline::extractTextGrids() (domain 工具，不需要处理器)
  ├─ MIDI 步骤: ITaskProcessor("midi_transcription").processBatch()
  ├─ DS 步骤:   CsvToDsConverter (domain 工具)
  └─ F0 步骤:   ITaskProcessor("pitch_analysis").processBatch()
```

**`TranscriptionRow` 不需要替换为 `LayerData`** — 它是 CSV 中间格式，而 `LayerData` 是 `.dstext` 的结构化标注。两者服务不同场景：

- `LayerData`: 交互式编辑（PhonemeLabeler, PitchLabeler），持久化到 `.dstext`
- `TranscriptionRow`: 批量流水线的内存表示，持久化到 CSV

处理器的 `processBatch()` 内部可以自由选择使用 `TranscriptionRow`、直接操作 CSV、或逐条调用 `process()`。

---

## 4 现有接口处置（修订版）

### 保留

| 接口 | 理由 | 调整 |
|------|------|------|
| `ModelManager` / `IModelManager` | 统一模型生命周期 | 瘦身：降级 6 个仅内部使用的虚方法 |
| `IModelProvider` / `ModelTypeId` | 可扩展模型注册 | 不变 |
| `InferenceModelProvider<Engine>` | 引擎→Provider 适配器 | 不变 |
| `IInferenceEngine` | ONNX 引擎抽象 | 不变 |
| `IG2PProvider` | 对齐处理器的注入依赖 | 处理器工厂注入，不通过 TaskInput |
| `IExportFormat` | 后处理导出 | 保留，接线到 DiffSingerLabeler |
| `IQualityMetrics` | 输出校验 | 保留，接线到 PipelineRunner |
| `IModelDownloader` | 模型下载 | 保留，接线到 initialize() |
| `ISlicerService` | 纯算法无模型 | 保留独立 |
| `IFileIOProvider` | 文件 I/O mock | 不变 |
| `IDocument` | 文档模型 | 删除 `format()` 方法和 `IDocumentFormat` |
| `IPageActions` | 页面行为 | 删除 4 个死方法 |
| `IPageLifecycle` | 页面生命周期 | 激活 `onShutdown`/`onWorkingDirectoryChanged` |

### 删除

| 接口 | 理由 |
|------|------|
| `IAlignmentService` | 被 ITaskProcessor 替代 |
| `IAsrService` | 被 ITaskProcessor 替代 |
| `IPitchService` | 被 ITaskProcessor 替代 |
| `ITranscriptionService` | 被 ITaskProcessor 替代 |
| `IDocumentFormat` | 零实现 |
| `EventBus` | 零消费者 |
| `PluginManager` / `IStepPlugin` | 被 TaskProcessorRegistry 替代 |
| `UpdateChecker` | 零消费者 |
| `RecentFiles` | 零消费者 |
| `dsfw::UndoStack` / `dsfw::ICommand` | app 用 QUndoStack |
| `IPageProgress` | AppShell 从未 query |

---

## 5 与 ds-format 规范的映射

```
.dsproj schema[]                      ←→  LayerData 的类型/关系元数据
.dsproj tasks[]                       ←→  TaskSpec (ITaskProcessor::taskSpec())
.dsproj defaults.models{}.processor   ←→  TaskProcessorRegistry 的 processorId
.dsproj defaults.models{}.path/etc    ←→  ProcessorConfig (传入 initialize)
.dsproj defaults.models{}.{extra}     ←→  ProcessorConfig (传入 process/processBatch)
.dstext layers[]                      ←→  TaskInput/TaskOutput 中的 LayerData
dstemp/{step}/                        ←→  processBatch() 的 I/O 目录
dstemp/{step}/*.dsitem                ←→  PipelineRunner 记录 processor + model + timestamp
```

---

## 6 设计决策记录

| ADR | 决策 | 理由 |
|-----|------|------|
| ADR-10 | ITaskProcessor 替代 4 个 per-domain 服务接口 | tasks[] 已定义 I/O 契约，per-domain 接口冗余 |
| ADR-11 | 所有模型通过 ModelManager 管理 | 统一 LRU 淘汰，消除并行的模型生命周期 |
| ADR-12 | ITaskProcessor 支持 process() + processBatch() 双模式 | 交互式 UI 需要单条处理，流水线需要批量文件处理；部分引擎有原生批量接口 (GAME alignCSV) |
| ADR-13 | capabilities() 替代 vocabInfo() 和 dynamic_cast setter | 统一的参数声明机制，UI 动态生成控件，参数通过 config JSON 传入 |
| ADR-14 | GAME 引擎注册为单个 ModelTypeId | Game::Game 内部管理子模型协调，外部拆分破坏内部不变量 |
| ADR-15 | ISlicerService 保持独立 | 纯算法无模型无 layer I/O |
| ADR-16 | TranscriptionRow 与 LayerData 并存 | 两者服务不同场景（CSV 批量 vs dstext 交互式），强行统一增加不必要的转换层 |
| ADR-17 | DsProjectDefaults 用 map 替代硬编码字段 | 新增 task/processor 无需改 C++ 结构体 |
| ADR-18 | 自注册工厂 (Essentia Registrar 模式) | 新增处理器只需在 .cpp 放一行静态注册，无需修改中心注册表 |
| ADR-19 | G2P 是处理器依赖而非 task | G2P 不产出 layer，是对齐处理器的内部实现细节 |
| ADR-20 | 并存迁移 | 每个处理器独立迁移，旧服务接口直到所有消费者迁移完毕才删除 |
