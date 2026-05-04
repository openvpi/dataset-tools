#pragma once
/// @file WaveFormat.h
/// @brief Audio format descriptor (sample rate, bit depth, channels).

#include <cstdint>

namespace dstools::audio {

/// @brief Audio format description preserving original qsmedia WaveFormat semantics.
class WaveFormat {
public:
    /// @brief Construct a default format (44100 Hz, 16-bit, stereo).
    WaveFormat() = default;

    /// @brief Construct a format with specific parameters.
    /// @param sampleRate Sample rate in Hz.
    /// @param bitsPerSample Bit depth per sample.
    /// @param channels Number of audio channels.
    WaveFormat(int sampleRate, int bitsPerSample, int channels)
        : m_sampleRate(sampleRate), m_bitsPerSample(bitsPerSample), m_channels(channels) {
    }

    /// @brief Get the sample rate in Hz.
    int sampleRate() const { return m_sampleRate; }

    /// @brief Get the bit depth per sample.
    int bitsPerSample() const { return m_bitsPerSample; }

    /// @brief Get the number of channels.
    int channels() const { return m_channels; }

    /// @brief Get the block alignment (channels * bitsPerSample / 8).
    int blockAlign() const { return m_channels * (m_bitsPerSample / 8); }

    /// @brief Get the average bytes per second.
    int averageBytesPerSecond() const { return m_sampleRate * blockAlign(); }

private:
    int m_sampleRate = 44100;
    int m_bitsPerSample = 16;
    int m_channels = 2;
};

} // namespace dstools::audio
