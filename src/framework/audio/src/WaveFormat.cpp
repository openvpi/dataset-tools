#include <dstools/WaveFormat.h>

namespace dstools::audio {

WaveFormat::WaveFormat() = default;

WaveFormat::WaveFormat(int sampleRate, int bitsPerSample, int channels)
    : m_sampleRate(sampleRate), m_bitsPerSample(bitsPerSample), m_channels(channels) {
}

int WaveFormat::sampleRate() const { return m_sampleRate; }
int WaveFormat::bitsPerSample() const { return m_bitsPerSample; }
int WaveFormat::channels() const { return m_channels; }
int WaveFormat::blockAlign() const { return m_channels * (m_bitsPerSample / 8); }
int WaveFormat::averageBytesPerSecond() const { return m_sampleRate * blockAlign(); }

} // namespace dstools::audio
