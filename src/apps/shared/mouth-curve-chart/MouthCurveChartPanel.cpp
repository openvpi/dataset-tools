#include "MouthCurveChartPanel.h"
#include "ChartConfigRegistry.h"

#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>

namespace dstools {

using namespace dsfw;

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

    dstools::ChartConfigDescriptor desc;
    desc.chartId = QStringLiteral("mouthCurve");
    desc.displayName = QStringLiteral("口型曲线图");
    desc.params = {
        {QStringLiteral("amplitudeMin"), QStringLiteral("最小振幅"), dstools::ParamType::Double, QVariant(0.5),
         QVariant(0.1), QVariant(5.0), QStringLiteral(""), 1, QStringLiteral("口型曲线垂直缩放下限。越小曲线越扁。")},
        {QStringLiteral("amplitudeMax"), QStringLiteral("最大振幅"), dstools::ParamType::Double, QVariant(10.0),
         QVariant(2.0), QVariant(50.0), QStringLiteral(""), 1,
         QStringLiteral("口型曲线垂直缩放上限。越大曲线波动越平缓。")},
    };
    dstools::ChartConfigRegistry::instance().registerChart(desc);
}

void MouthCurveChartPanel::loadConfigParams() {
    auto& reg = dstools::ChartConfigRegistry::instance();
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

void MouthCurveChartPanel::rebuildCache(const dstools::RegionUpdate& region) {
    Q_UNUSED(region)
}

void MouthCurveChartPanel::drawContent(QPainter& painter, const dstools::ChartCoordinate& coord) {
    Q_UNUSED(coord)
    if (m_curve.isEmpty()) {
        drawEmptyState(painter, QStringLiteral("No mouth curve data"));
        return;
    }

    paintCurve(painter);
}

void MouthCurveChartPanel::paintYAxisContent(QPainter& painter, const QRect& chartRect) {
    constexpr int kPadding = dstools::ChartPanelBase::kYAxisPadding;
    QRect innerRect(chartRect.x(), chartRect.y() + kPadding, chartRect.width(), chartRect.height() - 2 * kPadding);
    paintYAxisTicks(painter, innerRect, kCurveMin, kCurveMax, kYTickCount, 2);
}

// Catmull-Rom cubic spline interpolation.
// Returns the interpolated value between p1 and p2 at parameter t in [0,1].
static float catmullRom(float p0, float p1, float p2, float p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    return 0.5f * ((2.0f * p1) + (-p0 + p2) * t + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                   (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

// Cubic-interpolated value lookup from the curve data, with boundary fallback to linear.
static float cubicValueAt(const std::vector<float>& values, double indexF) {
    int n = static_cast<int>(values.size());
    if (n == 0)
        return 0.0f;
    int i = static_cast<int>(indexF);
    float frac = static_cast<float>(indexF - i);

    // Fall back to linear interpolation at boundaries (not enough neighbors for cubic)
    if (i < 0) {
        return values.front();
    }
    if (i >= n - 1) {
        return values.back();
    }
    if (i < 1 || i >= n - 2) {
        // Linear fallback
        return values[i] * (1.0f - frac) + values[i + 1] * frac;
    }

    // Catmull-Rom cubic interpolation using 4 surrounding points
    return catmullRom(values[i - 1], values[i], values[i + 1], values[i + 2], frac);
}

void MouthCurveChartPanel::paintCurve(QPainter& painter) {
    int yaw = yAxisWidth();
    QRect chartRect = chartContentRect();
    int cols = chartRect.width();

    if (cols <= 0)
        return;

    if (!m_converter)
        return;

    double totalDur = dataDurationSec();
    if (totalDur <= 0.0)
        return;

    painter.save();
    painter.setClipRect(chartRect);

    QPainterPath path;
    bool started = false;

    double viewStart = m_converter->startSec();
    double viewEnd = m_converter->endSec();
    double viewDuration = viewEnd - viewStart;

    const auto timestepUs = static_cast<double>(m_curve.timestep);
    const auto& values = m_curve.values;

    if (timestepUs <= 0.0 || values.size() < 2) {
        // Fallback to linear interpolation for sparse data
        for (int col = 0; col < cols; ++col) {
            double t = viewStart + viewDuration * static_cast<double>(col) / cols;
            if (t < 0.0 || t > totalDur)
                continue;

            float val = m_curve.getValueAtTime(secToUs(t)) * static_cast<float>(m_amplitudeScale);

            int x = yaw + col;
            int y = chartRect.bottom() - static_cast<int>(val / (kCurveMax - kCurveMin) * chartRect.height());

            if (!started) {
                path.moveTo(x, y);
                started = true;
            } else {
                path.lineTo(x, y);
            }
        }
    } else {
        for (int col = 0; col < cols; ++col) {
            double t = viewStart + viewDuration * static_cast<double>(col) / cols;
            if (t < 0.0 || t > totalDur)
                continue;

            double indexF = secToUs(t) / timestepUs;
            float val = cubicValueAt(values, indexF) * static_cast<float>(m_amplitudeScale);

            int x = yaw + col;
            int y = chartRect.bottom() - static_cast<int>(val / (kCurveMax - kCurveMin) * chartRect.height());

            if (!started) {
                path.moveTo(x, y);
                started = true;
            } else {
                path.lineTo(x, y);
            }
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
