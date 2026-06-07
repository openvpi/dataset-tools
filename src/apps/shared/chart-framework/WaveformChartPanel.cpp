#include "WaveformChartPanel.h"
#include "ChartConfigRegistry.h"

#include <QPainter>
#include <QPen>
#include <QPainterPath>
#include <QFont>
#include <dsfw/Theme.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace dstools {

using namespace dsfw;

using namespace dsfw;

WaveformChartPanel::WaveformChartPanel(ViewportController *viewport, QWidget *parent)
    : ChartPanelBase(QStringLiteral("waveform"), viewport, parent)
{
    setMinimumHeight(64);
    loadConfigParams();
}

void WaveformChartPanel::registerChartConfig() {
    static bool registered = false;
    if (registered) return;
    registered = true;

    ChartConfigDescriptor desc;
    desc.chartId = QStringLiteral("waveform");
    desc.displayName = QStringLiteral("波形图");
    desc.params = {
        {QStringLiteral("loudnessRef"), QStringLiteral("响度参考值"),
         ParamType::Double, QVariant(0.5), QVariant(0.01), QVariant(2.0),
         QStringLiteral(""), 2,
         QStringLiteral("响度归一化参考值。RMS / 此值做clamp，影响波形颜色映射灵敏度。")},
        {QStringLiteral("opacity"), QStringLiteral("填充透明度"),
         ParamType::Double, QVariant(0.16), QVariant(0.0), QVariant(1.0),
         QStringLiteral(""), 2,
         QStringLiteral("波形填充区域透明度。0 为完全透明，1 为完全不透明。")},
    };
    ChartConfigRegistry::instance().registerChart(desc);
}

void WaveformChartPanel::loadConfigParams() {
    auto &reg = ChartConfigRegistry::instance();
    m_loudnessRef = static_cast<float>(reg.value(QStringLiteral("waveform"), QStringLiteral("loudnessRef")).toDouble());
    m_opacity = reg.value(QStringLiteral("waveform"), QStringLiteral("opacity")).toDouble();
}

void WaveformChartPanel::setColor(const QColor &color) {
    m_waveColor = color;
    update();
}

void WaveformChartPanel::onAudioDataChanged() {
    m_yMax.clear();
    m_yMin.clear();
    m_rms.clear();
    m_loudnessMask.clear();
}

void WaveformChartPanel::rebuildCache(const RegionUpdate &region) {
    if (region.fullRebuild) {
        rebuildWaveformCache();
    } else {
        rebuildWaveformCachePartial(region);
    }
}

void WaveformChartPanel::rebuildWaveformCache() {
    int w = width();
    int yaw = yAxisWidth();
    int dataW = w - yaw;
    if (dataW <= 0 || m_samples.empty() || !m_converter) {
        m_yMax.clear();
        m_yMin.clear();
        m_rms.clear();
        m_loudnessMask.clear();
        return;
    }

    m_yMax.resize(dataW);
    m_yMin.resize(dataW);
    m_rms.resize(dataW);
    m_loudnessMask.resize(dataW);

    double viewStart = m_converter->startSec();
    double viewEnd = m_converter->endSec();

    if (viewEnd <= viewStart) {
        std::fill(m_yMax.begin(), m_yMax.end(), 0.0f);
        std::fill(m_yMin.begin(), m_yMin.end(), 0.0f);
        std::fill(m_rms.begin(), m_rms.end(), 0.0f);
        std::fill(m_loudnessMask.begin(), m_loudnessMask.end(), 0.0f);
        return;
    }

    for (int col = 0; col < dataW; ++col) {
        double tStart = viewStart + (viewEnd - viewStart) * col / dataW;
        double tEnd = viewStart + (viewEnd - viewStart) * (col + 1) / dataW;
        int sStart = static_cast<int>(tStart * m_sampleRate);
        int sEnd = static_cast<int>(tEnd * m_sampleRate);
        sStart = std::max(0, sStart);
        sEnd = std::min(static_cast<int>(m_samples.size()), sEnd);

        float yMax = 0.0f;
        float yMin = 0.0f;
        double sumSq = 0.0;
        int count = 0;

        for (int s = sStart; s < sEnd; ++s) {
            double val = m_samples[s];
            yMax = std::max(yMax, static_cast<float>(val));
            yMin = std::min(yMin, static_cast<float>(val));
            sumSq += val * val;
            ++count;
        }

        m_yMax[col] = yMax;
        m_yMin[col] = yMin;
        m_rms[col] = count > 0 ? static_cast<float>(std::sqrt(sumSq / count)) : 0.0f;

        float rms = m_rms[col];
        float loudness = std::clamp(rms / m_loudnessRef, 0.0f, 1.0f);
        m_loudnessMask[col] = loudness;
    }
}

void WaveformChartPanel::rebuildWaveformCachePartial(const RegionUpdate &region) {
    if (m_samples.empty() || !m_converter || m_converter->endSec() <= m_converter->startSec() || m_yMax.empty())
        return;

    int w = width();
    int yaw = yAxisWidth();
    int dataW = w - yaw;
    if (dataW <= 0) return;

    shiftCache(m_yMax, dataW, region.colShift);
    shiftCache(m_yMin, dataW, region.colShift);
    shiftCache(m_rms, dataW, region.colShift);
    shiftCache(m_loudnessMask, dataW, region.colShift);

    int dirtyStart = region.dirtyStartCol - yaw;
    int dirtyEnd = region.dirtyEndCol - yaw;
    if (dirtyStart < 0) dirtyStart = 0;
    if (dirtyEnd > dataW) dirtyEnd = dataW;

    double viewStart = m_converter->startSec();
    double viewEnd = m_converter->endSec();

    for (int col = dirtyStart; col < dirtyEnd; ++col) {
        double tStart = viewStart + (viewEnd - viewStart) * col / dataW;
        double tEnd = viewStart + (viewEnd - viewStart) * (col + 1) / dataW;
        int sStart = static_cast<int>(tStart * m_sampleRate);
        int sEnd = static_cast<int>(tEnd * m_sampleRate);
        sStart = std::max(0, sStart);
        sEnd = std::min(static_cast<int>(m_samples.size()), sEnd);

        float yMax = 0.0f;
        float yMin = 0.0f;
        double sumSq = 0.0;
        int count = 0;

        for (int s = sStart; s < sEnd; ++s) {
            double val = m_samples[s];
            yMax = std::max(yMax, static_cast<float>(val));
            yMin = std::min(yMin, static_cast<float>(val));
            sumSq += val * val;
            ++count;
        }

        m_yMax[col] = yMax;
        m_yMin[col] = yMin;
        m_rms[col] = count > 0 ? static_cast<float>(std::sqrt(sumSq / count)) : 0.0f;

        float rms = m_rms[col];
        float loudness = std::clamp(rms / m_loudnessRef, 0.0f, 1.0f);
        m_loudnessMask[col] = loudness;
    }
}

void WaveformChartPanel::drawContent(QPainter &painter, const ChartCoordinate &coord) {
    Q_UNUSED(coord)
    if (m_samples.empty()) {
        drawEmptyState(painter, tr("No audio loaded"));
        return;
    }
    drawWaveform(painter);
}

void WaveformChartPanel::drawWaveform(QPainter &painter) {
    if (m_yMax.empty()) return;

    int yaw = yAxisWidth();
    int h = height();
    QRect chartRect = chartContentRect();
    int dataW = chartRect.width();

    int h2 = h / 2;

    painter.save();
    painter.setClipRect(chartRect);

    QColor fillColor = m_waveColor;
    fillColor.setAlpha(static_cast<int>(m_opacity * 255));

    const float minVal = -1.0f;
    const float maxVal = 1.0f;
    const float rangeInv = 1.0f / (maxVal - minVal);

    QPainterPath fillPath;
    fillPath.moveTo(static_cast<float>(yaw), static_cast<float>(h2));

    for (int x = 0; x < dataW; ++x) {
        if (x >= static_cast<int>(m_yMax.size())) break;
        float xf = static_cast<float>(yaw + x);
        float yTop = h2 - (m_yMax[x] - minVal) * rangeInv * h2;
        fillPath.lineTo(xf, yTop);
    }

    for (int x = dataW - 1; x >= 0; --x) {
        if (x >= static_cast<int>(m_yMin.size())) continue;
        float xf = static_cast<float>(yaw + x);
        float yBot = h2 - (m_yMin[x] - minVal) * rangeInv * h2;
        fillPath.lineTo(xf, yBot);
    }

    fillPath.closeSubpath();
    painter.setPen(Qt::NoPen);
    painter.setBrush(fillColor);
    painter.drawPath(fillPath);

    painter.setBrush(Qt::NoBrush);
    QPen wavePen(m_waveColor, 0.5);
    painter.setPen(wavePen);

    for (int x = 0; x < dataW; ++x) {
        if (x >= static_cast<int>(m_yMax.size())) break;
        float xf = static_cast<float>(yaw + x);
        float yTop = h2 - (m_yMax[x] - minVal) * rangeInv * h2;
        float yBot = h2 - (m_yMin[x] - minVal) * rangeInv * h2;
        painter.drawLine(QPointF(xf, yTop), QPointF(xf, yBot));
    }

    painter.restore();
}

void WaveformChartPanel::paintYAxisContent(QPainter &painter, const QRect &chartRect) {
    int labelW = chartRect.width();
    int h = chartRect.height();

    painter.save();
    painter.setPen(QColor(140, 140, 140));
    QFont font(QStringLiteral("Arial"), 8);
    painter.setFont(font);

    const std::array levels = {1.0, 0.5, 0.0, -0.5, -1.0};
    for (double lv : levels) {
        double ratio = (lv - (-1.0)) / (1.0 - (-1.0));
        int y = static_cast<int>(h - ratio * h);
        QString label = QString::number(lv, 'f', 1);
        int clampedY = std::clamp(y, 8, h - 8);
        painter.drawText(QRect(0, clampedY - 8, labelW - 4, 16),
                         Qt::AlignRight | Qt::AlignVCenter, label);
    }
    painter.restore();
}

void WaveformChartPanel::renderFullData(QImage &image) {
    image.fill(Qt::transparent);
    if (m_samples.empty() || m_sampleRate <= 0) return;

    int imgW = image.width();
    int imgH = image.height();
    if (imgW <= 0 || imgH <= 0) return;

    double totalDur = dataDurationSec();
    if (totalDur <= 0.0) return;

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, false);

    int h2 = imgH / 2;
    const float minVal = -1.0f;
    const float maxVal = 1.0f;
    const float rangeInv = 1.0f / (maxVal - minVal);

    double samplesPerCol = static_cast<double>(m_samples.size()) / imgW;

    // Render complete waveform: for each column, find min/max across all samples in that column
    QPen wavePen(m_waveColor, 0.5);
    painter.setPen(wavePen);

    for (int col = 0; col < imgW; ++col) {
        int sStart = static_cast<int>(col * samplesPerCol);
        int sEnd = static_cast<int>((col + 1) * samplesPerCol);
        sStart = std::max(0, sStart);
        sEnd = std::min(static_cast<int>(m_samples.size()), sEnd);

        float yMax = 0.0f;
        float yMin = 0.0f;
        for (int s = sStart; s < sEnd; ++s) {
            float val = m_samples[s];
            yMax = std::max(yMax, val);
            yMin = std::min(yMin, val);
        }

        float yTop = h2 - (yMax - minVal) * rangeInv * h2;
        float yBot = h2 - (yMin - minVal) * rangeInv * h2;

        painter.drawLine(col, static_cast<int>(yTop), col, static_cast<int>(yBot));
    }
}

double WaveformChartPanel::dataDurationSec() const {
    if (m_samples.empty() || m_sampleRate <= 0) return 0.0;
    // m_samples is stereo interleaved
    return static_cast<double>(m_samples.size()) / (m_sampleRate * 2.0);
}

int WaveformChartPanel::optimalRenderWidth() const {
    if (m_samples.empty() || m_sampleRate <= 0) return width() * 4;

    int widgetWidth = width();
    if (widgetWidth <= 0) return 4;

    // Use visible duration from the converter for adaptive precision
    double visibleDuration = 0.0;
    if (m_converter) {
        visibleDuration = m_converter->endSec() - m_converter->startSec();
    }
    if (visibleDuration <= 0) return widgetWidth * 4;

    // At least 0.5 pixels per sample at the visible zoom level to avoid aliasing
    static constexpr double kMinPixelsPerSample = 0.5;
    int minWidth = static_cast<int>(visibleDuration * m_sampleRate * kMinPixelsPerSample);
    return std::max(widgetWidth * 4, minWidth);
}

} // namespace dstools
