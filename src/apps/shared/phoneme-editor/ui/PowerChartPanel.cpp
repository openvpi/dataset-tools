#include "PowerChartPanel.h"

#include <QPainter>
#include <QPainterPath>

#include <cmath>
#include <algorithm>

namespace dstools {
namespace phonemelabeler {

PowerChartPanel::PowerChartPanel(ViewportController *viewport, QWidget *parent)
    : ChartPanelBase(QStringLiteral("power"), viewport, parent)
{
    setMinimumHeight(80);
}

void PowerChartPanel::setAudioData(const std::vector<float> &samples, int sampleRate) {
    m_samples = samples;
    m_sampleRate = sampleRate;
    rebuildPowerCache();
    update();
}

void PowerChartPanel::drawContent(QPainter &painter) {
    if (m_samples.empty()) {
        painter.setPen(Qt::gray);
        painter.drawText(rect(), Qt::AlignCenter, tr("No audio loaded"));
        return;
    }

    drawReferenceLines(painter);
    drawPower(painter);
}

void PowerChartPanel::rebuildCache() {
    rebuildPowerCache();
}

void PowerChartPanel::drawReferenceLines(QPainter & /*painter*/) {
}

void PowerChartPanel::drawPower(QPainter &painter) {
    if (m_powerCache.empty()) return;

    painter.setRenderHint(QPainter::Antialiasing, true);

    int w = width();
    int h = height();
    int numPixels = std::min(w, static_cast<int>(m_powerCache.size()));
    if (numPixels < 2) return;

    auto getY = [&](int x) -> float {
        float norm = std::clamp((m_powerCache[x] - kMinPower) / (kMaxPower - kMinPower), 0.0f, 1.0f);
        return h - norm * h;
    };

    constexpr int kStep = 4;
    struct Pt { float x, y; };
    std::vector<Pt> pts;
    for (int x = 0; x < numPixels; x += kStep) {
        pts.push_back({static_cast<float>(x), getY(x)});
    }
    if (pts.empty() || static_cast<int>(pts.back().x) != numPixels - 1) {
        pts.push_back({static_cast<float>(numPixels - 1), getY(numPixels - 1)});
    }

    if (pts.size() < 2) return;

    QPainterPath strokePath;
    strokePath.moveTo(pts[0].x, pts[0].y);

    QPainterPath fillPath;
    fillPath.moveTo(0, h);
    fillPath.lineTo(pts[0].x, pts[0].y);

    for (size_t i = 0; i + 1 < pts.size(); ++i) {
        Pt p0 = (i > 0) ? pts[i - 1] : pts[i];
        Pt p1 = pts[i];
        Pt p2 = pts[i + 1];
        Pt p3 = (i + 2 < pts.size()) ? pts[i + 2] : pts[i + 1];

        float cp1x = p1.x + (p2.x - p0.x) / 6.0f;
        float cp1y = p1.y + (p2.y - p0.y) / 6.0f;
        float cp2x = p2.x - (p3.x - p1.x) / 6.0f;
        float cp2y = p2.y - (p3.y - p1.y) / 6.0f;

        strokePath.cubicTo(cp1x, cp1y, cp2x, cp2y, p2.x, p2.y);
        fillPath.cubicTo(cp1x, cp1y, cp2x, cp2y, p2.x, p2.y);
    }

    fillPath.lineTo(numPixels - 1, h);
    fillPath.closeSubpath();

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(128, 255, 128, 60));
    painter.drawPath(fillPath);

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(128, 255, 128, 200), 1.5));
    painter.drawPath(strokePath);

    painter.setRenderHint(QPainter::Antialiasing, false);
}

void PowerChartPanel::rebuildPowerCache() {
    if (m_samples.empty() || m_xf.viewEnd <= m_xf.viewStart) {
        m_powerCache.clear();
        return;
    }

    int numPixels = std::max(1, width());
    m_powerCache.resize(numPixels);

    int halfWindow = kWindowSize / 2;

    for (int x = 0; x < numPixels; ++x) {
        double tCenter = m_xf.viewStart + (m_xf.viewEnd - m_xf.viewStart) * (x + 0.5) / numPixels;
        int centerSample = static_cast<int>(tCenter * m_sampleRate);

        int wStart = std::max(0, centerSample - halfWindow);
        int wEnd = std::min(static_cast<int>(m_samples.size()), centerSample + halfWindow);

        if (wStart >= wEnd) {
            m_powerCache[x] = kMinPower;
            continue;
        }

        double sumSq = 0.0;
        for (int s = wStart; s < wEnd; ++s) {
            double val = m_samples[s] * kRefValue;
            sumSq += val * val;
        }
        double rms = std::sqrt(sumSq / (wEnd - wStart));

        float dB;
        if (rms < 1e-10) {
            dB = kMinPower;
        } else {
            dB = static_cast<float>(20.0 * std::log10(rms / kRefValue));
        }
        m_powerCache[x] = std::clamp(dB, kMinPower, kMaxPower);
    }
}

} // namespace phonemelabeler
} // namespace dstools