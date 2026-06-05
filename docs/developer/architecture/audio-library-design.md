# 独立音频库设计方案 — dsfw-audio

> 版本：1.0.0
> 日期：2026-06-04
> 状态：方案设计阶段
> 依赖：FFmpeg（解码 + 重采样），无 Qt 依赖（仅核心库，播放适配层可选）

---

## 0. 前置声明

### 0.1 方案目标

1. 构建一个**独立、高可靠**的 C++20 音频处理库，使用 FFmpeg 作为唯一第三方后端
2. 统一当前分散在两处的音频处理逻辑（`dstools-audio` + `audio-util`）
3. 支持常见格式：WAV、FLAC、MP3、OGG、M4A/AAC、Opus 等（FFmpeg 原生支持的所有音频格式）
4. 支持多声道流式解码 + 任意声道重采样 + 任意采样率转换
5. 支持重采样结果暂存内存（`AudioBuffer`），方便 HFA/RMVPE 等模型直接使用
6. 严格遵循项目设计准则（ARCH/ROBUST/CONCUR/INFRA 系列）
7. 核心库不依赖 Qt，仅播放适配层可选地依赖 Qt/SDL2

### 0.2 方案约束

- 不引入 FFmpeg 之外的第三方依赖
- 遵循 ARCH-07（开闭原则）：新增库，不修改 `dstools-audio` 稳定代码
- 遵循 ROBUST-02：`Result<T>` 传播错误，`try-catch` 仅限 FFmpeg 边界
- 遵循 INFRA-13：PIMPL 隔离 FFmpeg 头文件
- 遵循 INFRA-06：RAII 管理所有资源
- 遵循 CONCUR-01：>50ms 操作异步化
- 保持 C++20 / 4-space indent / 120-col / `#pragma once`

---

## 1. 现状分析

### 1.1 现有音频代码分布

| 位置 | 模块 | 功能 | 后端 | 问题 |
|------|------|------|------|------|
| `src/framework/audio/` | `dstools-audio` (STATIC) | 解码 + 播放 | FFmpeg + SDL2 | 输出固定 44100/stereo/float32；无独立重采样 API；布尔返回值 |
| `src/infer/audio-util/` | `audio-util` (STATIC) | 重采样 + 格式转换 | libsndfile + soxr + mp3/flac decoder | 仅支持 WAV/MP3/FLAC；用 soxr（项目已有 FFmpeg 却不用 swresample）；无统一接口 |
| `src/apps/shared/` | 应用层多处 | 手动混音为单声道 | 通过 AudioDecoder 读取后手动平均 | 代码重复（SlicerPage、SlicerService、WaveformRenderer 各写一份） |

### 1.2 关键问题

| 编号 | 问题 | 严重度 |
|------|------|--------|
| P1 | 两套并行的音频后端（FFmpeg vs libsndfile+soxr），功能重叠 | 高 |
| P2 | `AudioDecoder` 解码和重采样耦合，无法独立控制 | 高 |
| P3 | 无独立 `AudioResampler` API，RMVPE 端用独立的 soxr 路径 | 高 |
| P4 | 应用层手动混音代码重复 3+ 处，无标准化通道映射 | 中 |
| P5 | 不支持 OGG/Opus/M4A 等格式（`audio-util` 只支持 WAV/MP3/FLAC） | 中 |
| P6 | 无 `Result<T>` 错误处理，错误信息通过 `std::cerr` 丢弃 | 中 |
| P7 | 无单元测试覆盖 | 中 |
| P8 | `resample_to_vio` 输出为 PCM16 WAV VIO 格式，模型侧需二次解析 | 低 |

### 1.3 现有模块依赖关系

```
dstools-audio (STATIC, Layer 2)
    ├── PUBLIC:  Qt::Core
    ├── PRIVATE: FFmpeg (libavformat, libavcodec, libswresample, libavutil)
    └── PRIVATE: SDL2::SDL2

audio-util (STATIC, infer/)
    ├── PUBLIC:  (无)
    └── PRIVATE: libsndfile, soxr, mpg123, libflac
```

---

## 2. 设计目标与 API 能力矩阵

### 2.1 核心能力

| 能力 | 优先级 | 说明 |
|------|--------|------|
| 流式解码 | P0 | 分帧读取，支持大文件（>1h），内存可控 |
| 全量解码 | P0 | 一键解码到内存（小文件便捷模式） |
| 采样率转换 | P0 | 任意 → 任意采样率（8000~192000），支持多种质量等级 |
| 通道重映射 | P0 | 任意声道 → 任意声道（含立体声→单声道混音、单声道→立体声复制） |
| 样本格式转换 | P0 | 支持 float32 / int16 / int32 互转 |
| 内存缓冲区 | P0 | `AudioBuffer` 统一容器，支持 view/owned 两种模式 |
| 格式探测 | P1 | 不解码情况下获取时长、采样率、声道数、编码格式 |
| 分段解码 | P1 | 按时间区间解码（如 "解码 1.5s~3.2s 区间"） |
| 播放适配 | P1 | 可选 Qt/SDL2 适配层（复用现有 AudioPlayback） |
| 元数据读取 | P2 | 读取 artist/title/album 等标签（可选） |

### 2.2 支持的格式

通过 FFmpeg 原生支持所有常见音频格式：

| 格式 | 扩展名 | 编码 |
|------|--------|------|
| WAV | .wav | PCM16/24/32, Float32 |
| FLAC | .flac | FLAC |
| MP3 | .mp3 | MPEG Layer 3 |
| OGG Vorbis | .ogg | Vorbis |
| OGG Opus | .opus | Opus |
| M4A/AAC | .m4a, .aac | AAC |
| WMA | .wma | Windows Media Audio |
| AIFF | .aiff, .aif | PCM |
| 其他 | * | FFmpeg 支持的所有音频格式 |

---

## 3. 架构设计

### 3.1 模块分层

```
┌──────────────────────────────────────────────────────────────┐
│                    dsfw-audio (新独立库)                       │
│                                                              │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  AudioBuffer (值类型)                                │    │
│  │  - 统一内存容器，支持 float32/int16                  │    │
│  │  - view (引用) / owned (拥有) 两种模式               │    │
│  │  - 零拷贝 slice / subBuffer                         │    │
│  └─────────────────────────────────────────────────────┘    │
│                            ↑                                  │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  IAudioDecoder (纯虚接口)                            │    │
│  │  - probe()  → AudioFormatInfo                       │    │
│  │  - decode() → Result<AudioBuffer>                    │    │
│  │  - decodeSegment(startSec, endSec) → Result<...>    │    │
│  └─────────────────────────────────────────────────────┘    │
│                            ↑                                  │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  FfmpegAudioDecoder (PIMPL)                         │    │
│  │  - FFmpeg 解封装 + 解码                             │    │
│  │  - 流式 read() 接口                                 │    │
│  │  - 支持 seek 到指定时间                             │    │
│  └─────────────────────────────────────────────────────┘    │
│                            ↑                                  │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  IAudioResampler (纯虚接口)                          │    │
│  │  - convert(AudioBuffer src, ResampleConfig) →       │    │
│  │    Result<AudioBuffer>                               │    │
│  └─────────────────────────────────────────────────────┘    │
│                            ↑                                  │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  SwresampleResampler (PIMPL)                        │    │
│  │  - 基于 FFmpeg libswresample                        │    │
│  │  - 支持 sample rate / channel layout / sample fmt   │    │
│  └─────────────────────────────────────────────────────┘    │
│                                                              │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  AudioPipeline (组合解码+重采样)                      │    │
│  │  - 便捷组合：解码 → 重采样 → AudioBuffer             │    │
│  │  - 流式：decodeChunk() + resampleChunk()            │    │
│  └─────────────────────────────────────────────────────┘    │
│                                                              │
└──────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌──────────────────────────────────────────────────────────────┐
│  播放适配层（可选，存于 dstools-audio 或新模块）               │
│                                                              │
│  AudioPlaybackAdapter : IAudioPlayer                         │
│  - 桥接 dsfw-audio::IAudioDecoder → SDL2 回放              │
│  - 复用现有 AudioPlayback 逻辑                              │
└──────────────────────────────────────────────────────────────┘
```

### 3.2 命名空间

```cpp
namespace dsfw::audio {
    // 核心类型
    class AudioBuffer;
    struct AudioFormatInfo;
    struct ResampleConfig;

    // 接口
    class IAudioDecoder;
    class IAudioResampler;

    // 实现
    class FfmpegAudioDecoder;
    class SwresampleResampler;

    // 组合
    class AudioPipeline;
}
```

### 3.3 CMake 目标

```cmake
# cmake/audio/FindFFmpeg.cmake 或复用现有 ffmpeg_link_all
project(dsfw-audio)

dsfw_add_library(${PROJECT_NAME}
    STATIC
    EXPORT_SET dsfwTargets
    DEPENDS
    PUBLIC  dsfw::types      # Result<T>, TimePos
    PUBLIC  dsfw::base       # 可选，PathUtils 等
    PRIVATE FFmpeg::avformat
    PRIVATE FFmpeg::avcodec
    PRIVATE FFmpeg::swresample
    PRIVATE FFmpeg::avutil
)
```

依赖链：
```
dsfw-audio → dsfw-types (header-only, Result<T>)
dsfw-audio → dsfw-base   (可选，PathUtils)
```

**核心库不依赖 Qt**。Qt 依赖仅出现在播放适配层（可选）。

---

## 4. 核心 API 设计

### 4.1 AudioBuffer — 统一内存容器

```cpp
#pragma once

#include <cstdint>
#include <dsfw/Result.h>
#include <memory>
#include <span>
#include <vector>

namespace dsfw::audio {

    /// @brief Sample format enumeration.
    enum class SampleFormat : uint8_t {
        Unknown = 0,
        Float32,    ///< 32-bit IEEE float [-1.0, 1.0]
        Int16,      ///< 16-bit signed integer
        Int32,      ///< 32-bit signed integer
    };

    /// @brief Get bytes per sample for a format.
    constexpr int bytesPerSample(SampleFormat fmt) {
        switch (fmt) {
        case SampleFormat::Float32: return 4;
        case SampleFormat::Int16:   return 2;
        case SampleFormat::Int32:   return 4;
        default: return 0;
        }
    }

    /// @brief Unified audio buffer container with view/owned semantics.
    ///
    /// Memory layout: interleaved samples (channel 0, channel 1, ..., channel 0, channel 1, ...)
    /// Total samples = frameCount * channelCount
    class AudioBuffer {
    public:
        // -- Factory methods --

        /// @brief Create an owned buffer with allocated memory.
        ///        Data is uninitialized (use zero() to fill).
        static AudioBuffer create(int64_t frameCount, int channelCount,
                                  SampleFormat format = SampleFormat::Float32);

        /// @brief Create an owned buffer by copying external data.
        static AudioBuffer fromCopy(const void *data, int64_t frameCount, int channelCount,
                                    SampleFormat format);

        /// @brief Create a view (non-owning) over external data.
        ///        Caller must ensure data lifetime exceeds buffer usage.
        static AudioBuffer fromView(const void *data, int64_t frameCount, int channelCount,
                                    SampleFormat format);

        /// @brief Create an owned buffer by moving a vector.
        static AudioBuffer fromVector(std::vector<uint8_t> &&data, int64_t frameCount,
                                      int channelCount, SampleFormat format);

        // -- Accessors --

        [[nodiscard]] int64_t frameCount() const { return m_frameCount; }
        [[nodiscard]] int channelCount() const { return m_channelCount; }
        [[nodiscard]] SampleFormat format() const { return m_format; }
        [[nodiscard]] int64_t sampleCount() const { return m_frameCount * m_channelCount; }
        [[nodiscard]] size_t byteSize() const { return m_data.size(); }
        [[nodiscard]] bool isView() const { return m_isView; }
        [[nodiscard]] bool empty() const { return m_data.empty(); }

        /// @brief Duration in seconds at given sample rate.
        [[nodiscard]] double durationSec(int sampleRate) const;

        // -- Data access --

        [[nodiscard]] const uint8_t *rawData() const { return m_data.data(); }
        [[nodiscard]] uint8_t *rawData() { return m_data.data(); }

        /// @brief Get float samples (asserts Float32 format).
        [[nodiscard]] std::span<const float> floats() const;
        [[nodiscard]] std::span<float> floats();

        /// @brief Get int16 samples (asserts Int16 format).
        [[nodiscard]] std::span<const int16_t> int16s() const;
        [[nodiscard]] std::span<int16_t> int16s();

        /// @brief Access a single channel as float (no deinterleave, direct mapping).
        [[nodiscard]] float sampleAt(int64_t frame, int channel) const;

        // -- Utility --

        /// @brief Zero all samples.
        void zero();

        /// @brief Create a deep copy.
        [[nodiscard]] AudioBuffer clone() const;

        /// @brief Slice a range of frames (zero-copy if view, copy if owned).
        [[nodiscard]] AudioBuffer slice(int64_t startFrame, int64_t frameCount) const;

    private:
        AudioBuffer() = default;

        std::vector<uint8_t> m_data;
        int64_t m_frameCount = 0;
        int m_channelCount = 0;
        SampleFormat m_format = SampleFormat::Unknown;
        bool m_isView = false;
    };

} // namespace dsfw::audio
```

### 4.2 AudioFormatInfo — 格式探测结果

```cpp
namespace dsfw::audio {

    /// @brief Audio format info obtained without full decoding.
    struct AudioFormatInfo {
        int sampleRate = 0;
        int channelCount = 0;
        SampleFormat sampleFormat = SampleFormat::Unknown;
        int64_t totalFrameCount = 0;   ///< -1 if unknown (streaming)
        double durationSec = 0.0;
        int bitRate = 0;               ///< kbps, 0 if unknown
        std::string codecName;         ///< "pcm_s16le", "flac", "mp3", "vorbis", etc.
        std::string containerName;     ///< "wav", "flac", "mp3", "ogg", etc.
    };

} // namespace dsfw::audio
```

### 4.3 IAudioDecoder — 解码器接口

```cpp
namespace dsfw::audio {

    /// @brief Abstract audio decoder interface.
    ///
    /// Implementations decode audio files to AudioBuffer.
    /// Follows ARCH-06 (DIP) and ARCH-07 (OCP).
    class IAudioDecoder {
    public:
        virtual ~IAudioDecoder() = default;

        /// @brief Probe audio file metadata without decoding.
        /// @param path File path (UTF-8).
        /// @return Format info or error.
        virtual Result<AudioFormatInfo> probe(const std::string &path) = 0;

        /// @brief Open a file for streaming decode.
        /// @param path File path (UTF-8).
        /// @return Ok or error.
        virtual Result<void> open(const std::string &path) = 0;

        /// @brief Close the current file.
        virtual void close() = 0;

        /// @brief Check if a file is open.
        [[nodiscard]] virtual bool isOpen() const = 0;

        /// @brief Get format info of the opened file.
        [[nodiscard]] virtual const AudioFormatInfo &formatInfo() const = 0;

        /// @brief Decode the entire file into memory.
        ///        Convenience method; for large files use streaming read().
        virtual Result<AudioBuffer> decodeAll() = 0;

        /// @brief Decode a time segment.
        /// @param startSec Start time in seconds.
        /// @param endSec End time in seconds.
        virtual Result<AudioBuffer> decodeSegment(double startSec, double endSec) = 0;

        /// @brief Streaming read: decode next chunk of samples.
        /// @param frameCount Max frames to read.
        /// @return Buffer with decoded samples; empty buffer means EOF.
        virtual Result<AudioBuffer> read(int64_t frameCount) = 0;

        /// @brief Seek to a time position for subsequent read() calls.
        virtual Result<void> seekToTime(double sec) = 0;

        /// @brief Get current read position in seconds.
        [[nodiscard]] virtual double currentTime() const = 0;

        /// @brief Get total duration in seconds.
        [[nodiscard]] virtual double totalDuration() const = 0;
    };

} // namespace dsfw::audio
```

### 4.4 ResampleConfig — 重采样配置

```cpp
namespace dsfw::audio {

    /// @brief Resampling quality level.
    enum class ResampleQuality : uint8_t {
        Draft,      ///< Fast, lower quality (linear interpolation)
        Medium,     ///< Balanced (default)
        High,       ///< High quality (soxr HQ equivalent)
        Ultra,      ///< Maximum quality (slowest)
    };

    /// @brief Downmix strategy for multi-channel to fewer channels.
    enum class DownmixStrategy : uint8_t {
        Average,    ///< Average all channels (default)
        Drop,       ///< Drop extra channels
        SelectFirst,///< Keep only the first N channels
    };

    /// @brief Configuration for audio resampling.
    struct ResampleConfig {
        int targetSampleRate = 0;       ///< 0 = keep original
        int targetChannelCount = 0;     ///< 0 = keep original
        SampleFormat targetFormat = SampleFormat::Unknown;  ///< Unknown = keep original
        ResampleQuality quality = ResampleQuality::High;
        DownmixStrategy downmix = DownmixStrategy::Average;

        /// @brief Convenience: create config for mono float32 at target rate.
        static ResampleConfig forMonoFloat(int sampleRate) {
            return {sampleRate, 1, SampleFormat::Float32, ResampleQuality::High, DownmixStrategy::Average};
        }

        /// @brief Convenience: create config for stereo float32 at target rate.
        static ResampleConfig forStereoFloat(int sampleRate) {
            return {sampleRate, 2, SampleFormat::Float32, ResampleQuality::High, DownmixStrategy::Average};
        }
    };

} // namespace dsfw::audio
```

### 4.5 IAudioResampler — 重采样器接口

```cpp
namespace dsfw::audio {

    /// @brief Abstract audio resampler interface.
    class IAudioResampler {
    public:
        virtual ~IAudioResampler() = default;

        /// @brief Convert audio buffer to target format.
        /// @param input Source audio buffer.
        /// @param inputSampleRate Sample rate of the input.
        /// @param config Target configuration.
        /// @return Resampled buffer or error.
        virtual Result<AudioBuffer> convert(const AudioBuffer &input, int inputSampleRate,
                                            const ResampleConfig &config) = 0;

        /// @brief Convenience: resample to mono float32 at target rate.
        ///        Common pattern for HFA/RMVPE model input.
        Result<AudioBuffer> toMonoFloat(const AudioBuffer &input, int inputSampleRate,
                                        int targetSampleRate) {
            return convert(input, inputSampleRate,
                           ResampleConfig::forMonoFloat(targetSampleRate));
        }
    };

} // namespace dsfw::audio
```

### 4.6 AudioPipeline — 组合便捷层

```cpp
namespace dsfw::audio {

    /// @brief Combines decoder + resampler for common workflows.
    ///
    /// Usage:
    /// @code
    ///   auto pipeline = AudioPipeline::create();
    ///   auto result = pipeline.decodeAndResample("audio.mp3", ResampleConfig::forMonoFloat(16000));
    ///   if (result) {
    ///       auto &buffer = result.value();
    ///       // buffer is mono float32, 16000 Hz, ready for HFA model input
    ///       model->infer(buffer.floats(), buffer.frameCount());
    ///   }
    /// @endcode
    class AudioPipeline {
    public:
        /// @brief Create with default implementations.
        static AudioPipeline create();

        /// @brief Create with custom decoder and resampler (for testing).
        AudioPipeline(std::unique_ptr<IAudioDecoder> decoder,
                      std::unique_ptr<IAudioResampler> resampler);

        /// @brief Probe a file without decoding.
        Result<AudioFormatInfo> probe(const std::string &path);

        /// @brief Decode a file and resample to target format.
        ///        Streams through decoder→resampler to keep memory low.
        Result<AudioBuffer> decodeAndResample(const std::string &path,
                                              const ResampleConfig &config);

        /// @brief Decode a time segment and resample.
        Result<AudioBuffer> decodeSegmentAndResample(const std::string &path,
                                                     double startSec, double endSec,
                                                     const ResampleConfig &config);

        /// @brief Decode entire file to mono float32 at target rate.
        ///        Convenience for model inference pipelines.
        Result<AudioBuffer> decodeToMonoFloat(const std::string &path, int targetSampleRate = 16000);

        /// @brief Access the underlying decoder.
        [[nodiscard]] IAudioDecoder *decoder() const { return m_decoder.get(); }

        /// @brief Access the underlying resampler.
        [[nodiscard]] IAudioResampler *resampler() const { return m_resampler.get(); }

    private:
        std::unique_ptr<IAudioDecoder> m_decoder;
        std::unique_ptr<IAudioResampler> m_resampler;
    };

} // namespace dsfw::audio
```

---

## 5. 实现细节

### 5.1 FfmpegAudioDecoder (PIMPL)

```cpp
// 公共头文件: include/dsfw/audio/FfmpegAudioDecoder.h
#pragma once

#include <dsfw/audio/IAudioDecoder.h>
#include <memory>

namespace dsfw::audio {

    class FfmpegAudioDecoder : public IAudioDecoder {
    public:
        FfmpegAudioDecoder();
        ~FfmpegAudioDecoder() override;

        // Non-copyable, movable
        FfmpegAudioDecoder(const FfmpegAudioDecoder &) = delete;
        FfmpegAudioDecoder &operator=(const FfmpegAudioDecoder &) = delete;
        FfmpegAudioDecoder(FfmpegAudioDecoder &&) noexcept;
        FfmpegAudioDecoder &operator=(FfmpegAudioDecoder &&) noexcept;

        // IAudioDecoder impl
        Result<AudioFormatInfo> probe(const std::string &path) override;
        Result<void> open(const std::string &path) override;
        void close() override;
        bool isOpen() const override;
        const AudioFormatInfo &formatInfo() const override;
        Result<AudioBuffer> decodeAll() override;
        Result<AudioBuffer> decodeSegment(double startSec, double endSec) override;
        Result<AudioBuffer> read(int64_t frameCount) override;
        Result<void> seekToTime(double sec) override;
        double currentTime() const override;
        double totalDuration() const override;

    private:
        struct Impl;
        std::unique_ptr<Impl> d;
    };

} // namespace dsfw::audio
```

**Impl 内部使用**：
- `avformat_open_input()` / `avformat_find_stream_info()` — 解封装
- `avcodec_find_decoder()` / `avcodec_open2()` — 解码器
- `av_read_frame()` / `avcodec_send_packet()` / `avcodec_receive_frame()` — 分帧解码
- `av_seek_frame()` — 时间定位

**与现有 `AudioDecoder` 的关键区别**：
1. 输出**不内置重采样** — 解码器输出原始格式（源采样率、源声道、源格式）
2. `AudioBuffer` 代替裸 `std::vector<float>` — 携带格式元数据
3. `Result<T>` 代替 `bool` — 可追溯的错误信息
4. 支持 `probe()` 独立探测 — 不解码即可获取元数据
5. 支持 `decodeSegment()` — 按时间区间解码

### 5.2 SwresampleResampler (PIMPL)

```cpp
// 公共头文件: include/dsfw/audio/SwresampleResampler.h
#pragma once

#include <dsfw/audio/IAudioResampler.h>
#include <memory>

namespace dsfw::audio {

    class SwresampleResampler : public IAudioResampler {
    public:
        SwresampleResampler();
        ~SwresampleResampler() override;

        // Non-copyable, movable
        SwresampleResampler(const SwresampleResampler &) = delete;
        SwresampleResampler &operator=(const SwresampleResampler &) = delete;
        SwresampleResampler(SwresampleResampler &&) noexcept;
        SwresampleResampler &operator=(SwresampleResampler &&) noexcept;

        Result<AudioBuffer> convert(const AudioBuffer &input, int inputSampleRate,
                                    const ResampleConfig &config) override;

    private:
        struct Impl;
        std::unique_ptr<Impl> d;
    };

} // namespace dsfw::audio
```

**Impl 内部使用**：
- `swr_alloc_set_opts2()` — 配置重采样参数（通道布局、采样率、格式）
- `swr_init()` — 初始化
- `swr_convert()` — 执行重采样
- `swr_get_out_samples()` — 预估输出帧数

**通道映射策略**：
- 多声道→单声道：`DownmixStrategy::Average`（默认）— 等权平均
- 多声道→少声道：`DownmixStrategy::Drop` — 丢弃多余声道
- 少声道→多声道：复制 + 静音填充

**质量等级映射**：
| ResampleQuality | swresample 标志 | 说明 |
|-----------------|-----------------|------|
| Draft | `AV_SAMPLE_FMT_FLT` + 线性插值 | 最快 |
| Medium | `AV_SAMPLE_FMT_FLTP` + 中等滤波 | 默认 |
| High | `AV_SAMPLE_FMT_FLTP` + 高质量滤波 | 推荐 |
| Ultra | `AV_SAMPLE_FMT_S32P` + 最大滤波长度 | 最慢 |

### 5.3 AudioPipeline 实现要点

```cpp
Result<AudioBuffer> AudioPipeline::decodeAndResample(const std::string &path,
                                                     const ResampleConfig &config) {
    // 1. 探测格式
    auto infoResult = m_decoder->probe(path);
    if (!infoResult) return infoResult.error();
    auto &info = infoResult.value();

    // 2. 打开文件
    auto openResult = m_decoder->open(path);
    if (!openResult) return openResult.error();

    // 3. 流式解码 + 重采样
    //    分块读取避免大文件内存爆炸
    constexpr int64_t kChunkFrames = 4096;
    std::vector<AudioBuffer> chunks;

    while (true) {
        auto chunk = m_decoder->read(kChunkFrames);
        if (!chunk) return chunk.error();
        if (chunk->empty()) break;  // EOF

        auto resampled = m_resampler->convert(*chunk, info.sampleRate, config);
        if (!resampled) return resampled.error();
        chunks.push_back(std::move(*resampled));
    }

    m_decoder->close();

    // 4. 合并所有 chunk
    if (chunks.empty()) {
        return AudioBuffer::create(0, config.targetChannelCount, config.targetFormat);
    }
    return mergeBuffers(chunks);  // 辅助函数：拼接所有 AudioBuffer
}
```

---

## 6. 与现有代码的关系

### 6.1 迁移路径

```
阶段 A: 新建 dsfw-audio 库（独立，不修改现有代码）
    ├── A1: AudioBuffer + AudioFormatInfo + ResampleConfig 类型定义
    ├── A2: IAudioDecoder / IAudioResampler 接口
    ├── A3: FfmpegAudioDecoder 实现
    ├── A4: SwresampleResampler 实现
    ├── A5: AudioPipeline 组合层
    └── A6: 单元测试（AudioBuffer、解码器、重采样器、管道）

阶段 B: 应用层迁移（渐进式）
    ├── B1: WaveformRenderer 改用 AudioPipeline
    ├── B2: SlicerPage 改用 AudioPipeline（含自动混音→消除手动混音代码）
    ├── B3: SlicerService 改用 AudioPipeline
    ├── B4: RMVPE 推理改用 AudioPipeline（替代 resample_to_vio）
    ├── B5: AudioPlayback 适配层（桥接 IAudioDecoder → SDL2）

阶段 C: 清理旧代码
    ├── C1: 评估 dstools-audio 是否可完全替换
    ├── C2: 评估 audio-util 是否可删除（resample_to_vio 等）
    └── C3: 移除 libsndfile/soxr/mpg123 依赖（如不再需要）
```

### 6.2 与 dstools-audio 的关系

| 组件 | dstools-audio (现状) | dsfw-audio (新) | 迁移策略 |
|------|---------------------|-----------------|----------|
| AudioDecoder | FFmpeg 解码+重采样一体 | 拆分：FfmpegAudioDecoder + SwresampleResampler | 新库替代 |
| AudioPlayback | SDL2 回放 | 不在新库范围，保留适配层 | 适配层桥接 |
| AudioPlayer | 组合 Decoder+Playback | 不在新库范围 | 保留或适配 |
| IAudioPlayer | 播放器接口 | 不在新库范围 | 保留 |
| WaveFormat | 简单格式描述 | AudioFormatInfo 替代 | 新类型替代 |

### 6.3 与 audio-util 的关系

| 组件 | audio-util (现状) | dsfw-audio (新) | 迁移策略 |
|------|-------------------|-----------------|----------|
| resample_to_vio | soxr + libsndfile | AudioPipeline::decodeToMonoFloat | 新库替代 |
| write_vio_to_wav | libsndfile 写入 | 不在新库范围（可用 FFmpeg 实现） | 可选迁移 |
| readAudioToMonoFloat | libsndfile 读取 | AudioPipeline::decodeToMonoFloat | 新库替代 |
| SndfileHelper | libsndfile 封装 | 废弃 | 移除 |
| FlacDecoder | libflac | 废弃 | 移除 |
| Mp3Decoder | mpg123 | 废弃 | 移除 |

---

## 7. 单元测试策略

### 7.1 测试资源

- 合成测试音频：使用 Python 脚本生成已知内容的 WAV/FLAC/MP3/OGG 文件
- 存储于 `src/tests/audio/data/` 下
- 每个格式提供：440Hz 正弦波, 1s, 44100Hz, mono/stereo

### 7.2 测试用例

| 模块 | 测试类 | 关键用例 |
|------|--------|----------|
| AudioBuffer | TestAudioBuffer | 创建/克隆/切片/视图/格式转换 |
| FfmpegAudioDecoder | TestFfmpegDecoder | 探测 WAV/FLAC/MP3/OGG；全量解码；分段解码；流式读取；seek；错误路径 |
| SwresampleResampler | TestSwresampleResampler | 44100→16000；stereo→mono；mono→stereo；float32→int16；空输入；质量对比 |
| AudioPipeline | TestAudioPipeline | 端到端解码+重采样；MonoFloat 便捷方法；大文件流式 |

### 7.3 测试原则

遵循 INFRA-17（测试不依赖外部资源）：使用合成测试文件，不依赖真实音频文件。

---

## 8. 与现有重构方案 (v4.3) 的集成

### 8.1 建议插入位置

当前重构方案 v4.3 的阶段如下：

```
R1 → R2 → {R3, R4, R6, R8, R11, R12} → R10 → R9 → R7 → R14 → R15
```

建议新增阶段 **R16: dsfw-audio 独立音频库**，插入位置：

```
R1 → R2 → {R3, R4, R6, R8, R11, R12} → R10 → R9 → R7 → R16 → R14 → R15
```

**插入理由**：
- R16 在 R14（缩放精度自适应）之前：R14 需要高质量音频数据，R16 提供的 `AudioBuffer` 统一了数据格式
- R16 在 R15（渲染管线性能优化）之前：R15 的缓存策略可以使用 `AudioBuffer` 的零拷贝特性
- R16 在 R11（audio-util PathCompat 清理）之后：R11 先完成 audio-util 的路径调整，R16 再整体替换 audio-util
- R16 不阻塞 R1~R12 的任何阶段：可独立开发

### 8.2 细化子阶段

| 子阶段 | 名称 | 工作量 | 风险 | 依赖 |
|--------|------|--------|------|------|
| R16.1 | AudioBuffer + 类型定义 | 2d | 低 | 无 |
| R16.2 | IAudioDecoder / IAudioResampler 接口 | 1d | 低 | R16.1 |
| R16.3 | FfmpegAudioDecoder (PIMPL) | 3d | 中 | R16.2 |
| R16.4 | SwresampleResampler (PIMPL) | 2d | 中 | R16.2 |
| R16.5 | AudioPipeline 组合层 | 1d | 低 | R16.3, R16.4 |
| R16.6 | 单元测试 | 2d | 低 | R16.5 |
| R16.7 | 应用层迁移（WaveformRenderer） | 1d | 低 | R16.5 |
| R16.8 | 应用层迁移（SlicerPage/SlicerService） | 1d | 中 | R16.5 |
| R16.9 | 推理层迁移（RMVPE, audio-util 替换） | 1d | 中 | R16.5 |
| R16.10 | 旧代码清理 | 1d | 中 | R16.7~R16.9 |

**总工作量估算**：约 15 人天

### 8.3 风险与缓解

| 风险 | 严重度 | 缓解措施 |
|------|--------|----------|
| FFmpeg API 版本差异 | 中 | 使用 vcpkg 固定版本；PIMPL 隔离 |
| 重采样质量与 soxr 不一致 | 中 | 对比测试；保留 High/Ultra 质量等级 |
| 大文件流式解码内存泄漏 | 中 | 单元测试覆盖大文件；valgrind/ASan 检查 |
| 现有应用层代码迁移回归 | 高 | 逐模块迁移，每步验证；保留旧代码直到新代码稳定 |

---

## 9. 附录

### 9A. 与参考项目的对比

| 特性 | 本项目 (dsfw-audio) | miniaudio | libnyquist | JUCE Audio |
|------|---------------------|-----------|------------|------------|
| 后端 | FFmpeg | 自包含 | libsndfile+mpg123+libflac+vorbis+opus | 自包含 |
| 格式支持 | 全（FFmpeg 支持的所有） | WAV/MP3/FLAC | WAV/MP3/FLAC/OGG/Opus | WAV/AIFF/FLAC/OGG/MP3 |
| 重采样 | swresample (高质量) | 内置线性 | 无 | 内置多种算法 |
| 依赖 | FFmpeg 4个库 | 无 | 多个库 | 无 |
| 许可证 | MIT | MIT/PublicDomain | MIT | GPL/Commercial |
| 内存容器 | AudioBuffer | 裸数组 | 裸数组 | AudioBuffer |
| 流式 | 支持 | 支持 | 部分支持 | 支持 |

### 9B. 设计决策记录

| 决策 | 选择 | 备选 | 理由 |
|------|------|------|------|
| 后端统一 | FFmpeg (avformat+avcodec+swresample) | libsndfile+soxr+多个解码器 | 项目已有 FFmpeg；统一后端减少依赖；swresample 质量高 |
| 接口设计 | IAudioDecoder + IAudioResampler 分离 | 单一 IAudiodecoder 含重采样 | 职责分离（ARCH-01）；开闭原则（ARCH-07）；可独立测试 |
| AudioBuffer | 值类型 + view/owned 模式 | 纯 shared_ptr 管理 | 零拷贝切片；AI 模型可直接使用指针；符合 INFRA-06 |
| 命名空间 | `dsfw::audio` | `dstools::audio` | 框架层（dsfw），不依赖应用层；可独立复用 |
| Qt 依赖 | 核心库无 Qt | 依赖 Qt::Core | 最大化复用性；播放适配层可选用 Qt |
| PIMPL | 所有 FFmpeg 实现类 PIMPL | 头文件暴露 FFmpeg 类型 | INFRA-13 要求；避免编译期泄漏 |
| 质量等级 | 4 级 (Draft/Medium/High/Ultra) | 固定质量 | 不同场景不同需求：HFA 推理用 High，实时预览用 Medium |