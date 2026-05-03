# Phase L 细化实施方案

> 本文档是 [refactoring-roadmap.md](refactoring-roadmap.md) Phase L 的逐任务实施指南。
> 每个任务给出精确的文件路径、接口签名、实现要点和验收标准。

---

### L.0 — 时间类型基础设施 + 精度回归测试 ✅ (已完成，详见代码)

---

### L.0b — 曲线插值工具库 (CurveTools) ✅ (已完成，详见代码)

---

### L.1 — Boundary / Layer 领域模型 + .dstext I/O ✅ (已完成，详见代码)

---

### L.2 — PipelineContext + IAudioPreprocessor + IFormatAdapter ✅ (已完成，详见代码)

---

### L.3 — 4 个格式适配器 ✅ (已完成，详见代码)

---

### L.4 — PipelineRunner ✅ (已完成，详见代码)

---

### L.5 — 新增处理器包装 ✅ (已完成，详见代码)

---

### L.6 — processBatch 移除 ✅ (已完成，详见代码)

---

### L.7 — F0Curve / DSFile 迁移 ✅ (已完成，详见代码)

---

### L.8 — PhonemeLabeler 时间迁移 ✅ (已完成，详见代码)

---

## L.11 — DsProject 扩展（精确修改清单）

**当前 `DsProjectDefaults`**：

```cpp
struct DsProjectDefaults {
    QString asrModelPath;       // ← 遗留，L.10 删
    QString hubertModelPath;    // ← 遗留，L.10 删
    QString gameModelPath;      // ← 遗留，L.10 删
    QString rmvpeModelPath;     // ← 遗留，L.10 删
    int gpuIndex = -1;          // ← 遗留，L.10 删
    int hopSize = 512;
    int sampleRate = 44100;
    std::map<QString, TaskModelConfig> taskModels;
};
```

**新增字段**：

```cpp
struct PreprocessorConfig {
    QString id;
    nlohmann::json params;
};

struct SlicerConfig {
    double threshold = -40.0;
    int minLength = 5000;       // ms
    int minInterval = 300;      // ms
    int hopSize = 10;           // ms
    int maxSilKept = 500;       // ms
    int maxSliceLength = 15000; // ms
    QString namingPrefix = "auto";
};

struct ValidationConfig {
    double minSliceLength = 0.3;   // seconds
    double maxSliceLength = 30.0;
    double minPitchCoverage = 0.5;
};

struct ExportConfig {
    QStringList formats = {"csv", "ds"};
    int dsHopSize = 512;
    int dsSampleRate = 44100;
    bool includeDiscarded = false;
};

struct SliceInfo {
    QString id;
    TimePos in = 0;
    TimePos out = 0;
    QString language;          // optional override
    QString status = "active"; // active / discarded / error
    QString discardReason;
    QString discardedAtStep;
};

// schema / tasks 用 nlohmann::json 存储（不硬编码结构，保持灵活性）

struct DsProjectDefaults {
    // 遗留字段（L.10 删除）...
    int hopSize = 512;
    int sampleRate = 44100;
    std::map<QString, TaskModelConfig> taskModels;

    // 新增
    std::vector<PreprocessorConfig> preprocessors;
    SlicerConfig slicer;
    ValidationConfig validation;
    ExportConfig exportConfig;
};
```

**DsProject 新增**：

```cpp
class DsProject {
    // ... existing ...

    // 新增
    nlohmann::json schema() const;
    void setSchema(const nlohmann::json &schema);

    nlohmann::json tasks() const;
    void setTasks(const nlohmann::json &tasks);

    // items 扩展：slices 含 status/discardReason
    // 通过 m_extraFields 保留
};
```

---

### L.12 — 编译速度优化 ✅ (已完成，详见代码)

---

## 验收标准总表

| 阶段 | 编译通过 | 测试通过 | 现有功能不退化 | 状态 |
|------|---------|---------|--------------|------|
| L.0 | ✓ 纯新增 | TestTimePos 全绿 | 无影响 | ✅ |
| L.0b | ✓ 纯新增 | TestCurveTools 全绿 (19 用例) | 无影响 | ✅ |
| L.1 | ✓ 纯新增 | TestDsTextDocument 全绿 | 无影响 | ✅ |
| L.2 | ✓ 纯新增 | TestPipelineContext 全绿 | 无影响 | ✅ |
| L.3 | ✓ 纯新增 | TestFormatAdapters 全绿 | 无影响 | ✅ |
| L.4 | ✓ 纯新增 | TestPipelineRunner 全绿 | 无影响 | ✅ |
| L.5 | ✓ 纯新增 | TestSlicerProcessor + TestAddPhNumProcessor 全绿 | 无影响 | ✅ |
| L.6 | ✓ 编译修复 | 所有现有 24 测试 + 更新后测试全绿 | GameInfer/HubertFA/CLI 功能等价 | ✅ |
| L.7 | ✓ 编译修复 | TestF0Curve 适配后全绿 | PitchLabeler 功能等价 | ✅ |
| L.8 | ✓ 编译修复 | 手动验证 PhonemeLabeler 边界拖动/绑定 | 拖动/绑定行为不变 | ✅ |
| L.9 | ✓ 编译修复 | 手动验证 DiffSingerLabeler 全流程 | 9 步 wizard 功能等价 | ✅ |
| L.10 | ✓ deprecated 警告 | 所有测试全绿 | 无功能退化 | ✅ |
| L.11 | ✓ 编译修复 | TestDsProject 全绿 | .dsproj 读写兼容 | ✅ |
| L.12 | ✓ 编译修复 | 所有现有测试全绿 | 编译产物二进制一致（除调试信息格式） | ✅ |

---

> 更新时间：2026-05-03
