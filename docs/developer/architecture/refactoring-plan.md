# 大型重构方案 — dataset-tools（融合音频库迁移）

> 版本：2.0.0
> 日期：2026-06-05
> 状态：方案设计阶段
> 来源：全项目代码探索 + 实际代码对照 + 两份旧方案（v1.0.0 重构方案 + v1.0.0 音频库设计）交叉验证
>
> 本方案全面融合了原有"重构方案"和"独立音频库设计方案"，纠正了两份方案中与实际代码不一致的地方，
> 并根据 [human-decisions.md](../human-decisions.md) 设计准则逐条复核，确保所有设计决策可追溯。

---

## 0. 核心原则

0.1 **功能不变**：所有对外行为（UI 交互、数据格式、流水线步骤、导出结果）保持一致。
0.2 **数据安全**：原子写入、备份机制、不可变快照、校验和验证完整保留并强化。
0.3 **设计准则优先**：所有重构必须通过 [human-decisions.md](../human-decisions.md) 速查表复核。
0.4 **流水线兼容**：不改变 10 步标注流水线的步骤定义、层依赖 DAG 和脏数据传播规则。
0.5 **无历史包袱**：废弃接口、已删除类的残留引用、临时桥接代码全部清理。

---

## 1. 现有方案交叉验证 — 发现的错误与缺失

### 1.1 旧重构方案（refactoring-plan.md v1.0.0）中与实际代码不符之处

| 编号 | 错误 | 实际情况 | 严重度 |
|------|------|---------|--------|
| E1 | Phase 2 说"新建 dsfw-audio" | **dsfw-audio 已存在**于 `src/framework/audio/dsfw-audio/`，有完整实现（AudioBuffer、FfmpegAudioDecoder、SwresampleResampler、AudioPipeline）和单元测试 | 致命 |
| E2 | API 设计描述 `AudioDecoder` 有 `Config` 结构体、`Result<void>`、`seek()` 等 | 实际 dsfw-audio 有 **IAudioDecoder 接口** + FfmpegAudioDecoder 实现，API 与方案描述不同 | 致命 |
| E3 | 依赖图将 dsfw-audio 放在 dsfw-widgets 同级 | 实际 dsfw-audio 是 **dstools-audio 的子目录**，`add_subdirectory(dsfw-audio)` 在 dstools-audio 的 CMakeLists.txt 中 | 高 |
| E4 | 说"移除 audio-util" | audio-util 不仅包含音频 I/O（resample_to_vio, readAudioToMonoFloat），还包含 **Slicer 静音检测算法**，被 8 个模块依赖，不能简单删除 | 高 |
| E5 | 说"移除 libsndfile, soxr, mpg123" | 这些依赖通过 audio-util 被 8 个模块间接使用，移除前必须完成迁移 | 高 |
| E6 | Phase 3 namespace 迁移未提及 `Result<T>` | `Result<T>` 在 `dstools` 命名空间（`src/types/include/dstools/Result.h`），但 dsfw-audio 使用 `dstools::Result<T>`，存在命名空间不一致 | 中 |
| E7 | EditorPageBase 拆分方案过于激进（5 个子组件） | 实际 EditorPageBase 约 800 行，拆分为 5 个组件增加过多间接层，违反 ARCH-05（组合优于继承，但过度组合也是问题） | 中 |
| E8 | Phase 5 新增 8 条设计准则 | 设计准则应维护在 human-decisions.md，重构方案不应定义新准则 | 中 |

### 1.2 旧音频库设计（audio-library-design.md v1.0.0）中与实际代码不符之处

| 编号 | 错误 | 实际情况 | 严重度 |
|------|------|---------|--------|
| E9 | 标注"方案设计阶段" | dsfw-audio 代码**已实现**，AudioBuffer、FfmpegAudioDecoder、SwresampleResampler、AudioPipeline 全部有 `.cpp` 实现 | 致命 |
| E10 | 引用"重构方案 v4.3"的 R1-R16 阶段 | 实际重构方案是 v1.0.0，不存在 v4.3，两个文档版本号不一致 | 高 |
| E11 | IAudioDecoder 和 IAudioResampler 接口 | 违反 **INFRA-02**（接口必要性原则）：各只有一个实现（FfmpegAudioDecoder、SwresampleResampler），无测试 mock 需求 | 高 |
| E12 | 说"播放适配层不在新库范围" | 但旧 dstools-audio 的 AudioPlayer/AudioPlayback/IAudioPlayer 是应用层核心依赖（PlayWidget 使用），不迁移播放层就无法删除旧库 | 高 |
| E13 | 设计中的 `probe()` 参数是 `const std::string&` | 实际代码使用 `const std::string&`（一致），但项目规范要求使用 `std::filesystem::path`（INFRA-05） | 中 |
| E14 | 说"不依赖 Qt" | 实际 dsfw-audio 确实不依赖 Qt（正确），但**没有 Qt 适配层**，导致 PlayWidget 无法使用 | 中 |

### 1.3 两份方案之间的冲突

| 编号 | 冲突 | 重构方案说 | 音频设计说 | 实际代码 |
|------|------|-----------|-----------|---------|
| E15 | AudioDecoder API | 具体类 + Config 结构体 | IAudioDecoder 接口 + FfmpegAudioDecoder | 接口 + 实现（与音频设计一致） |
| E16 | 播放层归属 | 在 dsfw-audio 中 | 不在新库范围 | 仍在 dstools-audio |
| E17 | dsfw-audio 位置 | 独立于 dstools-audio | 独立库 | dstools-audio 的子目录 |

---

## 2. 现状分析（基于实际代码）

### 2.1 音频系统现状

```
现有音频代码（3 套并行）：

┌─ dstools-audio（旧，STATIC，被所有应用代码使用）──────────────┐
│  AudioDecoder    → FFmpeg 解码+重采样一体，固定输出 44100/stereo/float  │
│  AudioPlayback   → SDL2 播放                                       │
│  AudioPlayer     → 组合 Decoder+Playback，实现 IAudioPlayer         │
│  IAudioPlayer    → QObject 接口（PlayWidget 依赖）                  │
│  WaveFormat      → 简单格式描述                                     │
│  已知 Bug: C1(decodeAll 位置重置), C2(setPosition clamp)           │
└────────────────────────────────────────────────────────────────┘

┌─ dsfw-audio（新，STATIC，仅自己的测试使用）────────────────────┐
│  AudioBuffer     → 统一内存容器（float32/int16，view/owned）       │
│  IAudioDecoder   → 解码器接口（仅 FfmpegAudioDecoder 一个实现）     │
│  IAudioResampler → 重采样器接口（仅 SwresampleResampler 一个实现）  │
│  AudioPipeline   → 组合解码+重采样的便捷层                          │
│  AudioFormatInfo → 格式探测结果                                     │
│  ResampleConfig  → 重采样配置                                       │
│  使用 dstools::Result<T>，不在 dsfw 命名空间                        │
└────────────────────────────────────────────────────────────────┘

┌─ audio-util（旧，STATIC，被推理库和 libs 使用）────────────────┐
│  Slicer          → RMS 静音检测（非音频 I/O，是算法）              │
│  resample_to_vio → 基于 libsndfile+soxr 的重采样                  │
│  readAudioToMonoFloat → 读文件+转单声道 float                      │
│  SndfileHelper   → libsndfile 封装                                 │
│  FlacDecoder     → libflac 解码                                    │
│  Mp3Decoder      → mpg123 解码                                     │
└────────────────────────────────────────────────────────────────┘
```

### 2.2 关键依赖关系（实际 CMake）

```
dstools-audio 被使用:  WaveformRenderer, SlicerPage, SlicerService, DsSlicerPage,
                       TestAudioProcessing, MoeCurveProcessor, AudioPlayer（自身）

audio-util 被使用:     rmvpe-infer, hubert-infer, game-infer, moe-lib, lyric-fa,
                       slicer(lib), dstools-domain, apps/shared/data-sources, apps/cli

dsfw-audio 被使用:     仅自己的测试（TestAudioBuffer, TestAudioDecoder, TestAudioResampler, TestAudioPipeline）
```

### 2.3 命名空间混乱（实际代码）

| 类 | 所在目录 | 当前命名空间 | 应属命名空间 |
|----|---------|------------|------------|
| ServiceLocator | src/framework/core/ | `dstools` | `dsfw` |
| Logger | src/framework/core/ | `dstools` | `dsfw` |
| AppSettings | src/framework/core/ | `dstools` | `dsfw` |
| Result\<T\> | src/types/ | `dstools` | `dsfw` |
| AsyncTask | src/framework/core/ | `dstools` | `dsfw` |
| AudioDecoder（旧） | src/framework/audio/ | `dstools::audio` | `dsfw::audio`（迁移后删除） |
| AudioBuffer（新） | src/framework/audio/dsfw-audio/ | `dsfw::audio` | 正确 |
| resample_to_vio | src/infer/audio-util/ | `AudioUtil` | 独立命名空间（迁移后删除） |

### 2.4 应用层架构（实际代码）

```
apps/shared/data-sources/
├── EditorPageBase.h/.cpp    → ~800 行，承载 6 项职责
├── EnginePool.h             → 引擎池管理（已有独立类）
├── SliceListPanel           → 切片列表（已有独立类）
├── BatchProcessDialog       → 批量处理对话框（已有独立类）
├── SlicerPage, PhonemeLabelerPage, PitchLabelerPage, MinLabelPage  → 具体页面
├── ExportService, PitchExtractionService, SliceExportService        → 服务类

apps/shared/bridges/
├── DsTextDocBridge.h/.cpp   → 静态工具类，4 个转换方法，~200 行

apps/shared/audio-visualizer/
├── AudioVisualizerContainer → 音频可视化容器
├── EditorContainerBase      → 编辑器容器基类
├── AudioEditorWidgetBase    → 编辑器 Widget 基类
```

---

## 3. 新融合方案总览

### 3.1 设计决策（每项均对照 human-decisions.md 复核）

| 决策 | 对照准则 | 依据 |
|------|---------|------|
| 移除 IAudioDecoder/IAudioResampler 接口 | INFRA-02（接口必要性） | 各只有一个实现，无测试 mock 需求 |
| FfmpegAudioDecoder/SwresampleResampler 改为具体类 | INFRA-03（实现优先） | 先具体类，未来有需求再抽取接口 |
| 保留 AudioPipeline 作为便捷组合层 | ARCH-05（组合优于继承） | 组合解码+重采样，不引入新接口 |
| 新增 AudioPlayerAdapter 到 dsfw-audio | INFRA-01（ServiceLocator 限定） | 播放器是页面级资源，不全局共享 |
| 将 Slicer 从 audio-util 移到 dsfw-signal | ARCH-01（职责单一） | Slicer 是静音检测算法，不是音频 I/O |
| 移除 libsndfile/soxr/mpg123 依赖 | 统一后端 | FFmpeg 已覆盖所有格式 |
| 命名空间迁移使用 namespace alias 过渡 | ARCH-03（接口稳定） | 渐进式迁移，不破坏现有代码 |
| 不拆分 EditorPageBase 为 5 个子组件 | ARCH-04（60%阈值） | 当前 ~800 行未达到必须拆分的阈值，先做职责内聚 |

### 3.2 分阶段策略

```
Phase 1: dsfw-audio 完善               → ✅ 已完成 (接口修正 + 播放层 + 错误处理 + 命名空间 + CMake独立)
Phase 2: 推理层音频迁移                → audio-util 替换 + Slicer 重定位
Phase 3: 应用层音频迁移                → 旧 AudioDecoder 调用者迁移 + 旧代码删除
Phase 4: 框架层清理                    → 命名空间 ✅ + dstools-widgets 移除 + infer-common 独立
Phase 5: 应用层内聚                    → EditorPageBase 职责整理 + DsTextDocBridge 简化
Phase 6: 质量加固                      → 测试补充 + 安全检查 + 文档更新
```

**每阶段验收标准**：全量编译通过 + 现有测试通过 + 该阶段涉及页面手动回归通过。

### 3.3 目标模块依赖图

```
┌──────────────────────────────────────────────────────────────────┐
│                        应用层 (src/apps/)                        │
│  LabelSuite · DsLabeler · dstools-cli · WidgetGallery           │
│  shared/ → data-sources, audio-visualizer, phoneme-editor,      │
│            pitch-editor, min-label-editor, settings, log-page,   │
│            chart-framework, bridges, mouth-curve-chart           │
└─────┬──────────┬──────────────┬──────────┬──────────┬───────────┘
      │          │              │          │          │
      ▼          ▼              ▼          ▼          ▼
┌──────────────────────────────────────────────────────────────────┐
│  dstools-domain (STATIC)                                         │
│  DsDocument, DsProject, DsTextDocument, DsPitchDocument,        │
│  ModelManager, F0Curve, FormatAdapters, PinyinG2PProvider,      │
│  ExportFormats, ProjectBackupManager, AudioFileResolver          │
└──────────────────────────┬───────────────────────────────────────┘
                           │
┌──────────────────────────┴───────────────────────────────────────┐
│  dsfw-widgets (SHARED DLL)                                       │
│  PlayWidget, FileProgressTracker, ProgressDialog,               │
│  PropertyEditor, SettingsDialog, LogViewer, PathSelector,        │
│  ShortcutManager, ViewportController                             │
└─────┬──────────────┬──────────────┬──────────────────────────────┘
      │              │              │
      ▼              ▼              ▼
┌───────────┐ ┌──────────────┐ ┌──────────────┐
│dsfw-ui-core│ │dsfw-audio    │ │dsfw-infer    │
│(STATIC)   │ │(STATIC)      │ │(STATIC, 新)  │
│           │ │              │ │              │
│AppShell   │ │AudioBuffer   │ │OnnxEnv       │
│IconNavBar │ │FfmpegDecoder │ │OnnxModelBase │
│Theme      │ │SwresampleRsp │ │IInferenceEng │
│IPage*     │ │AudioPipeline │ │              │
│Frameless  │ │AudioPlayback │ │Qt-free       │
│           │ │AudioPlayerAd │ │              │
│           │ │              │ │依赖: ONNX RT │
│           │ │依赖: FFmpeg  │ │              │
│           │ │+ SDL2        │ │              │
└─────┬─────┘ └──────┬───────┘ └──────┬───────┘
      │              │              │
      └──────────────┼──────────────┘
                     │
              ┌──────┴───────┐
              │  dsfw-core   │
              │  (STATIC)    │
              │              │
              │AppSettings   │
              │ServiceLocator│
              │AsyncTask     │
              │Logger        │
              │Pipeline      │
              │IFormatAdapter│
              │IG2PProvider  │
              │IModelProvider│
              └──────┬───────┘
                     │
              ┌──────┴───────┐
              │  dsfw-base   │
              │  (STATIC)    │
              │  Qt-free     │
              │              │
              │JsonHelper    │
              │PathUtils     │
              └──────┬───────┘
                     │
              ┌──────┴───────┐
              │ dsfw-types   │
              │ (HEADER-ONLY)│
              │              │
              │Result<T>     │
              │TimePos       │
              │ExecutionProv │
              └──────────────┘
                     │
              ┌──────┴───────┐
              │ dsfw-signal  │
              │ (STATIC)     │
              │              │
              │CurveTools    │
              │F0Curve       │
              │MusicMath     │
              │Slicer (新)   │
              └──────────────┘
```

**关键变更**：
- **dsfw-audio** 独立为 STATIC 库（从 dstools-audio 子目录移出），包含完整的解码+重采样+播放
- **dsfw-infer** 新增 STATIC 库，从 dsfw-core 中独立
- **dsfw-signal** 新增 Slicer（从 audio-util 移入）
- **移除** dstools-audio（旧）、audio-util（旧）、dstools-widgets（INTERFACE 层）
- **移除** libsndfile、soxr、mpg123 依赖

---

## 4. Phase 1：dsfw-audio 完善

> **目标**：修正 dsfw-audio 的设计缺陷，补充播放层，使其成为可替代旧 dstools-audio 的完整音频库。

### 4.1 移除 IAudioDecoder/IAudioResampler 接口

**依据**：INFRA-02（接口必要性原则）— 各只有一个实现，无测试 mock 需求。

**方案**：
- 删除 `IAudioDecoder.h`、`IAudioResampler.h`
- `FfmpegAudioDecoder` 和 `SwresampleResampler` 改为具体类（已有 PIMPL，头文件不含 FFmpeg）
- `AudioPipeline` 内部直接使用 `FfmpegAudioDecoder` 和 `SwresampleResampler`

**修改文件**：
- 删除：`dsfw-audio/include/dsfw/audio/IAudioDecoder.h`
- 删除：`dsfw-audio/include/dsfw/audio/IAudioResampler.h`
- 修改：`FfmpegAudioDecoder.h` — 移除 `: public IAudioDecoder`，直接声明方法
- 修改：`SwresampleResampler.h` — 移除 `: public IAudioResampler`，直接声明方法
- 修改：`AudioPipeline.h` — 使用具体类型而非接口指针
- 修改：`AudioPipeline.cpp` — 直接构造 FfmpegAudioDecoder/SwresampleResampler

**验收标准**：
- 编译通过，dsfw-audio 测试通过
- `#include <dsfw/audio/IAudioDecoder.h>` 不存在于任何文件中

### 4.2 补充播放层

**依据**：旧 dstools-audio 的 AudioPlayer/AudioPlayback/IAudioPlayer 是 PlayWidget 的核心依赖，必须迁移。

**方案**：新增 `AudioPlaybackAdapter` 和 `AudioPlayerAdapter` 到 dsfw-audio（Qt 适配层）。

```
dsfw-audio 新增文件:
├── AudioPlaybackAdapter.h/.cpp  → SDL2 播放（从旧 AudioPlayback 迁移，改进错误处理）
├── AudioPlayerAdapter.h/.cpp    → 组合 FfmpegAudioDecoder + AudioPlaybackAdapter
└── IAudioPlayerAdapter.h        → 播放器接口（Qt 信号，供 PlayWidget 使用）
```

**API 设计**：

```cpp
namespace dsfw::audio {

// 播放器接口（保留 QObject + 信号，供 PlayWidget 使用）
class IAudioPlayerAdapter : public QObject {
    Q_OBJECT
public:
    static constexpr int kInterfaceVersion = 1;
    enum class State { Stopped, Playing };

    explicit IAudioPlayerAdapter(QObject* parent = nullptr) : QObject(parent) {}
    ~IAudioPlayerAdapter() override = default;

    virtual Result<void> open(const std::filesystem::path& path) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual double duration() const = 0;
    virtual double position() const = 0;
    virtual void setPosition(double sec) = 0;
    virtual void play() = 0;
    virtual void stop() = 0;
    virtual bool isPlaying() const = 0;
    virtual QStringList devices() const = 0;
    virtual QString currentDevice() const = 0;
    virtual void setDevice(const QString& device) = 0;
    virtual bool setup(int sampleRate, int channels, int bufferSize) = 0;

signals:
    void stateChanged(State newState);
    void deviceChanged(const QString& device);
};

// 具体实现
class AudioPlayerAdapter : public IAudioPlayerAdapter {
    Q_OBJECT
public:
    explicit AudioPlayerAdapter(QObject* parent = nullptr);
    ~AudioPlayerAdapter() override;

    Result<void> open(const std::filesystem::path& path) override;
    // ... 其余方法

private:
    std::unique_ptr<AudioPlaybackAdapter> m_playback;
    std::unique_ptr<FfmpegAudioDecoder> m_decoder;
};

} // namespace dsfw::audio
```

**关键改进**（相对于旧 dstools-audio）：
- `open()` 返回 `Result<void>` 而非 `bool`（ROBUST-02）
- 路径参数使用 `std::filesystem::path` 而非 `QString`（INFRA-05）
- 内部使用新的 `FfmpegAudioDecoder`（输出原始格式，解码和重采样分离）
- 修复旧 Bug：C1（decodeAll 位置重置）、C2（setPosition clamp）

**验收标准**：
- 编译通过
- 手动测试：PlayWidget 可正常播放 WAV/MP3/FLAC/OGG
- 手动测试：seek、设备切换、范围播放正常

### 4.3 命名空间与 Result 类型修正

**问题**：dsfw-audio 当前使用 `dstools::Result<T>`，但自身在 `dsfw::audio` 命名空间。

**方案**（短期，Phase 1 执行）：
- 在 dsfw-audio 内部使用 `dstools::Result<T>` 保持不变（Phase 4 统一迁移 Result 到 dsfw 命名空间）
- 确保所有头文件 `#include <dstools/Result.h>` 路径正确

**长期方案**（Phase 4 执行）：
- 将 `Result<T>` 从 `dstools` 命名空间迁移到 `dsfw`
- 添加 `namespace dstools { using dsfw::Result; }` 兼容过渡

### 4.4 错误处理加固

**问题**：当前 FfmpegAudioDecoder 和 SwresampleResampler 的 FFmpeg 调用缺少 try-catch 保护。

**方案**：在 FFmpeg 边界添加 try-catch（ROBUST-02），将 FFmpeg 错误码转为 `Result<T>::Error()` 并包含 av_strerror() 消息。

**修改文件**：
- `FfmpegAudioDecoder.cpp` — 所有 FFmpeg API 调用包装错误检查
- `SwresampleResampler.cpp` — swr_convert 等调用添加错误信息

**验收标准**：
- 传入无效文件路径时，返回 `Result<T>::Error()` 含具体错误信息
- 传入损坏的音频文件时，不崩溃，返回错误

### 4.5 dsfw-audio 独立为顶层 CMake 目标

**问题**：dsfw-audio 当前是 dstools-audio 的子目录，依赖关系不清晰。

**方案**：
- 将 `src/framework/audio/dsfw-audio/` 提升为 `src/framework/audio/` 下的独立 target
- CMakeLists.txt 中 `add_subdirectory(dsfw-audio)` 改为直接在 `src/framework/audio/CMakeLists.txt` 中定义（与 dstools-audio 并列）
- 目标名称：`dsfw-audio`（不变）

**验收标准**：
- `cmake --build --preset release` 编译通过
- dsfw-audio 与 dstools-audio 并列编译，无循环依赖

---

## 5. Phase 2：推理层音频迁移

> **目标**：将 audio-util 的音频 I/O 功能替换为 dsfw-audio，将 Slicer 算法重定位到正确模块。

### 5.1 Slicer 重定位到 dsfw-signal

**依据**：ARCH-01（职责单一）— Slicer 是 RMS 静音检测算法，不是音频 I/O。dsfw-signal 是信号处理模块，职责匹配。

**方案**：
- 从 `src/infer/audio-util/` 中提取 `Slicer.h` / `Slicer.cpp`
- 移动到 `src/framework/signal/`（dsfw-signal 模块）
- 命名空间从 `AudioUtil` 改为 `dsfw::signal`
- `SlicerParams` 和 `SlicerError` 一并迁移

**修改文件**：
- 新增：`src/framework/signal/include/dsfw/signal/Slicer.h`
- 新增：`src/framework/signal/src/Slicer.cpp`
- 修改：`src/libs/slicer/` — 更新 include 路径
- 修改：`src/libs/slicer/CMakeLists.txt` — 链接 dsfw-signal 替代 audio-util
- 删除：`src/infer/audio-util/include/audio-util/Slicer.h`
- 删除：`src/infer/audio-util/src/Slicer.cpp`

**验收标准**：
- 编译通过
- slicer lib 的切片功能正常（手动测试 SlicerPage）

### 5.2 推理库迁移到 dsfw-audio

**当前调用者**（使用 audio-util 的 `resample_to_vio` / `readAudioToMonoFloat`）：

| 调用者 | 使用的函数 | 迁移方式 |
|--------|-----------|---------|
| rmvpe-infer (Rmvpe.cpp) | `resample_to_vio` | 替换为 `AudioPipeline::decodeToMonoFloat` |
| hubert-infer (Hfa.cpp) | `resample_to_vio` | 替换为 `AudioPipeline::decodeToMonoFloat` |
| game-infer (Game.cpp) | `resample_to_vio` | 替换为 `AudioPipeline::decodeToMonoFloat` |
| moe-lib (MoeCurveProcessor.cpp) | `resample_to_vio` | 替换为 `AudioPipeline::decodeToMonoFloat` |
| lyric-fa (AsrPipeline.cpp) | `readAudioToMonoFloat` | 替换为 `AudioPipeline::decodeToMonoFloat` |
| dstools-domain (CsvToDsConverter.cpp) | `resample_to_vio` | 替换为 `AudioPipeline::decodeToMonoFloat` |
| apps/cli (main.cpp) | `resample_to_vio` | 替换为 `AudioPipeline::decodeToMonoFloat` |
| libs/slicer/ | Slicer（算法）+ 音频 I/O | Slicer 移到 dsfw-signal，I/O 替换为 AudioPipeline |

**方案**：逐文件替换，每个文件替换后验证编译。

**验收标准**：
- 所有推理库编译通过
- `resample_to_vio` 和 `readAudioToMonoFloat` 不再被任何非 audio-util 文件引用

### 5.3 删除 audio-util 和旧依赖

**方案**：
- 删除 `src/infer/audio-util/` 整个目录
- 从 CMake 中移除 `SndFile::sndfile`、`soxr`、`mpg123`、`libflac` 依赖（确认无其他使用者后）
- 从 vcpkg.json 中移除对应包（如无其他使用者）

**验收标准**：
- 编译通过
- `find_package(SndFile)` 不再出现
- audio-util 目录不存在

---

## 6. Phase 3：应用层音频迁移

> **目标**：将应用层所有旧 AudioDecoder 调用迁移到 dsfw-audio，删除旧 dstools-audio。

### 6.1 旧 AudioDecoder 调用者迁移

**当前调用者**（使用 `dstools/AudioDecoder.h`）：

| 调用者 | 用途 | 迁移方式 |
|--------|------|---------|
| WaveformRenderer | 解码音频绘制波形 | 替换为 `AudioPipeline::decodeAndResample` |
| SlicerPage | 切片时解码音频 | 替换为 `AudioPipeline::decodeAndResample` |
| SlicerService | 切片服务中解码音频 | 替换为 `AudioPipeline::decodeAndResample` |
| DsSlicerPage | DsLabeler 切片 | 替换为 `AudioPipeline::decodeAndResample` |
| MoeCurveProcessor | 口型曲线处理 | 替换为 `AudioPipeline::decodeToMonoFloat` |
| TestAudioProcessing | 测试代码 | 替换为 `AudioPipeline` |

**关键注意事项**：
- 旧 AudioDecoder 输出固定 44100Hz/stereo/float32，新 dsfw-audio 输出原始格式
- WaveformRenderer 需要原始采样率绘制时间轴，需要在 ResampleConfig 中指定
- 应用层手动混音代码（3 处）统一替换为 `SwresampleResampler` 或 `ResampleConfig::targetChannelCount = 1`

**验收标准**：
- 所有文件编译通过
- `#include <dstools/AudioDecoder.h>` 不存在于任何非 dstools-audio 文件中
- 波形图渲染正确
- 切片功能正常

### 6.2 PlayWidget 迁移

**方案**：
- PlayWidget 当前依赖 `dstools::audio::IAudioPlayer`
- 改为依赖 `dsfw::audio::IAudioPlayerAdapter`（Phase 1 中新增）

**修改文件**：
- `PlayWidget.h` — 更新 include 和类型
- `PlayWidget.cpp` — 更新构造函数

**验收标准**：
- 编译通过
- 播放/暂停/停止/seek/设备切换正常
- 范围播放正常

### 6.3 删除旧 dstools-audio

**方案**：
- 删除 `src/framework/audio/src/AudioDecoder.cpp`
- 删除 `src/framework/audio/src/AudioPlayback.cpp`
- 删除 `src/framework/audio/src/AudioPlayer.cpp`
- 删除 `src/framework/audio/include/dstools/` 下所有旧头文件
- 更新 `src/framework/audio/CMakeLists.txt` — 移除旧 target 定义
- 从所有 CMakeLists.txt 中移除 `dstools-audio` 链接

**验收标准**：
- 编译通过
- 旧 dstools-audio 目录中只保留 dsfw-audio 子目录
- `target_link_libraries(... dstools-audio)` 不存在

---

## 7. Phase 4：框架层清理

### 7.1 命名空间迁移 ✅ 已完成

**方案**：渐进式迁移，使用 namespace alias 过渡。

| 类 | 当前命名空间 | 目标命名空间 | 过渡方式 |
|----|------------|------------|---------|
| ServiceLocator | `dstools` | `dsfw` | namespace alias |
| Logger | `dstools` | `dsfw` | namespace alias |
| LogNotifier | `dstools` | `dsfw` | namespace alias |
| AppSettings | `dstools` | `dsfw` | namespace alias |
| AsyncTask | `dstools` | `dsfw` | namespace alias |
| Result\<T\> | `dstools` | `dsfw` | namespace alias |
| FormatAdapterRegistry | `dstools` | `dsfw` | namespace alias |
| TaskProcessorRegistry | `dstools` | `dsfw` | namespace alias |

**步骤**：
1. 在每个头文件中将类定义放入 `dsfw` 命名空间
2. 在头文件末尾添加 `namespace dstools { using dsfw::ClassName; }`
3. 更新所有 `#include` 路径中的框架引用
4. 一个版本后移除 `namespace dstools` 中的 using 声明

**验收标准**：
- 编译通过
- 框架层代码（`src/framework/`、`src/types/`）中不再有 `namespace dstools {` 定义框架类

### 7.2 infer-common 独立为 dsfw-infer

**依据**：当前 OnnxEnv.cpp 和 OnnxModelBase.cpp 通过 `target_sources` 编译入 dsfw-core，导致 dsfw-core 依赖 ONNX Runtime。

**方案**：
- 新建 `src/framework/infer/` 目录
- 移动 `src/framework/core/src/OnnxEnv.cpp` 和 `OnnxModelBase.cpp` 到 dsfw-infer
- 新建 `dsfw-infer` STATIC CMake target
- 更新所有推理库的 CMakeLists.txt 链接 dsfw-infer

**验收标准**：
- 编译通过
- dsfw-core 不再依赖 onnxruntime
- 所有推理库正确链接 dsfw-infer

### 7.3 移除 dstools-widgets INTERFACE 层

**方案**：
- 将导出宏定义合并到 dsfw-widgets
- 删除 `src/ui-core/` 下的 dstools-widgets 定义
- 应用层直接链接 `dsfw::widgets`

**验收标准**：
- 编译通过
- dstools-widgets 的 CMakeLists.txt 不存在

### 7.4 SettingsKey 集中管理

**方案**：
- 所有 SettingsKey 定义在 `src/apps/shared/settings/Keys.h` 中
- `CommonKeys.h` 中的框架级 key 也纳入统一管理
- 禁止在其他文件中使用 `SettingsKey("...", ...)` 临时定义

**验收标准**：
- Grep `SettingsKey<` 仅出现在 `Keys.h` 和 `AppSettings.h` 中

---

## 8. Phase 5：应用层内聚

### 8.1 EditorPageBase 职责整理

**依据**：ARCH-04（60%阈值）— 当前 ~800 行，尚未达到必须拆分的阈值。采用**职责内聚**而非**拆分为独立类**。

**方案**（轻量级）：

```
EditorPageBase（不拆分，按区域整理）:
├── [区域1] 生命周期管理    → IPageLifecycle 实现（onPageEnter/Leave）
├── [区域2] 菜单/工具栏      → IPageActions 实现（setupMenu/setupToolBar）
├── [区域3] 批量处理        → 委托给 BatchProcessDialog（已有独立类）
├── [区域4] 引擎池管理      → 委托给 EnginePool（已有独立类）
├── [区域5] 切片上下文      → 内联方法，不独立成类
└── [区域6] 标准 UI 辅助    → 内联方法（createStandardLayout 等）
```

**具体行动**：
1. 在 EditorPageBase.cpp 中用 `#pragma region` 注释分隔 6 个区域
2. 提取重复的 UI 创建代码为私有辅助方法
3. 将 `BatchProcessRunner` 逻辑委托给 `BatchProcessDialog`（已存在）
4. 不创建新的子组件类

**验收标准**：
- 编译通过
- 所有页面功能不变
- 代码可读性提升（区域分隔清晰）

### 8.2 DsTextDocBridge 简化

**依据**：当前是静态工具类，~200 行，4 个转换方法，复杂度可控。

**方案**：
- 保持为静态工具类，不拆分
- 清理未使用的 `verifyLayerRoundtrip` 和 `verifyPitchDocRoundtrip`（如果仅在测试中使用，移到测试文件）
- 添加文档注释说明每个转换方向

**验收标准**：
- 编译通过
- 往返测试通过
- 代码可读性提升

### 8.3 libs 层与 shared 层职责对齐

**方案**（明确边界）：

| 层 | 职责 | 可依赖 |
|----|------|--------|
| libs/ | 纯业务逻辑 + ITaskProcessor 实现（无 UI 依赖） | dsfw-core, dstools-domain, dsfw-infer, dsfw-audio |
| apps/shared/ | UI 组件 + 页面服务 + 数据源（有 Qt Widgets 依赖） | dsfw-widgets, dstools-domain, libs |

**验收标准**：
- libs 中的代码可被 CLI 工具复用
- 无循环依赖

---

## 9. Phase 6：质量加固

### 9.1 单元测试补充

| 优先级 | 模块 | 当前状态 | 行动 |
|--------|------|---------|------|
| P0 | dsfw-audio（已有测试） | 有测试 | 补充边界情况：损坏文件、空文件、seek 边界 |
| P0 | AudioPlayerAdapter | 无测试 | 新增测试 |
| P1 | PipelineRunner | 有测试 | 补充边界情况 |
| P1 | 迁移后的推理库 | 无专项测试 | 验证解码+重采样管线正确性 |
| P2 | PlayWidget | 无测试 | 新增播放控制逻辑测试 |

### 9.2 并发安全检查

1. 审查所有 `QtConcurrent::run` 调用，确保有 QPointer guard 或存活令牌
2. 审查所有 `QMetaObject::invokeMethod` 调用，确保上下文有效性检查
3. 审查所有共享可变状态，确保有 mutex 或 atomic 保护

### 9.3 错误处理审查

1. 审查所有 `catch` 块，确保不静默吞掉异常
2. 审查所有 `Result<T>::Error()` 调用，确保错误消息追溯到根因
3. 审查所有 out-parameter 错误返回，确保调用者检查

### 9.4 文档更新

1. 更新 `overview.md` 中的模块依赖图
2. 更新 `human-decisions.md` 附录 A（已废止决策）
3. 更新 `audio-library-design.md` 状态为"已实现"
4. 更新 `component-design.md` 中的核心组件说明

---

## 10. 实施依赖与风险

### 10.1 执行顺序（严格顺序依赖）

```
Phase 1 (dsfw-audio 完善)
  ├─ 1.1 移除接口          ← 无依赖，优先执行
  ├─ 1.2 补充播放层         ← 依赖 1.1
  ├─ 1.3 命名空间修正       ← 无依赖，可与 1.1 并行
  ├─ 1.4 错误处理加固       ← 无依赖，可与 1.1 并行
  └─ 1.5 独立 CMake 目标   ← 依赖 1.1~1.4

Phase 2 (推理层迁移)
  ├─ 2.1 Slicer 重定位      ← 依赖 Phase 1 完成
  ├─ 2.2 推理库迁移         ← 依赖 Phase 1 完成
  └─ 2.3 删除 audio-util   ← 依赖 2.1 + 2.2

Phase 3 (应用层迁移)
  ├─ 3.1 旧调用者迁移       ← 依赖 Phase 1 + 2
  ├─ 3.2 PlayWidget 迁移   ← 依赖 1.2
  └─ 3.3 删除旧 dstools-audio ← 依赖 3.1 + 3.2

Phase 4 (框架层清理)       ← 依赖 Phase 1~3 完成
Phase 5 (应用层内聚)       ← 依赖 Phase 1~4 完成
Phase 6 (质量加固)         ← 依赖 Phase 1~5 完成
```

### 10.2 风险矩阵

| 风险 | 严重度 | 缓解措施 |
|------|--------|---------|
| 音频播放行为变化（seek、范围播放） | 高 | 每个格式单独验证，Phase 3 前保留旧库作为回退 |
| 推理结果变化（重采样算法从 soxr 切换到 swresample） | 高 | 逐模型对比推理输出，确保差异在可接受范围 |
| 编译错误连锁反应 | 中 | 每步提交，可独立回退 |
| audio-util 删除后遗漏引用 | 中 | Grep 验证所有引用已迁移 |

### 10.3 回退策略

每个 Phase 独立提交。Phase 1 完成后，旧 dstools-audio 和 audio-util 保留至少到 Phase 3 验证通过，确保无回归后才删除。

---

## 11. 附录：重构检查清单

每个 Phase 完成后，必须通过以下检查：

- [ ] `cmake --build --preset release` 编译通过（0 error）
- [ ] 所有现有单元测试通过
- [ ] `clang-format` 格式检查通过
- [ ] 手动回归测试：该 Phase 涉及的功能正常
- [ ] 设计文档更新（如涉及）
- [ ] Memory MCP 更新（新增实体、更新 common-agent-errors）