#pragma once
/// @file AudioBuffer.h
/// @brief Unified audio buffer container with view/owned semantics.

#include <cstdint>
#include <span>
#include <variant>
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
    ///
    /// Supports two storage modes:
    /// - Owned: buffer owns the data via std::vector<uint8_t> (mutable access)
    /// - View: non-owning reference to external data via std::span<const uint8_t> (read-only, zero-copy)
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

        /// @brief Create a non-owning view over external data (zero-copy).
        ///        Caller must ensure data lifetime exceeds buffer usage.
        ///        View buffers are read-only; mutation methods will assert.
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
        [[nodiscard]] size_t byteSize() const;
        [[nodiscard]] bool isView() const;
        [[nodiscard]] bool empty() const { return byteSize() == 0; }

        /// @brief Duration in seconds at given sample rate.
        [[nodiscard]] double durationSec(int sampleRate) const;

        // -- Data access (const) --

        [[nodiscard]] const uint8_t *rawData() const;

        /// @brief Get float samples (asserts Float32 format).
        [[nodiscard]] std::span<const float> floats() const;

        /// @brief Get int16 samples (asserts Int16 format).
        [[nodiscard]] std::span<const int16_t> int16s() const;

        /// @brief Access a single channel as float.
        [[nodiscard]] float sampleAt(int64_t frame, int channel) const;

        // -- Data access (mutable, owned buffers only) --

        [[nodiscard]] uint8_t *rawData();

        /// @brief Get mutable float samples (asserts Float32 format, owned mode).
        [[nodiscard]] std::span<float> floats();

        /// @brief Get mutable int16 samples (asserts Int16 format, owned mode).
        [[nodiscard]] std::span<int16_t> int16s();

        // -- Utility --

        /// @brief Zero all samples (owned buffers only).
        void zero();

        /// @brief Create a deep copy (always owned).
        [[nodiscard]] AudioBuffer clone() const;

        /// @brief Slice a range of frames (zero-copy sub-span if view, copy if owned).
        [[nodiscard]] AudioBuffer slice(int64_t startFrame, int64_t frameCount) const;

    private:
        AudioBuffer() = default;

        using DataStorage = std::variant<std::vector<uint8_t>, std::span<const uint8_t>>;

        [[nodiscard]] const uint8_t *dataPtr() const;
        [[nodiscard]] uint8_t *dataPtrMutable();

        DataStorage m_data;
        int64_t m_frameCount = 0;
        int m_channelCount = 0;
        SampleFormat m_format = SampleFormat::Unknown;
    };

} // namespace dsfw::audio