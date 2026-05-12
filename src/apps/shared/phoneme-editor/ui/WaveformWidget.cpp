#include "WaveformWidget.h"
#include "IBoundaryModel.h"
#include "BoundaryDragController.h"
#include "commands/BoundaryCommands.h"
#include <dsfw/Log.h>

#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QDebug>

#include <cmath>
#include <algorithm>

#include <dstools/AudioDecoder.h>
#include <dstools/WaveFormat.h>

namespace dstools {
namespace phonemelabeler {

namespace {
    constexpr int kDefaultBufferSize = 4096;
}

WaveformWidget::WaveformWidget(ViewportController *viewport, QWidget *parent)
    : AudioChartWidget(viewport, parent) {
    setMinimumHeight(150);
}

WaveformWidget::~WaveformWidget() = default;

void WaveformWidget::setAudioData(const std::vector<float> &samples, int sampleRate) {
    m_samples = samples;
    m_sampleRate = sampleRate;
    rebuildCache();
    update();
}

void WaveformWidget::loadAudio(const QString &path) {
    dstools::audio::AudioDecoder decoder;
    if (!decoder.open(path)) {
        DSFW_LOG_WARN("audio", ("Failed to open audio file: " + path.toStdString()).c_str());
        return;
    }

    auto fmt = decoder.format();
    m_sampleRate = fmt.sampleRate();

    std::vector<float> allSamples;
    const int bufferSize = kDefaultBufferSize;
    std::vector<float> buffer(bufferSize);

    while (true) {
        int read = decoder.read(buffer.data(), 0, bufferSize);
        if (read <= 0) break;
        allSamples.insert(allSamples.end(), buffer.begin(), buffer.begin() + read);
    }
    decoder.close();

    int channels = fmt.channels();
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

    rebuildCache();
    update();
}

void WaveformWidget::setPlayhead(double sec) {
    m_playhead = sec;
    update();
}

void WaveformWidget::onPlayheadChanged(double sec) {
    setPlayhead(sec);
}

void WaveformWidget::rebuildCache() {
    rebuildMinMaxCache();
}

void WaveformWidget::onViewportDragStart(double timeSec) {
    emit positionClicked(timeSec);
}

void WaveformWidget::onVerticalZoom(double factor) {
    m_amplitudeScale = std::clamp(m_amplitudeScale * factor, 0.1, 20.0);
    update();
}

void WaveformWidget::paintEvent(QPaintEvent * /*event*/) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor(30, 30, 30));

    if (m_samples.empty()) {
        painter.setPen(Qt::gray);
        painter.drawText(rect(), Qt::AlignCenter, tr("No audio loaded"));
        return;
    }

    drawWaveform(painter);
    drawDbAxis(painter);
    drawPlayCursor(painter);
}

void WaveformWidget::resizeEvent(QResizeEvent *event) {
    rebuildMinMaxCache();
    AudioChartWidget::resizeEvent(event);
}

void WaveformWidget::drawWaveform(QPainter &painter) {
    if (m_samples.empty()) return;

    int w = width();
    int h = height();
    int centerY = h / 2;

    painter.setPen(QPen(QColor(60, 60, 60), 1));
    painter.drawLine(0, centerY, w, centerY);

    if (m_minMaxCache.empty()) {
        rebuildMinMaxCache();
    }

    painter.setPen(QPen(QColor(100, 180, 255), 1));

    int numPixels = static_cast<int>(m_minMaxCache.size());
    for (int x = 0; x < w && x < numPixels; ++x) {
        const auto &mm = m_minMaxCache[x];
        float scaledMax = std::clamp(mm.max * static_cast<float>(m_amplitudeScale), -1.0f, 1.0f);
        float scaledMin = std::clamp(mm.min * static_cast<float>(m_amplitudeScale), -1.0f, 1.0f);
        int yMin = centerY - static_cast<int>(scaledMax * centerY);
        int yMax = centerY - static_cast<int>(scaledMin * centerY);
        if (yMin > yMax) std::swap(yMin, yMax);
        painter.drawLine(x, yMin, x, yMax);
    }
}

void WaveformWidget::drawPlayCursor(QPainter &painter) {
    if (m_playhead < 0.0) return;

    int x = timeToX(m_playhead);
    if (x < 0 || x > width()) return;

    painter.setPen(QPen(QColor(255, 100, 100), 2));
    painter.drawLine(x, 0, x, height());
}

void WaveformWidget::drawDbAxis(QPainter &painter) {
    int h = height();
    int centerY = h / 2;

    QFont font = painter.font();
    font.setPixelSize(9);
    painter.setFont(font);
    painter.setPen(QColor(100, 100, 120));

    static const float dbValues[] = {0.0f, -6.0f, -12.0f, -24.0f};

    for (float db : dbValues) {
        float linear = std::pow(10.0f, db / 20.0f);
        float scaled = linear * static_cast<float>(m_amplitudeScale);
        if (scaled > 1.0f) continue;

        int yTop = centerY - static_cast<int>(scaled * centerY);

        painter.setPen(QColor(100, 100, 120));
        QString label = (db == 0.0f) ? QStringLiteral("0dB") : QString::number(static_cast<int>(db)) + "dB";
        painter.drawText(2, yTop - 1, label);
    }
}

void WaveformWidget::rebuildMinMaxCache() {
    if (m_samples.empty() || m_viewEnd <= m_viewStart) {
        m_minMaxCache.clear();
        return;
    }

    int numPixels = qMax(1, width());
    m_minMaxCache.resize(numPixels);
    m_cacheResolution = (m_viewEnd - m_viewStart);

    for (int x = 0; x < numPixels; ++x) {
        double tStart = m_viewStart + (m_viewEnd - m_viewStart) * x / numPixels;
        double tEnd = m_viewStart + (m_viewEnd - m_viewStart) * (x + 1) / numPixels;

        int sStart = std::max(0, static_cast<int>(tStart * m_sampleRate));
        int sEnd = std::min(static_cast<int>(m_samples.size()),
                            static_cast<int>(tEnd * m_sampleRate));

        if (sStart >= sEnd || sStart >= static_cast<int>(m_samples.size())) {
            m_minMaxCache[x] = {0.0f, 0.0f};
            continue;
        }

        float mn = m_samples[sStart];
        float mx = m_samples[sStart];
        for (int s = sStart + 1; s < sEnd; ++s) {
            mn = std::min(mn, m_samples[s]);
            mx = std::max(mx, m_samples[s]);
        }

        m_minMaxCache[x] = {mn, mx};
    }
}

} // namespace phonemelabeler
} // namespace dstools
