#include "MouthCurveChartPanel.h"
#include "ChartConfigRegistry.h"

#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>

namespace dstools {

MouthCurveChartPanel::MouthCurveChartPanel(dsfw::widgets::ViewportController* viewport, QWidget* parent)
    : ChartPanelBase(QStringLiteral("mouthCurve"), viewport, parent) {
    setMinimumHeight(120);
    m_amplitudeScale = 1.0;
    loadConfigParams();
}

void MouthCurveChartPanel::registerChartConfig() {
    static bool registered = false;
    if (registered)
        return;
    registered = true;

    chart::ChartConfigDescriptor desc;
    desc.chartId = QStringLiteral("mouthCurve");
    desc.displayName = QStringLiteral("口型曲线图");
    desc.params = {
        {QStringLiteral("amplitudeMin"), QStringLiteral("最小振幅"), chart::ParamType::Double, QVariant(0.5),
         QVariant(0.1), QVariant(5.0), QStringLiteral(""), 1, QStringLiteral("口型曲线垂直缩放下限。越小曲线越扁。")},
        {QStringLiteral("amplitudeMax"), QStringLiteral("最大振幅"), chart::ParamType::Double, QVariant(10.0),
         QVariant(2.0), QVariant(50.0), QStringLiteral(""), 1,
         QStringLiteral("口型曲线垂直缩放上限。越大曲线波动越平缓。")},
    };
    chart::ChartConfigRegistry::instance().registerChart(desc);
}

void MouthCurveChartPanel::loadConfigParams() {
    auto& reg = chart::ChartConfigRegistry::instance();
    m_configAmplitudeMin = reg.value(QStringLiteral("mouthCurve"), QStringLiteral("amplitudeMin")).toDouble();
    m_configAmplitudeMax = reg.value(QStringLiteral("mouthCurve"), QStringLiteral("amplitudeMax")).toDouble();
}

double MouthCurveChartPanel::amplitudeMinForZoom() const {
    return m_configAmplitudeMin;
}

double MouthCurveChartPanel::amplitudeMaxForZoom() const {
    return m_configAmplitudeMax;
}

void MouthCurveChartPanel::setData(const MouthCurve& curve) {
    m_curve = curve;
    m_cacheDirty = true;
    update();
}

void MouthCurveChartPanel::rebuildCache(const chart::RegionUpdate& region) {
    Q_UNUSED(region)
}

void MouthCurveChartPanel::drawContent(QPainter& painter, const chart::ChartCoordinate& coord) {
    Q_UNUSED(coord)
    if (m_curve.isEmpty()) {
        drawEmptyState(painter, QStringLiteral("No mouth curve data"));
        return;
    }

    paintCurve(painter);
}

void MouthCurveChartPanel::paintYAxisContent(QPainter& painter, const QRect& chartRect) {
    constexpr int kPadding = chart::ChartPanelBase::kYAxisPadding;
    QRect innerRect(chartRect.x(), chartRect.y() + kPadding, chartRect.width(), chartRect.height() - 2 * kPadding);
    paintYAxisTicks(painter, innerRect, kCurveMin, kCurveMax, kYTickCount, 2);
}

void MouthCurveChartPanel::paintCurve(QPainter& painter) {
    int yaw = yAxisWidth();
    QRect chartRect = chartContentRect();
    int cols = chartRect.width();

    if (cols <= 0)
        return;

    double totalDur = dataDurationSec();
    if (totalDur <= 0.0)
        return;

    int dataSize = static_cast<int>(m_curve.values.size());

    painter.save();
    painter.setClipRect(chartRect);

    QPainterPath path;
    bool started = false;

    for (int col = 0; col < cols; ++col) {
        double t = m_converter->startSec() +
                   (m_converter->endSec() - m_converter->startSec()) * static_cast<double>(col) / cols;

        int curveIdx = static_cast<int>(t / totalDur * dataSize);
        if (curveIdx < 0 || curveIdx >= dataSize)
            continue;

        float val = m_curve.values[curveIdx] * static_cast<float>(m_amplitudeScale);
        val = std::clamp(val, kCurveMin, kCurveMax);

        int x = yaw + col;
        int y = chartRect.bottom() - static_cast<int>(val / (kCurveMax - kCurveMin) * chartRect.height());

        if (!started) {
            path.moveTo(x, y);
            started = true;
        } else {
            path.lineTo(x, y);
        }
    }

    painter.setPen(QPen(QColor(0x00, 0xDC, 0xDC), 2));
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.drawPath(path);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.restore();
}

void MouthCurveChartPanel::renderFullData(QImage &image) {
    Q_UNUSED(image)
}

double MouthCurveChartPanel::dataDurationSec() const {
    return m_curve.durationSec();
}

} // namespace dstools
