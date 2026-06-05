#pragma once
/// @file AudioBuffer.h
/// @brief Unified audio buffer container with view/owned semantics.

#include <cstdint>
#include <span>
#include <vector>

namespace dsfw::audio {

    /// @brief Sample format enumeration.
    enum class SampleFormat : uint8_t {
        Unknown = 0,
        Float32, ///< 32-bit IEEE float [-1.0, 1.0]
        Int16,   ///< 16-bit signed integer
        Int32,   ///< 32-bit signed integer
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

        /// @brief Access a single channel as float.
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