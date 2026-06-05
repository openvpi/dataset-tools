#include "WaveformRenderer.h"

#include <dsfw/audio/AudioPipeline.h>

namespace dstools {
namespace phonemelabeler {

WaveformRenderer::WaveformRenderer(QObject* parent) : QObject(parent) {
}

bool WaveformRenderer::loadAudio(const QString& path) {
    m_samples.clear();
    m_sampleRate = 0;

    auto pipeline = dsfw::audio::AudioPipeline::create();
    auto probeResult = pipeline.probe(path.toStdString());
    if (!probeResult.ok()) {
        emit error(tr("Failed to open audio file: %1").arg(path));
        emit audioLoaded(false);
        return false;
    }
    int sr = probeResult.value().sampleRate;

    auto decodeResult = pipeline.decodeToMonoFloat(path.toStdString(), sr);
    if (!decodeResult.ok()) {
        emit error(tr("Failed to decode audio file: %1").arg(path));
        emit audioLoaded(false);
        return false;
    }

    auto buffer = decodeResult.value();
    m_sampleRate = sr;
    auto floats = buffer.floats();
    m_samples.assign(floats.begin(), floats.end());

    emit audioLoaded(true);
    return true;
}

} // namespace phonemelabeler
} // namespace dstools
