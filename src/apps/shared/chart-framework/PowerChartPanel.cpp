#include "PowerChartPanel.h"
#include "ChartConfigRegistry.h"

#include <QPainter>
#include <QPainterPath>
#include <dsfw/Theme.h>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace dstools {
namespace chart {

PowerChartPanel::PowerChartPanel(ViewportController *viewport, QWidget *parent)
    : ChartPanelBase(QStringLiteral("power"), viewport, parent)
{
    setMinimumHeight(80);
    loadConfigParams();
}

void PowerChartPanel::registerChartConfig() {
    static bool registered = false;
    if (registered) return;
    registered = true;

    ChartConfigDescriptor desc;
    desc.chartId = QStringLiteral("power");
    desc.displayName = QStringLiteral("功率图");
    desc.params = {
        {QStringLiteral("windowSize"), QStringLiteral("RMS窗口大小"),
         ParamType::Int, QVariant(2048), QVariant(256), QVariant(8192),
         QStringLiteral(" samples"), 0,
         QStringLiteral("RMS计算窗口大小。值越大曲线越平滑。")},
        {QStringLiteral("minDb"), QStringLiteral("最小强度(dB)"),
         ParamType::Double, QVariant(-96.0), QVariant(-200.0), QVariant(0.0),
         QStringLiteral(" dB"), 1,
         QStringLiteral("功率图动态范围下限。低于此值的强度映射为黑色。")},
        {QStringLiteral("maxDb"), QStringLiteral("最大强度(dB)"),
         ParamType::Double, QVariant(0.0), QVariant(-60.0), QVariant(20.0),
         QStringLiteral(" dB"), 1,
         QStringLiteral("功率图动态范围上限。高于此值的强度映射为最亮色。")},
    };
    ChartConfigRegistry::instance().registerChart(desc);
}

void PowerChartPanel::loadConfigParams() {
    auto &reg = ChartConfigRegistry::instance();
    m_configWindowSize = reg.value(QStringLiteral("power"), QStringLiteral("windowSize")).toInt();
    m_configMinDb = static_cast<float>(reg.value(QStringLiteral("power"), QStringLiteral("minDb")).toDouble());
    m_configMaxDb = static_cast<float>(reg.value(QStringLiteral("power"), QStringLiteral("maxDb")).toDouble());
}

void PowerChartPanel::onAudioDataChanged() {
    m_fullDataDirty = true;
    update();
}

void PowerChartPanel::paintYAxisContent(QPainter &painter, const QRect &chartRect) {
    int labelW = chartRect.width();
    int h = height();

    painter.save();
    painter.setPen(QColor(160, 160, 180));
    QFont font(QStringLiteral("Arial"), 9);
    painter.setFont(font);

    std::vector<std::pair<float, QString>> levels;
    if (h >= 200) {
        levels = {{0.0f, QStringLiteral("0")},
                  {-6.0f, QStringLiteral("-6")},
                  {-12.0f, QStringLiteral("-12")},
                  {-24.0f, QStringLiteral("-24")},
                  {-48.0f, QStringLiteral("-48")},
                  {-96.0f, QStringLiteral("-96")}};
    } else if (h >= 120) {
        levels = {{0.0f, QStringLiteral("0")},
                  {-12.0f, QStringLiteral("-12")},
                  {-48.0f, QStringLiteral("-48")}};
    } else {
        levels = {{0.0f, QStringLiteral("0")}};
    }

    float range = m_configMaxDb - m_configMinDb;
    for (const auto &lv : levels) {
        float norm = (lv.first - m_configMinDb) / range;
        int yPos = static_cast<int>(h - norm * h);
        if (yPos < 0 || yPos >= h) continue;
        int textTop = std::clamp(yPos - 8, 0, h - 16);
        painter.drawText(QRect(0, textTop, labelW - 4, 16),
                         Qt::AlignRight | Qt::AlignVCenter, lv.second);
    }

    painter.drawLine(labelW - 1, 0, labelW - 1, h - 1);
    painter.restore();
}

void PowerChartPanel::renderFullData(QImage &image) {
    image.fill(Qt::transparent);
    if (m_samples.empty() || m_sampleRate <= 0) return;

    int imgW = image.width();
    int imgH = image.height();
    if (imgW <= 0 || imgH <= 0) return;

    double totalDur = dataDurationSec();
    if (totalDur <= 0.0) return;

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);

    double samplesPerCol = static_cast<double>(m_samples.size()) / imgW;
    int halfWindow = m_configWindowSize / 2;

    // Render RMS curve: compute RMS for each column, convert to dB, draw as path
    QPainterPath strokePath;
    QPainterPath fillPath;
    bool started = false;

    for (int col = 0; col < imgW; ++col) {
        double tCenter = totalDur * (col + 0.5) / imgW;
        int centerSample = static_cast<int>(tCenter * m_sampleRate);

        int wStart = std::max(0, centerSample - halfWindow);
        int wEnd = std::min(static_cast<int>(m_samples.size()), centerSample + halfWindow);

        float dB = m_configMinDb;
        if (wStart < wEnd) {
            double sumSq = 0.0;
            for (int s = wStart; s < wEnd; ++s) {
                double val = m_samples[s];
                sumSq += val * val;
            }
            double rms = std::sqrt(sumSq / (wEnd - wStart));
            if (rms >= 1e-10) {
                dB = static_cast<float>(20.0 * std::log10(rms));
            }
        }
        dB = std::clamp(dB, m_configMinDb, m_configMaxDb);

        float norm = (dB - m_configMinDb) / (m_configMaxDb - m_configMinDb);
        float y = imgH - norm * imgH;

        if (!started) {
            strokePath.moveTo(static_cast<float>(col), y);
            fillPath.moveTo(0.0f, static_cast<float>(imgH));
            fillPath.lineTo(static_cast<float>(col), y);
            started = true;
        } else {
            strokePath.lineTo(static_cast<float>(col), y);
            fillPath.lineTo(static_cast<float>(col), y);
        }
    }

    if (!started) return;

    fillPath.lineTo(static_cast<float>(imgW - 1), static_cast<float>(imgH));
    fillPath.closeSubpath();

    QColor powerColor = dsfw::Theme::instance().palette().audioVisualizer.powerColor;
    QColor fillColor = powerColor;
    fillColor.setAlpha(60);

    painter.setPen(Qt::NoPen);
    painter.setBrush(fillColor);
    painter.drawPath(fillPath);

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(powerColor, 1.5));
    painter.drawPath(strokePath);
}

double PowerChartPanel::dataDurationSec() const {
    if (m_samples.empty() || m_sampleRate <= 0) return 0.0;
    // m_samples is stereo interleaved
    return static_cast<double>(m_samples.size()) / (m_sampleRate * 2.0);
}

} // namespace chart
} // namespace dstools