#include "WaveformChartPanel.h"

#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>

#include <algorithm>
#include <cmath>

namespace dstools {
namespace phonemelabeler {

WaveformChartPanel::WaveformChartPanel(ViewportController *viewport, QWidget *parent)
    : ChartPanelBase(QStringLiteral("waveform"), viewport, parent)
{
    setMinimumHeight(150);
}

void WaveformChartPanel::setAudioData(const std::vector<float> &samples, int sampleRate) {
    m_samples = samples;
    m_sampleRate = sampleRate;
    m_minMaxCache.clear();
    m_cacheDirty = true;
    update();
}

void WaveformChartPanel::setPlayhead(double sec) {
    PlayheadState state;
    state.positionSec = sec;
    onPlayheadUpdate(state);
}

void WaveformChartPanel::drawContent(QPainter &painter) {
    if (m_samples.empty()) {
        painter.setPen(Qt::gray);
        painter.drawText(rect(), Qt::AlignCenter, tr("No audio loaded"));
        return;
    }

    int w = width();
    int h = height();
    int centerY = h / 2;

    painter.setPen(QPen(QColor(60, 60, 60), 1));
    painter.drawLine(0, centerY, w, centerY);

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

    drawDbAxis(painter);
}

void WaveformChartPanel::onVerticalZoom(double factor) {
    m_amplitudeScale = std::clamp(m_amplitudeScale * factor, 0.1, 20.0);
    update();
}

void WaveformChartPanel::rebuildCache() {
    rebuildMinMaxCache();
}

void WaveformChartPanel::resizeEvent(QResizeEvent *event) {
    ChartPanelBase::resizeEvent(event);
}

void WaveformChartPanel::drawDbAxis(QPainter &painter) {
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

void WaveformChartPanel::rebuildMinMaxCache() {
    double viewDuration = m_xf.viewEnd - m_xf.viewStart;
    if (m_samples.empty() || viewDuration <= 0.0) {
        m_minMaxCache.clear();
        return;
    }

    int numPixels = qMax(1, width());
    m_minMaxCache.resize(numPixels);
    m_cacheViewDuration = viewDuration;

    for (int x = 0; x < numPixels; ++x) {
        double tStart = m_xf.viewStart + viewDuration * x / numPixels;
        double tEnd = m_xf.viewStart + viewDuration * (x + 1) / numPixels;

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