#include "WaveformRenderer.h"

#include <dstools/AudioDecoder.h>
#include <dstools/WaveFormat.h>

namespace dstools {
namespace phonemelabeler {

namespace {
    constexpr int kDefaultBufferSize = 4096;
} // namespace

WaveformRenderer::WaveformRenderer(QObject *parent)
    : QObject(parent) {
}

bool WaveformRenderer::loadAudio(const QString &path) {
    m_samples.clear();
    m_sampleRate = 0;

    dstools::audio::AudioDecoder decoder;
    if (!decoder.open(path)) {
        emit error(tr("Failed to open audio file: %1").arg(path));
        emit audioLoaded(false);
        return false;
    }

    auto fmt = decoder.format();
    m_sampleRate = fmt.sampleRate();
    int channels = fmt.channels();

    std::vector<float> allSamples;
    const int bufferSize = kDefaultBufferSize;
    std::vector<float> buffer(bufferSize);
    while (true) {
        int read = decoder.read(buffer.data(), 0, bufferSize);
        if (read <= 0) break;
        allSamples.insert(allSamples.end(), buffer.begin(), buffer.begin() + read);
    }
    decoder.close();

    // Convert to mono
    if (channels > 1) {
        m_samples.resize(allSamples.size() / channels);
        for (size_t i = 0; i < m_samples.size(); ++i) {
            float sum = 0.0f;
            for (int c = 0; c < channels; ++c) {
                sum += allSamples[i * channels + c];
            }
            m_samples[i] = sum / channels;
        }
    } else {
        m_samples = std::move(allSamples);
    }

    emit audioLoaded(true);
    return true;
}

} // namespace phonemelabeler
} // namespace dstools