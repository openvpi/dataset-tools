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
namespace chart {

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
    m_fullDataDirty = true;
    update();
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

} // namespace chart
} // namespace dstools