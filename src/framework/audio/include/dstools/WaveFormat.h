#pragma once
/// @file WaveFormat.h
/// @brief Audio format descriptor (sample rate, bit depth, channels).

#include <cstdint>

namespace dstools::audio {

/// @brief Audio format description preserving original qsmedia WaveFormat semantics.
class WaveFormat {
public:
    /// @brief Construct a default format (44100 Hz, 16-bit, stereo).
    WaveFormat();

    /// @brief Construct a format with specific parameters.
    /// @param sampleRate Sample rate in Hz.
    /// @param bitsPerSample Bit depth per sample.
    /// @param channels Number of audio channels.
    WaveFormat(int sampleRate, int bitsPerSample, int channels);

    /// @brief Get the sample rate in Hz.
    /// @return Sample rate.
    int sampleRate() const;

    /// @brief Get the bit depth per sample.
    /// @return Bits per sample.
    int bitsPerSample() const;

    /// @brief Get the number of channels.
    /// @return Channel count.
    int channels() const;

    /// @brief Get the block alignment (channels * bitsPerSample / 8).
    /// @return Block align in bytes.
    int blockAlign() const;

    /// @brief Get the average bytes per second.
    /// @return Bytes per second.
    int averageBytesPerSecond() const;

private:
    int m_sampleRate = 44100;
    int m_bitsPerSample = 16;
    int m_channels = 2;
};

} // namespace dstools::audio
