#include "MouthCurveChartPanel.h"
#include "ChartConfigRegistry.h"

#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>

namespace dstools {

MouthCurveChartPanel::MouthCurveChartPanel(dsfw::widgets::ViewportController* viewport, QWidget* parent)
    : ChartPanelBase(QStringLiteral("mouthCurve"), viewport, parent) {
    setMinimumHeight(120);
    m_amplitudeScale = 2.0;
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
    m_fullDataDirty = true;
    update();
}

void MouthCurveChartPanel::paintYAxisContent(QPainter& painter, const QRect& chartRect) {
    constexpr int kPadding = chart::ChartPanelBase::kYAxisPadding;
    QRect innerRect(chartRect.x(), chartRect.y() + kPadding, chartRect.width(), chartRect.height() - 2 * kPadding);
    paintYAxisTicks(painter, innerRect, kCurveMin, kCurveMax, kYTickCount, 2);
}

void MouthCurveChartPanel::renderFullData(QImage &image) {
    image.fill(Qt::transparent);
    if (m_curve.isEmpty()) return;

    int imgW = image.width();
    int imgH = image.height();
    if (imgW <= 0 || imgH <= 0) return;

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);

    double totalDur = dataDurationSec();
    if (totalDur <= 0.0) return;

    QPainterPath path;
    bool started = false;

    for (int col = 0; col < imgW; ++col) {
        // Map column to time, then to curve index using timeToIndex
        double t = totalDur * col / imgW;
        double curveIdx = t / (totalDur / m_curve.values.size());

        int idx = static_cast<int>(curveIdx);
        if (idx < 0 || idx >= static_cast<int>(m_curve.values.size())) continue;

        float val = m_curve.values[idx] * static_cast<float>(m_amplitudeScale);
        val = std::clamp(val, kCurveMin, kCurveMax);

        float norm = (val - kCurveMin) / (kCurveMax - kCurveMin);
        float y = imgH - norm * imgH;

        if (!started) {
            path.moveTo(static_cast<float>(col), y);
            started = true;
        } else {
            path.lineTo(static_cast<float>(col), y);
        }
    }

    if (!started) return;

    painter.setPen(QPen(QColor(0x00, 0xDC, 0xDC), 2));
    painter.drawPath(path);
}

double MouthCurveChartPanel::dataDurationSec() const {
    return m_curve.durationSec();
}

} // namespace dstools
