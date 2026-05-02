# Phase L 细化实施方案

> 本文档是 [refactoring-roadmap.md](refactoring-roadmap.md) Phase L 的逐任务实施指南。
> 每个任务给出精确的文件路径、接口签名、实现要点和验收标准。

---

## L.0 — 时间类型基础设施 + 精度回归测试

### L.0.1 引入 TimePos 类型定义

**文件**：`src/types/include/dstools/TimePos.h`（新建）

```cpp
#pragma once

/// @file TimePos.h
/// @brief Integer microsecond time representation for zero-drift precision.

#include <cstdint>
#include <cmath>

namespace dstools {

/// All time positions and durations in microseconds (1 s = 1'000'000 μs).
/// Eliminates floating-point accumulation errors across boundary binding and I/O.
using TimePos = int64_t;

/// 1 second in TimePos units.
inline constexpr TimePos kMicrosecondsPerSecond = 1'000'000;

/// 1 millisecond in TimePos units.
inline constexpr TimePos kMicrosecondsPerMs = 1'000;

/// Convert floating-point seconds to TimePos (rounds to nearest μs).
inline TimePos secToUs(double sec) {
    return static_cast<TimePos>(std::llround(sec * 1e6));
}

/// Convert TimePos to floating-point seconds.
inline double usToSec(TimePos us) {
    return static_cast<double>(us) / 1e6;
}

/// Convert TimePos to floating-point milliseconds.
inline double usToMs(TimePos us) {
    return static_cast<double>(us) / 1e3;
}

/// Convert floating-point milliseconds to TimePos.
inline TimePos msToUs(double ms) {
    return static_cast<TimePos>(std::llround(ms * 1e3));
}

// ── F0 value helpers ──

/// Convert Hz (double) to millihertz (int32_t). 261.6 Hz → 261600 mHz.
inline int32_t hzToMhz(double hz) {
    return static_cast<int32_t>(std::lround(hz * 1000.0));
}

/// Convert millihertz (int32_t) to Hz (double). 261600 → 261.6.
inline double mhzToHz(int32_t mhz) {
    return static_cast<double>(mhz) / 1000.0;
}

/// Format TimePos as seconds string with 6 decimal places ("1.200000").
/// For MakeDiffSinger CSV/DS compatibility.
QString timePosToSecString(TimePos us);

/// Parse seconds string ("1.200000") to TimePos. Returns 0 on failure.
TimePos secStringToTimePos(const QString &str);

} // namespace dstools
```

**实现**：`src/types/src/TimePos.cpp`（新建）— 仅 `timePosToSecString` / `secStringToTimePos` 两个非 inline 函数。

**CMake**：`src/types/CMakeLists.txt` 从 INTERFACE 改为 STATIC（新增 .cpp 文件），或将两个函数也做 inline（用 `QString::number`）。推荐保持 INTERFACE + header-only：

```cpp
// header-only 方案：
inline QString timePosToSecString(TimePos us) {
    return QString::number(usToSec(us), 'f', 6);
}

inline TimePos secStringToTimePos(const QString &str) {
    bool ok = false;
    double sec = str.toDouble(&ok);
    return ok ? secToUs(sec) : 0;
}
```

**验收**：编译通过，现有代码不受影响（纯新增头文件）。

---

### L.0.2 精度回归测试

**文件**：`src/tests/types/TestTimePos.cpp`（新建）

**测试用例**：

| 用例 | 输入 | 期望输出 | 验证点 |
|------|------|---------|--------|
| `sec_roundtrip` | `1.200000` | `secToUs(1.2) == 1200000`, `usToSec(1200000) == 1.2` | 6 位小数往返无损 |
| `mds_compat` | MDS 格式 `"0.080000"` | `secStringToTimePos("0.080000") == 80000` | MDS CSV 兼容 |
| `zero` | `0.0` | `secToUs(0.0) == 0` | 零值 |
| `negative` | `-1.5` | `secToUs(-1.5) == -1500000` | 负值（偏移量） |
| `large` | `3600.0` (1小时) | `secToUs(3600.0) == 3600000000` | 大值不溢出 |
| `sub_us_rounding` | `0.0000005` (0.5 μs) | `secToUs(0.0000005) == 1` (四舍五入) | 亚微秒四舍五入 |
| `f0_roundtrip` | `261.6 Hz` | `hzToMhz(261.6) == 261600`, `mhzToHz(261600) == 261.6` | F0 往返 |
| `f0_zero` | `0.0 Hz` (unvoiced) | `hzToMhz(0.0) == 0` | 无声帧 |
| `ms_roundtrip` | `1.0 ms` | `msToUs(1.0) == 1000`, `usToMs(1000) == 1.0` | 毫秒往返 |

---

### L.0.3 — 已合并到 L.0.1（hzToMhz/mhzToHz 在同一头文件中）

---

## L.0b — 曲线插值工具库 (CurveTools)

### 背景

不同 F0 模型输出的 hop_size 和 sample_rate 各不相同：

| 模型 | 内部采样率 | hop_size | 原始 timestep | 备注 |
|------|-----------|----------|--------------|------|
| RMVPE | 16000 | 160 | 10.0 ms | 固定 10ms，需重采样到目标 timestep |
| FCPE | 16000 | 160 | 10.0 ms | 同 RMVPE |
| Parselmouth | 任意 | — | ~8.0 ms (FMIN dependent) | Praat 算法，timestep 由参数决定 |
| CREPE | 16000 | 160 | 10.0 ms | — |
| DiffSinger 训练 | 44100 | 512 | 11.61 ms | 最终目标 timestep |

MakeDiffSinger 的 `get_pitch.py` 中有关键函数 `resample_align_curve`，把模型原始 timestep 重采样到目标 timestep 并对齐到指定长度。**当前 C++ 代码完全缺失此功能**——RMVPE 输出原始 f0 chunk 后无重采样步骤。

此外还需要：
- **无声帧插值**：F0=0 的帧需要在 log 域内线性插值（已有 `Rmvpe.cpp:interp_f0`，但仅限 RMVPE 内部、非通用）
- **曲线平滑**：移动平均、中值滤波（`PitchProcessor::movingAverage` 已有但耦合在 PitchLabeler 内部）
- **频域转换**：Hz ↔ log2(Hz) ↔ MIDI ↔ mHz（部分已有于 `PitchUtils.h`）

### L.0b.1 CurveTools 头文件

**文件**：`src/domain/include/dstools/CurveTools.h`（新建）

```cpp
#pragma once

/// @file CurveTools.h
/// @brief Curve interpolation, resampling, and processing utilities.
///
/// Designed for F0 curves but applicable to any equally-spaced time-series data.
/// All time values in microseconds (TimePos). All F0 values in millihertz (int32_t).

#include <dstools/TimePos.h>
#include <vector>
#include <cstdint>

namespace dstools {

// ═══════════════════════════════════════════════════════════════════
// 1. Resampling — 不同 timestep 间的曲线变换
// ═══════════════════════════════════════════════════════════════════

/// @brief 将等间隔采样曲线从源 timestep 重采样到目标 timestep，并对齐到指定长度。
///
/// 等价于 MakeDiffSinger get_pitch.py 的 resample_align_curve()。
/// 使用线性插值。超出源范围的尾部用末尾值填充。
///
/// @param values       源采样值序列
/// @param srcTimestep  源 timestep（微秒）
/// @param dstTimestep  目标 timestep（微秒）
/// @param alignLength  目标输出长度（采样点数）。
///                     若为 0，自动计算：ceil(duration / dstTimestep)。
/// @return 重采样后的值序列，长度 = alignLength。
///
/// 用例：RMVPE 输出 10ms timestep → 重采样到 11.61ms (44100/512)
///       resampleCurve(rmvpeF0, secToUs(0.01), hopSizeToTimestep(512, 44100), targetLen)
std::vector<int32_t> resampleCurve(const std::vector<int32_t> &values,
                                    TimePos srcTimestep,
                                    TimePos dstTimestep,
                                    int alignLength = 0);

/// @brief 同上，double 版本（用于 Hz 域 F0 或其他浮点曲线）。
std::vector<double> resampleCurveF(const std::vector<double> &values,
                                    TimePos srcTimestep,
                                    TimePos dstTimestep,
                                    int alignLength = 0);

/// @brief 根据 hop_size 和 sample_rate 计算 timestep（微秒）。
/// @param hopSize   跳步大小（采样点数）
/// @param sampleRate 采样率（Hz）
/// @return timestep = hopSize / sampleRate * 1e6（微秒）
inline TimePos hopSizeToTimestep(int hopSize, int sampleRate) {
    return static_cast<TimePos>(
        std::llround(static_cast<double>(hopSize) / sampleRate * 1e6));
}

/// @brief 根据音频总采样数和 hop_size 计算期望的 F0 帧数。
/// @param audioSamples 音频总采样点数
/// @param hopSize      跳步大小
/// @return 帧数 = ceil((audioSamples + hopSize - 1) / hopSize)
inline int expectedFrameCount(int64_t audioSamples, int hopSize) {
    return static_cast<int>((audioSamples + hopSize - 1) / hopSize);
}

// ═══════════════════════════════════════════════════════════════════
// 2. 无声帧插值 — 填充 F0=0 的 unvoiced 帧
// ═══════════════════════════════════════════════════════════════════

/// @brief 在 log2(Hz) 域内对无声帧（值 = 0）做线性插值。
///
/// 等价于 MakeDiffSinger get_pitch.py 的 interp_f0()。
/// 首尾无声段用最近的有声值填充（constant extrapolation）。
/// 全部无声则不修改。
///
/// @param[in,out] values F0 值序列（mHz）。0 = unvoiced。原地修改。
/// @param[out]    uv     可选。输出无声标记：true = 原始无声。
///                       nullptr 则不输出。
void interpUnvoiced(std::vector<int32_t> &values,
                    std::vector<bool> *uv = nullptr);

/// @brief 同上，double 版本（Hz 域）。
void interpUnvoicedF(std::vector<double> &values,
                     std::vector<bool> *uv = nullptr);

// ═══════════════════════════════════════════════════════════════════
// 3. 平滑滤波 — 移动平均、中值滤波
// ═══════════════════════════════════════════════════════════════════

/// @brief 移动平均滤波（跳过零值 / unvoiced 帧）。
///
/// 从 PitchProcessor::movingAverage 提取并泛化。
///
/// @param values   输入曲线
/// @param window   窗口大小（采样点数，奇数）
/// @param skipZero 是否跳过零值帧。默认 true（F0 曲线的 unvoiced 帧）。
/// @return 平滑后的曲线，长度不变。
std::vector<double> movingAverage(const std::vector<double> &values,
                                  int window,
                                  bool skipZero = true);

/// @brief 中值滤波（跳过零值）。
/// @param values   输入曲线
/// @param window   窗口大小（奇数）
/// @return 滤波后的曲线。
std::vector<double> medianFilter(const std::vector<double> &values,
                                 int window);

// ═══════════════════════════════════════════════════════════════════
// 4. 频域转换 — Hz / MIDI / mHz / log2 之间的批量转换
// ═══════════════════════════════════════════════════════════════════

/// @brief Hz 序列 → mHz 序列（批量，0 保持为 0）。
std::vector<int32_t> hzToMhzBatch(const std::vector<double> &hz);

/// @brief mHz 序列 → Hz 序列（批量）。
std::vector<double> mhzToHzBatch(const std::vector<int32_t> &mhz);

/// @brief Hz 序列 → MIDI 序列（批量，0 保持为 0）。
std::vector<double> hzToMidiBatch(const std::vector<double> &hz);

/// @brief MIDI 序列 → Hz 序列（批量）。
std::vector<double> midiToHzBatch(const std::vector<double> &midi);

/// @brief mHz 序列 → MIDI 序列（批量，0 保持为 0）。
std::vector<double> mhzToMidiBatch(const std::vector<int32_t> &mhz);

// ═══════════════════════════════════════════════════════════════════
// 5. 曲线对齐 — 将曲线截断/填充到指定长度
// ═══════════════════════════════════════════════════════════════════

/// @brief 截断或填充曲线到指定长度。
/// 过长则截断。过短则用 fillValue 填充末尾。
template <typename T>
std::vector<T> alignToLength(const std::vector<T> &values,
                              int targetLength,
                              T fillValue = T{}) {
    std::vector<T> result(targetLength, fillValue);
    int copyLen = std::min(static_cast<int>(values.size()), targetLength);
    std::copy_n(values.begin(), copyLen, result.begin());
    if (targetLength > static_cast<int>(values.size()) && !values.empty()) {
        // 用末尾值填充（而非 fillValue），匹配 MDS resample_align_curve 行为
        std::fill(result.begin() + copyLen, result.end(), values.back());
    }
    return result;
}

// ═══════════════════════════════════════════════════════════════════
// 6. 高级：Smoothstep crossfade（曲线拼接过渡）
// ═══════════════════════════════════════════════════════════════════

/// @brief 在曲线首尾应用 smoothstep 交叉淡入淡出。
///
/// 从 PitchProcessor::applyModulationDriftPreview 提取。
/// 用于曲线拼接时避免硬切割伪影。
///
/// @param[in,out] values   目标曲线（原地修改）
/// @param         original 原始曲线（用于 crossfade 的另一端）
/// @param         crossfadeLen 首尾各淡入/淡出的采样点数
void smoothstepCrossfade(std::vector<double> &values,
                          const std::vector<double> &original,
                          int crossfadeLen);

} // namespace dstools
```

### L.0b.2 CurveTools 实现

**文件**：`src/domain/src/CurveTools.cpp`（新建）

**关键算法**：

#### resampleCurve（线性插值重采样）

```cpp
std::vector<int32_t> resampleCurve(const std::vector<int32_t> &values,
                                    TimePos srcTimestep,
                                    TimePos dstTimestep,
                                    int alignLength) {
    if (values.empty() || srcTimestep <= 0 || dstTimestep <= 0)
        return {};

    // 源曲线总时长
    double tMax = static_cast<double>(values.size() - 1) * srcTimestep;

    // 目标长度
    int outLen = alignLength > 0
        ? alignLength
        : static_cast<int>(std::ceil(tMax / dstTimestep)) + 1;

    std::vector<int32_t> result(outLen);

    for (int i = 0; i < outLen; ++i) {
        double t = static_cast<double>(i) * dstTimestep;
        double srcIdx = t / static_cast<double>(srcTimestep);

        int lo = static_cast<int>(srcIdx);
        if (lo < 0) lo = 0;
        if (lo >= static_cast<int>(values.size()) - 1) {
            result[i] = values.back();
            continue;
        }

        double frac = srcIdx - lo;
        // 线性插值
        result[i] = static_cast<int32_t>(std::lround(
            values[lo] * (1.0 - frac) + values[lo + 1] * frac));
    }

    return result;
}
```

#### interpUnvoiced（log2 域无声帧插值）

```cpp
void interpUnvoiced(std::vector<int32_t> &values, std::vector<bool> *uvOut) {
    const int n = static_cast<int>(values.size());
    if (n == 0) return;

    // 标记 unvoiced
    std::vector<bool> uv(n);
    for (int i = 0; i < n; ++i)
        uv[i] = (values[i] == 0);

    if (uvOut) *uvOut = uv;

    // 全部 unvoiced → 不修改
    bool anyVoiced = false;
    for (int i = 0; i < n; ++i)
        if (!uv[i]) { anyVoiced = true; break; }
    if (!anyVoiced) return;

    // 找首尾有声帧
    int first = 0, last = n - 1;
    while (first < n && uv[first]) ++first;
    while (last >= 0 && uv[last]) --last;

    // 首尾用最近有声值填充
    for (int i = 0; i < first; ++i)
        values[i] = values[first];
    for (int i = last + 1; i < n; ++i)
        values[i] = values[last];

    // 中间段：log2 域线性插值
    for (int i = first; i <= last; ++i) {
        if (!uv[i]) continue;
        int prev = i - 1; // prev 一定有声
        int next = i + 1;
        while (next <= last && uv[next]) ++next;
        if (next > last) break;

        double logPrev = std::log2(mhzToHz(values[prev]));
        double logNext = std::log2(mhzToHz(values[next]));

        for (int j = i; j < next; ++j) {
            if (!uv[j]) continue;
            double t = static_cast<double>(j - prev) / (next - prev);
            double logInterp = logPrev + t * (logNext - logPrev);
            values[j] = hzToMhz(std::pow(2.0, logInterp));
        }
        i = next - 1; // 跳到下一个有声帧
    }
}
```

### L.0b.3 CurveTools 测试

**文件**：`src/tests/domain/TestCurveTools.cpp`（新建）

| 用例 | 输入 | 验证点 |
|------|------|--------|
| `resample_identity` | src=dst timestep | 输出 = 输入 |
| `resample_upsample` | 10ms → 5ms | 长度翻倍，中间值线性插值 |
| `resample_downsample` | 10ms → 20ms | 长度减半 |
| `resample_align` | 任意 → alignLength=100 | 输出恰好 100 点 |
| `resample_tail_fill` | 短序列 → 长 alignLength | 尾部用末值填充 |
| `resample_rmvpe_to_ds` | 10ms → 11.61ms (44100/512) | 等价于 MDS resample_align_curve |
| `interp_no_uv` | 全部有声 | 不修改 |
| `interp_all_uv` | 全部无声 | 不修改 |
| `interp_edges` | 首尾无声 | 用最近有声值填充 |
| `interp_middle` | 中间无声段 | log2 域线性插值，验证精度 |
| `interp_mds_compat` | MDS get_pitch.py 相同输入 | 输出与 Python 一致（容差 ±1 mHz） |
| `moving_avg_skip_zero` | 含零值序列 | 零值跳过，非零值正确平均 |
| `median_filter` | 含尖刺序列 | 尖刺被中值消除 |
| `hz_midi_batch` | 已知频率列表 | A4=440Hz=69.0 MIDI |
| `align_short` | 3 点 → 10 | 末尾填充末值 |
| `align_long` | 10 点 → 3 | 截断 |
| `smoothstep_crossfade` | 两段曲线拼接 | 拼接处平滑过渡，非硬切割 |
| `hopsize_to_timestep` | hopSizeToTimestep(512, 44100) | == 11610 μs |
| `expected_frames` | 44100 samples, hop=512 | == 87 帧 |

---

## L.1 — Boundary / Layer 领域模型 + .dstext I/O

### L.1.1 DsTextTypes.h

**文件**：`src/domain/include/dstools/DsTextTypes.h`（新建）

```cpp
#pragma once

#include <dstools/TimePos.h>
#include <QString>
#include <nlohmann/json.hpp>
#include <vector>

namespace dstools {

/// A single boundary marker in a layer.
struct Boundary {
    int id = 0;          ///< File-unique ID. Monotonically increasing.
    TimePos pos = 0;     ///< Position in microseconds.
    QString text;        ///< Content of the interval to the right of this boundary.
};

/// An interval-based annotation layer (text, note types).
struct IntervalLayer {
    QString name;
    std::vector<Boundary> boundaries;  ///< May be stored unordered; sorted on load.

    /// Sort boundaries by pos ascending, stable (preserve insertion order for equal pos).
    void sortBoundaries();

    /// Get the next available boundary ID (max existing + 1).
    int nextId() const;
};

/// A curve-based annotation layer (F0, energy, etc.)
struct CurveLayer {
    QString name;
    TimePos timestep = 0;              ///< Microseconds between samples.
    std::vector<int32_t> values;       ///< Sample values (e.g. mHz for F0).
};

/// Audio reference stored in .dstext.
struct DsTextAudio {
    QString path;        ///< Relative path to audio file.
    TimePos in = 0;      ///< Slice start in the source audio (μs).
    TimePos out = 0;     ///< Slice end in the source audio (μs).
};

/// A binding group: boundary IDs that move together.
using BindingGroup = std::vector<int>;

/// In-memory representation of a .dstext file.
struct DsTextDocument {
    QString version = "2.0.0";
    DsTextAudio audio;
    std::vector<IntervalLayer> layers;
    std::vector<CurveLayer> curves;
    std::vector<BindingGroup> groups;

    // ── File I/O ──
    static Result<DsTextDocument> load(const QString &path);
    Result<void> save(const QString &path) const;

    // ── Group helpers ──

    /// Find the binding group containing the given boundary ID, or nullptr.
    const BindingGroup *findGroup(int boundaryId) const;

    /// Get all boundary IDs bound to the given ID (including itself).
    std::vector<int> boundIdsOf(int boundaryId) const;

    /// Rebuild groups from schema relationships + layer alignment.
    /// Called after a processor produces new layer data.
    void rebuildGroupsFromSchema(const nlohmann::json &schema);
};

} // namespace dstools
```

### L.1.2 / L.1.3 — DsTextDocument 实现

**文件**：`src/domain/src/DsTextDocument.cpp`（新建）

**要点**：

- **v2 读取**：`pos` 字段为 `integer`（μs）
- **v1 检测与迁移**：若 version 为 `"1.0.0"` 或 `pos` 不存在但 `position` 存在，执行：
  - `pos = round(position × 1e6)`
  - 重命名字段
  - 更新 version 为 `"2.0.0"`
- **写出**：始终写 v2 格式
- **排序**：加载后对每层 boundaries 按 pos 升序排序
- **边界自动补全**：确保最左 boundary pos=0，最右 boundary pos=(out-in)
- **round-trip**：保留未知 JSON 字段（用 `nlohmann::json m_extraFields`）

### L.1.4 — CurveLayer（已合并到 L.1.1）

### L.1.5 — 测试

**文件**：`src/tests/domain/TestDsTextDocument.cpp`（新建）

| 用例 | 验证点 |
|------|--------|
| `load_v2_roundtrip` | v2 JSON 读写往返，所有字段完整保留 |
| `load_v1_migration` | v1 格式（float `position`）自动迁移为 v2（int64 `pos`） |
| `boundary_auto_complete` | 缺少左/右边界时自动补全 |
| `sort_boundaries` | 乱序 boundaries 加载后排序正确 |
| `groups_lookup` | findGroup / boundIdsOf 正确查找绑定关系 |
| `curve_layer` | curve 层读写往返 |
| `unknown_fields_preserved` | 额外 JSON 字段不丢失 |

---

## L.2 — PipelineContext + IAudioPreprocessor + IFormatAdapter

### L.2.1 PipelineContext

**文件**：`src/framework/core/include/dsfw/PipelineContext.h`（新建）

```cpp
#pragma once

#include <dsfw/TaskTypes.h>
#include <dstools/TimePos.h>
#include <dstools/Result.h>
#include <nlohmann/json.hpp>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <map>
#include <vector>

namespace dstools {

/// Processing record for a single pipeline step.
struct StepRecord {
    QString stepName;
    QString processorId;
    QDateTime startTime;
    QDateTime endTime;
    bool success = false;
    QString errorMessage;
    ProcessorConfig usedConfig;
};

/// Per-item pipeline state accumulating across all steps.
struct PipelineContext {
    // ── Identity ──
    QString audioPath;
    QString itemId;

    // ── Layer data ──
    std::map<QString, nlohmann::json> layers;   ///< category → layer data.
    ProcessorConfig globalConfig;

    // ── Status ──
    enum class Status { Active, Discarded, Error };
    Status status = Status::Active;
    QString discardReason;
    QString discardedAtStep;

    // ── Progress tracking ──
    QStringList completedSteps;
    std::vector<StepRecord> stepHistory;

    // ── Core methods ──

    /// Build a TaskInput from this context's layers, per spec's input slots.
    /// Returns error if a required input layer is missing.
    Result<TaskInput> buildTaskInput(const TaskSpec &spec) const;

    /// Apply a TaskOutput's layers back into this context, per spec's output slots.
    void applyTaskOutput(const TaskSpec &spec, const TaskOutput &output);

    // ── Serialization ──
    nlohmann::json toJson() const;
    static Result<PipelineContext> fromJson(const nlohmann::json &j);

    /// Save to file (dstemp/contexts/{itemId}.json).
    Result<void> saveToFile(const QString &dir) const;

    /// Load from file.
    static Result<PipelineContext> loadFromFile(const QString &path);
};

} // namespace dstools
```

**buildTaskInput 实现要点**：

```cpp
Result<TaskInput> PipelineContext::buildTaskInput(const TaskSpec &spec) const {
    TaskInput input;
    input.audioPath = audioPath;
    input.config = globalConfig;
    for (const auto &slot : spec.inputs) {
        auto it = layers.find(slot.category);
        if (it == layers.end())
            return Err<TaskInput>("Missing required layer: " + slot.category.toStdString());
        input.layers[slot.name] = it->second;
    }
    return Ok(std::move(input));
}
```

**applyTaskOutput 实现要点**：

```cpp
void PipelineContext::applyTaskOutput(const TaskSpec &spec, const TaskOutput &output) {
    for (const auto &slot : spec.outputs) {
        auto it = output.layers.find(slot.name);
        if (it != output.layers.end())
            layers[slot.category] = it->second;  // key by category, not slot name
    }
}
```

### L.2.2 IAudioPreprocessor

**文件**：`src/framework/core/include/dsfw/IAudioPreprocessor.h`（新建）

```cpp
#pragma once

#include <dsfw/TaskTypes.h>
#include <dstools/Result.h>
#include <QString>

namespace dstools {

class IAudioPreprocessor {
public:
    virtual ~IAudioPreprocessor() = default;
    virtual QString preprocessorId() const = 0;
    virtual QString displayName() const = 0;
    virtual Result<void> process(const QString &inputPath,
                                 const QString &outputPath,
                                 const ProcessorConfig &config) = 0;
};

} // namespace dstools
```

### L.2.3 IFormatAdapter

**文件**：`src/framework/core/include/dsfw/IFormatAdapter.h`（新建）

```cpp
#pragma once

#include <dsfw/TaskTypes.h>
#include <dstools/Result.h>
#include <QString>
#include <map>
#include <nlohmann/json.hpp>

namespace dstools {

class IFormatAdapter {
public:
    virtual ~IFormatAdapter() = default;
    virtual QString formatId() const = 0;
    virtual QString displayName() const = 0;
    virtual bool canImport() const { return false; }
    virtual bool canExport() const { return false; }

    virtual Result<void> importToLayers(
        const QString &filePath,
        std::map<QString, nlohmann::json> &layers,
        const ProcessorConfig &config) = 0;

    virtual Result<void> exportFromLayers(
        const std::map<QString, nlohmann::json> &layers,
        const QString &outputPath,
        const ProcessorConfig &config) = 0;
};

} // namespace dstools
```

### L.2.4 FormatAdapterRegistry

**文件**：`src/framework/core/include/dsfw/FormatAdapterRegistry.h`（新建）

```cpp
#pragma once

#include <dsfw/IFormatAdapter.h>
#include <map>
#include <memory>
#include <mutex>

namespace dstools {

class FormatAdapterRegistry {
public:
    static FormatAdapterRegistry &instance();

    void registerAdapter(std::unique_ptr<IFormatAdapter> adapter);
    IFormatAdapter *adapter(const QString &formatId) const;
    QStringList availableFormats() const;

private:
    FormatAdapterRegistry() = default;
    mutable std::mutex m_mutex;
    std::map<QString, std::unique_ptr<IFormatAdapter>> m_adapters;
};

} // namespace dstools
```

### L.2.5 — PipelineContext 测试

| 用例 | 验证点 |
|------|--------|
| `buildTaskInput_ok` | 所有必需层存在 → 正确构建 TaskInput |
| `buildTaskInput_missing` | 缺少必需层 → 返回 Error |
| `applyTaskOutput` | 输出层按 category 写入 context |
| `json_roundtrip` | toJson → fromJson 往返，所有字段保留 |
| `status_propagation` | Discarded 状态 + reason + step 正确序列化 |
| `file_roundtrip` | saveToFile → loadFromFile 往返 |

---

## L.3 — 4 个格式适配器

### L.3.1 LabAdapter

**文件**：`src/domain/src/adapters/LabAdapter.h/.cpp`

```cpp
class LabAdapter : public IFormatAdapter {
public:
    QString formatId() const override { return "lab"; }
    bool canImport() const override { return true; }
    bool canExport() const override { return true; }

    // import: read single-line text → grapheme layer
    //   "gan shou ting zai" → [{text:"gan", pos:0}, {text:"shou", pos:0}, ...]
    //   NOTE: .lab has no timing info → all pos = 0 (timing added by alignment step)
    Result<void> importToLayers(...) override;

    // export: grapheme layer → single-line text
    //   [{text:"gan",...}, {text:"shou",...}] → "gan shou ..."
    Result<void> exportFromLayers(...) override;
};
```

### L.3.2 TextGridAdapter

```cpp
class TextGridAdapter : public IFormatAdapter {
    // TextGrid has 2 layouts:
    //   2-tier: [words, phones]              — per-slice
    //   3-tier: [sentences, words, phones]   — whole-song (MDS combine_tg output)
    //
    // import (2-tier → layers):
    //   "phones" tier → phoneme layer
    //     Each interval → {phone: text, start: secToUs(minTime), end: secToUs(maxTime)}
    //     Empty mark → "SP"
    //   "words" tier → grapheme layer
    //     Each interval → {text: mark, pos: secToUs(minTime)}
    //
    // import (3-tier → slices + layers):
    //   "sentences" tier → slice boundaries
    //     Each interval with non-empty mark → one slice
    //     Empty mark → discarded slice (user deleted the sentence in Praat/vLabeler)
    //   For each non-empty sentence interval:
    //     Clip words/phones within [sentenceStart, sentenceEnd]
    //     Time-shift to zero: pos -= sentenceStart
    //     → produces per-slice grapheme + phoneme layers
    //   Returns: config["slices"] = JSON array of slice boundaries
    //
    // export (layers → 2-tier):
    //   phoneme → "phones" IntervalTier (usToSec for times)
    //   grapheme → "words" IntervalTier
    //
    // export (layers → 3-tier, for refinement workflow):
    //   slice list → "sentences" tier (mark = itemId, discarded → empty mark)
    //   All slices' grapheme → concatenated "words" tier
    //   All slices' phoneme → concatenated "phones" tier
    //
    // Tier name matching is case-insensitive.
    // Internally uses textgrid.hpp for parsing/writing.
};
```

### L.3.3 CsvAdapter

```cpp
class CsvAdapter : public IFormatAdapter {
    // import: transcriptions.csv → multiple layers
    //   Internally: read CSV → TranscriptionRow[] → convert each field to layer:
    //     name → stored in config (not a layer)
    //     ph_seq + ph_dur → phoneme layer [{phone, start, end}]
    //         cumulative sum of ph_dur gives start times
    //     ph_num → ph_num layer [int, ...]
    //     note_seq + note_dur + note_slur + note_glide → midi layer [{note, duration, slur, glide}]
    //
    // export: layers → transcriptions.csv
    //   Reverse conversion. Only writes columns that have data.
    //   Uses TranscriptionCsv::write() internally.
    //   Time values: usToSec() → snprintf("%.6f")
};
```

### L.3.4 DsFileAdapter

```cpp
class DsFileAdapter : public IFormatAdapter {
    // import: .ds JSON → layers
    //   Uses DsDocument::loadFile()
    //   ph_seq + ph_dur → phoneme layer (same as CSV)
    //   ph_num → ph_num layer
    //   note_seq + note_dur + note_slur + note_glide → midi layer
    //   f0_seq + f0_timestep → pitch layer {f0: [mHz], timestep: μs}
    //   offset → stored in config
    //
    // export: layers → .ds JSON
    //   Reverse. f0: mhzToHz each value, format "%.1f"
    //   durations: usToSec, format "%.6f" (space-separated)
    //   Compatible with MDS convert_ds.py output
};
```

---

## L.4 — PipelineRunner

### L.4.1–L.4.2

**文件**：`src/framework/core/include/dsfw/PipelineRunner.h`（新建）

```cpp
#pragma once

#include <dsfw/PipelineContext.h>
#include <dsfw/ITaskProcessor.h>
#include <dsfw/IAudioPreprocessor.h>
#include <dsfw/IFormatAdapter.h>
#include <dsfw/TaskProcessorRegistry.h>

namespace dstools {

using ValidationCallback = std::function<
    PipelineContext::Status(const PipelineContext &, const TaskSpec &, QString &reason)>;

struct StepConfig {
    QString taskName;
    QString processorId;
    ProcessorConfig config;
    bool optional = false;
    bool manual = false;
    QString importFormat;
    QString importPath;
    QString exportFormat;
    QString exportPath;
    ValidationCallback validator;
};

struct PipelineOptions {
    std::vector<std::shared_ptr<IAudioPreprocessor>> preprocessors;
    std::vector<StepConfig> steps;
    QStringList audioFiles;
    ProcessorConfig globalConfig;
    QString workingDir;         ///< dstemp root
};

class PipelineRunner : public QObject {
    Q_OBJECT
public:
    explicit PipelineRunner(QObject *parent = nullptr);

    Result<void> run(const PipelineOptions &opts);

signals:
    void progress(int step, int item, int totalItems, const QString &desc);
    void manualStepRequired(const StepConfig &step,
                            std::vector<PipelineContext> &contexts);
    void stepCompleted(int step, const QString &stepName);
    void itemDiscarded(const QString &itemId, const QString &reason);

private:
    void saveSnapshot(const PipelineContext &ctx, const QString &stepName,
                      const QString &snapshotDir);
    void saveContext(const PipelineContext &ctx, const QString &contextDir);
};

} // namespace dstools
```

**实现要点**（`PipelineRunner.cpp`）：

1. **预处理阶段**：按顺序对每个 audioFile 执行所有 preprocessors
2. **切片阶段**：找到 `audio_slice` 步骤，创建 SlicerProcessor，生成切片列表，为每个切片创建 PipelineContext
3. **整首歌步骤**（`granularity == whole_audio`）：如 LyricFA，运行一次，结果分发到各 context
4. **逐切片步骤循环**：
   - 跳过 `status == Discarded` 的 context
   - 人工步骤：emit `manualStepRequired`，等待外部完成
   - 自动步骤：保存快照 → 可选导入 → buildTaskInput → process → applyTaskOutput → 验证 → 可选导出 → 记录历史 → 持久化 context
5. **错误处理**：process 失败 → 设 status=Error，记录 errorMessage，继续下一项

---

## L.6 — processBatch 移除（精确修改清单）

| 文件 | 行 | 操作 |
|------|-----|------|
| `ITaskProcessor.h` | 62-64 | 删除 `virtual Result<BatchOutput> processBatch(...)` 声明 |
| `TaskTypes.h` | 46-59 | 删除 `BatchInput` 和 `BatchOutput` 结构体 |
| `TaskProcessorRegistry.cpp` | 1-2 (`#include <dsfw/BatchCheckpoint.h>`) | 删除 include |
| `TaskProcessorRegistry.cpp` | 52-116 | 删除整个 `ITaskProcessor::processBatch` 默认实现 |
| `GameMidiProcessor.h` | 35 | 删除 `processBatch` 声明 |
| `GameMidiProcessor.cpp` | 139-169 | 删除 `processBatch` 实现 |
| `RmvpePitchProcessor.h` | 31 | 删除 `processBatch` 声明 |
| `RmvpePitchProcessor.cpp` | 84-135 | 删除 `processBatch` 实现 + 静态注册保持不变 |
| `MainWidget.h` (GameInfer) | — | 无变化（已用 process()） |
| `MainWidget.cpp` (GameInfer) | 找到 `processBatch` 调用 | 改为循环调 `process()` |
| `HubertFAPage.h` | 66 | 删除 `m_checkpoint` 成员 |
| `HubertFAPage.cpp` | 找到 `processBatch`/`BatchCheckpoint` | 改为循环调 `process()` |
| `cli/main.cpp` | 找到 `BatchInput` 构造 | 改为循环调 `process()` |
| `examples/minimal-task-processor/` | 找到 `processBatch` 示例 | 删除 |
| `TestTaskProcessorRegistry.cpp` | 找到 `processBatch` 测试 | 删除该测试 |

---

## L.7 — F0Curve / DSFile 迁移（精确修改清单）

### L.7.1 F0Curve

**当前签名 → 新签名**：

| 当前 | 新 |
|------|-----|
| `double timestep` | `TimePos timestep` |
| `vector<double> values` | `vector<int32_t> values` (mHz) |
| `double totalDuration() const` | `TimePos totalDuration() const` |
| `double getValueAtTime(double timeSec) const` | `int32_t getValueAtTime(TimePos time) const` |
| `vector<double> getRange(double start, double end) const` | `vector<int32_t> getRange(TimePos start, TimePos end) const` |
| `void setRange(double start, const vector<double> &)` | `void setRange(TimePos start, const vector<int32_t> &)` |

**实现变更**（`F0Curve.cpp`）：

```cpp
// Before:
double index = time / timestep;

// After:
// timestep and time are both in μs (int64), so:
double index = static_cast<double>(time) / static_cast<double>(timestep);
// rest of interpolation logic unchanged (uses double internally for sub-sample precision)
```

### L.7.2 DSFile

| 字段 | 当前 | 新 |
|------|------|-----|
| `Phone::start` | `double` | `TimePos` |
| `Phone::duration` | `double` | `TimePos` |
| `Phone::end()` | `return start + duration` | 不变（int64 加法） |
| `Note::duration` | `double` | `TimePos` |
| `Note::start` | `double` | `TimePos` |
| `DSFile::offset` | `double` | `TimePos` |

**JSON 读取迁移**（`DSFile.cpp loadFromJson`）：

```cpp
// Before:
phone.duration = (item["ph_dur"] 解析为 double);

// After:
double durSec = ... // parse from string "0.080000"
phone.duration = secToUs(durSec);
```

**JSON 写回迁移**（`DSFile.cpp writeBackToJson`）：

```cpp
// Before:
ph_dur_str += QString::number(phone.duration, 'f', 6);

// After:
ph_dur_str += timePosToSecString(phone.duration);
```

### L.7.3 PitchLabeler Commands

| Command | 需改字段 | 说明 |
|---------|---------|------|
| `DeleteNotesCommand` | `NoteSnapshot::duration` → `TimePos` | 快照中的 duration |
| `MergeNoteLeftCommand` | `NoteSnapshot::duration` → `TimePos` | 同上 |
| `ModulationDriftCommand` | `vector<double> m_oldF0/m_newF0` → `vector<int32_t>` | F0 值改 mHz |
| 其余 5 个 | 无 double 时间字段 | SetNoteGlide/ToggleSlur/ToggleRest/SnapToPitch/PitchMove 不涉及时间 |

**实际需改 Command：3 个**（不是 8 个，审计修正）。

### L.7.4 PitchProcessor → CurveTools 替换

`PitchProcessor::movingAverage` 和 `applyModulationDriftPreview` 中的 smoothstep crossfade 提取到 CurveTools 后，PitchProcessor 改为调用 CurveTools API：

```cpp
// Before (PitchProcessor.cpp):
auto centerline = movingAverage(segHz, window);
// ... smoothstep crossfade inline code ...

// After:
auto centerline = CurveTools::movingAverage(segHz, window);
// ... use CurveTools::smoothstepCrossfade(newVals, segHz, crossfade);
```

`PitchProcessor::movingAverage` 静态方法可保留为 `CurveTools::movingAverage` 的转发，或直接删除。

### L.7.5 RmvpePitchProcessor 增加重采样

**当前问题**：RMVPE 固定输出 10ms timestep (16kHz, hop=160)，但目标 hopSize/sampleRate 可能不同。MDS 的 `get_pitch.py` 用 `resample_align_curve` 解决此问题，C++ 缺失此步骤。

```cpp
// RmvpePitchProcessor::process() 新增：
Result<TaskOutput> RmvpePitchProcessor::process(const TaskInput &input) {
    // ... existing RMVPE inference → raw f0 (10ms timestep, Hz) ...

    // 从 config 读取目标参数
    int hopSize = input.config.value("hopSize", 512);
    int sampleRate = input.config.value("sampleRate", 44100);
    TimePos targetTimestep = hopSizeToTimestep(hopSize, sampleRate);
    TimePos srcTimestep = secToUs(0.01);  // RMVPE 固定 10ms

    // 计算目标帧数
    int64_t audioSamples = /* from audio file length */;
    int targetLen = expectedFrameCount(audioSamples, hopSize);

    // Hz → mHz
    auto mhzValues = hzToMhzBatch(f0All);

    // 无声帧插值
    interpUnvoiced(mhzValues);

    // 重采样到目标 timestep
    auto resampled = resampleCurve(mhzValues, srcTimestep, targetTimestep, targetLen);

    // 输出
    TaskOutput output;
    nlohmann::json pitchLayer;
    pitchLayer["f0"] = resampled;
    pitchLayer["timestep"] = targetTimestep;
    output.layers["pitch"] = std::move(pitchLayer);
    return Ok(std::move(output));
}
```

**同样适用于未来的 FCPE、CREPE 等后端**——它们只要输出原始 f0 + 声明自己的 srcTimestep，CurveTools 统一重采样。

---

## L.8 — PhonemeLabeler 时间迁移（精确修改清单）

### L.8.1 TextGridDocument

**关键策略**：TextGridDocument 内部仍用 `textgrid::TextGrid`（textgrid.hpp，double 秒），但公共 API 全部暴露 `TimePos`。在 getter/setter **边界处**做 `secToUs/usToSec` 转换。

```cpp
// Before:
void moveBoundary(int tierIndex, int boundaryIndex, double newTime);
double boundaryTime(int tierIndex, int boundaryIndex) const;

// After:
void moveBoundary(int tierIndex, int boundaryIndex, TimePos newTime);
TimePos boundaryTime(int tierIndex, int boundaryIndex) const;

// Implementation:
void TextGridDocument::moveBoundary(int tierIndex, int boundaryIndex, TimePos newTime) {
    // Convert to double for textgrid.hpp internal use
    double sec = usToSec(newTime);
    // ... existing textgrid manipulation with sec ...
    emit boundaryMoved(tierIndex, boundaryIndex, newTime);
}

TimePos TextGridDocument::boundaryTime(int tierIndex, int boundaryIndex) const {
    double sec = /* get from textgrid.hpp */;
    return secToUs(sec);
}
```

**信号变更**：
```cpp
// Before:
void boundaryMoved(int tierIndex, int boundaryIndex, double newTime);

// After:
void boundaryMoved(int tierIndex, int boundaryIndex, TimePos newTime);
```

### L.8.2 BoundaryBindingManager

```cpp
// Before:
struct AlignedBoundary {
    int tierIndex;
    int boundaryIndex;
    double time;
};

// After:
struct AlignedBoundary {
    int tierIndex;
    int boundaryIndex;
    TimePos time;
};

// Tolerance: Before used double ms comparison, now use int64 μs:
// Before: abs(a - b) < toleranceMs / 1000.0
// After:  abs(a - b) <= m_toleranceUs   (where m_toleranceUs = msToUs(toleranceMs))
```

### L.8.3 Commands

| Command | 字段变更 |
|---------|---------|
| `MoveBoundaryCommand` | `double m_oldTime, m_newTime` → `TimePos` |
| `LinkedMoveBoundaryCommand` | 构造参数 `double oldTime, newTime` → `TimePos`；内部 `AlignedBoundary` 已随 L.8.2 改 |
| `InsertBoundaryCommand` | `double m_time` → `TimePos` |
| `RemoveBoundaryCommand` | `double m_savedTime` → `TimePos` |
| `SetIntervalTextCommand` | 无时间字段，**不需改** |

**实际需改 Command：4 个**（不是 5 个）。

### L.8.4 Widgets

所有 5 个 widget 的 `xToTime`/`timeToX` 需要改签名，但**内部像素计算仍用 double**：

```cpp
// Before:
double xToTime(int x) const {
    return m_viewStart + (x / width()) * (m_viewEnd - m_viewStart);
}

// After:
TimePos xToTime(int x) const {
    double sec = usToSec(m_viewStart) + (static_cast<double>(x) / width())
                 * usToSec(m_viewEnd - m_viewStart);
    return secToUs(sec);
}
```

`m_viewStart` / `m_viewEnd` 类型从 `double`（秒）→ `TimePos`（μs）。

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

## 验收标准总表

| 阶段 | 编译通过 | 测试通过 | 现有功能不退化 |
|------|---------|---------|--------------|
| L.0 | ✓ 纯新增 | TestTimePos 全绿 | 无影响 |
| L.0b | ✓ 纯新增 | TestCurveTools 全绿 (19 用例) | 无影响 |
| L.1 | ✓ 纯新增 | TestDsTextDocument 全绿 | 无影响 |
| L.2 | ✓ 纯新增 | TestPipelineContext 全绿 | 无影响 |
| L.3 | ✓ 纯新增 | TestFormatAdapters 全绿 | 无影响 |
| L.4 | ✓ 纯新增 | TestPipelineRunner 全绿 | 无影响 |
| L.5 | ✓ 纯新增 | TestSlicerProcessor + TestAddPhNumProcessor 全绿 | 无影响 |
| L.6 | ✓ 编译修复 | 所有现有 24 测试 + 更新后测试全绿 | GameInfer/HubertFA/CLI 功能等价 |
| L.7 | ✓ 编译修复 | TestF0Curve 适配后全绿 | PitchLabeler 功能等价 |
| L.8 | ✓ 编译修复 | 手动验证 PhonemeLabeler 边界拖动/绑定 | 拖动/绑定行为不变 |
| L.9 | ✓ 编译修复 | 手动验证 DiffSingerLabeler 全流程 | 9 步 wizard 功能等价 |
| L.10 | ✓ deprecated 警告 | 所有测试全绿 | 无功能退化 |
| L.11 | ✓ 编译修复 | TestDsProject 全绿 | .dsproj 读写兼容 |
| L.12 | ✓ 编译修复 | 所有现有测试全绿 | 编译产物二进制一致（除调试信息格式） |

---

## L.12 — 编译速度优化（精确修改清单）

### L.12.1 PCH 注入

**修改文件**：`cmake/DstoolsHelpers.cmake`

在 `dstools_add_library` 函数尾部（`endfunction()` 前）新增：

```cmake
# --- Precompiled headers (PCH) -----------------------------------------------
if(NOT _type STREQUAL "INTERFACE" AND NOT ARG_NO_PCH)
    target_precompile_headers(${target_name} PRIVATE
        <QString>
        <QStringList>
        <QObject>
        <QDebug>
        <vector>
        <map>
        <memory>
        <string>
        <functional>
        <cstdint>
    )
endif()
```

在 `dstools_add_executable` 函数尾部同理新增（相同 PCH 列表）。

函数签名新增 `NO_PCH` 选项：
```cmake
set(_options STATIC SHARED INTERFACE AUTOMOC AUTORCC AUTOUIC NO_PCH)
```

**推理层豁免**：`src/infer/` 下的 4 个库（audio-util, game-infer, hubert-infer, rmvpe-infer）使用各自独立的 CMakeLists.txt + `infer-target.cmake` 宏定义目标，**不经过** `dstools_add_library`，因此不受影响。如需为推理层添加 PCH，需单独在各自 CMakeLists.txt 中调用 `target_precompile_headers`。

### L.12.2 ccache/sccache 自动检测

**修改文件**：`CMakeLists.txt`（根目录），在 `project()` 之后、`add_subdirectory(src)` 之前新增：

```cmake
# Compiler cache (ccache / sccache)
find_program(SCCACHE_PROGRAM sccache)
find_program(CCACHE_PROGRAM ccache)
if(SCCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
    message(STATUS "Compiler cache: sccache (${SCCACHE_PROGRAM})")
elseif(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message(STATUS "Compiler cache: ccache (${CCACHE_PROGRAM})")
    if(MSVC)
        # ccache 不兼容 MSVC /Zi (pdb 文件锁)，改用 /Z7 (内嵌调试信息)
        foreach(_config DEBUG RELWITHDEBINFO)
            string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_${_config} "${CMAKE_C_FLAGS_${_config}}")
            string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_${_config} "${CMAKE_CXX_FLAGS_${_config}}")
        endforeach()
    endif()
endif()
```

### L.12.4 MSVC /MP 并行编译

**修改文件**：`cmake/DstoolsHelpers.cmake`

在 `dstools_add_library` 和 `dstools_add_executable` 的现有 MSVC 编译选项处追加 `/MP`：

```cmake
# Before:
if(MSVC)
    target_compile_options(${target_name} PRIVATE /utf-8 /W4 /wd4251)

# After:
if(MSVC)
    target_compile_options(${target_name} PRIVATE /utf-8 /W4 /wd4251 /MP)
```

**注意**：`/MP` 与 PCH 兼容。MSVC 文档确认 `/MP` 和 `/Yu`（使用 PCH）可共存。

### 验收

| 检查项 | 方法 |
|--------|------|
| PCH 生效 | 构建日志出现 `cmake_pch.hxx` 相关编译命令 |
| ccache 生效 | `ccache -s` 显示 hit 统计（或 sccache 同理） |
| /MP 生效 | MSVC 构建日志出现 `/MP` 标志 |
| 功能不退化 | 所有 24 个现有测试 + 新增测试全绿 |
| 二进制一致 | Release 构建的应用功能等价（/Z7 仅影响调试信息格式，不影响运行时行为） |

> 更新时间：2026-05-02
