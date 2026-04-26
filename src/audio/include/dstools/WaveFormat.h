#pragma once
#include <cstdint>

namespace dstools::audio {

/// Audio format description. Preserves original qsmedia WaveFormat semantics.
class WaveFormat {
public:
    WaveFormat();
    WaveFormat(int sampleRate, int bitsPerSample, int channels);

    int sampleRate() const;
    int bitsPerSample() const;
    int channels() const;
    int blockAlign() const;      // channels * bitsPerSample / 8
    int averageBytesPerSecond() const;

private:
    int m_sampleRate = 44100;
    int m_bitsPerSample = 16;
    int m_channels = 2;
};

} // namespace dstools::audio
