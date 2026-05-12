#include "PowerWidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QHideEvent>

#include <algorithm>
#include <cmath>

namespace dstools {
namespace phonemelabeler {

PowerWidget::PowerWidget(ViewportController *viewport, QWidget *parent)
    : AudioChartWidget(viewport, parent)
{
    setMinimumHeight(80);
}

PowerWidget::~PowerWidget() = default;

void PowerWidget::setAudioData(const std::vector<float> &samples, int sampleRate) {
    m_samples = samples;
    m_sampleRate = sampleRate;
    rebuildPowerCache();
    update();
}

void PowerWidget::setDocument(TextGridDocument *doc) {
    m_document = doc;
    setBoundaryModel(doc);
}

void PowerWidget::rebuildCache() {
    rebuildPowerCache();
}

void PowerWidget::rebuildPowerCache() {
    m_cachedViewStart = m_viewStart;
    m_cachedViewEnd = m_viewEnd;
    m_cachedWidth = width();
}

void PowerWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), QColor(5, 5, 10));

    if (m_samples.empty()) {
        painter.setPen(Qt::gray);
        painter.drawText(rect(), Qt::AlignCenter, tr("No audio loaded"));
        return;
    }

    drawReferenceLines(painter);
    drawPower(painter);
}

void PowerWidget::drawReferenceLines(QPainter &painter) {
    painter.setPen(QPen(QColor(80, 80, 80), 1, Qt::DashLine));
    int yMid = height() / 2;
    painter.drawLine(0, yMid, width(), yMid);
}

void PowerWidget::drawPower(QPainter &painter) {
    if (m_samples.empty() || m_sampleRate <= 0) return;

    double totalDuration = static_cast<double>(m_samples.size()) / m_sampleRate;
    if (totalDuration <= 0.0) return;

    int w = width();
    int h = height();
    if (w <= 0 || h <= 0) return;

    double regionStart = m_viewStart;
    double regionEnd = m_viewEnd;
    double regionDuration = regionEnd - regionStart;
    if (regionDuration <= 0.0) return;

    int startSample = static_cast<int>(regionStart / totalDuration * m_samples.size());
    int endSample = static_cast<int>(regionEnd / totalDuration * m_samples.size());
    if (startSample < 0) startSample = 0;
    if (endSample > static_cast<int>(m_samples.size())) endSample = static_cast<int>(m_samples.size());
    if (endSample <= startSample) return;

    std::vector<double> rmsCache(w, 0.0);
    for (int x = 0; x < w; ++x) {
        double t = regionStart + regionDuration * (x + 0.5) / w;
        int centerSample = static_cast<int>(t / totalDuration * m_samples.size());
        int halfWin = static_cast<int>(m_sampleRate * 0.01);
        int s0 = std::max(0, centerSample - halfWin);
        int s1 = std::min(static_cast<int>(m_samples.size()), centerSample + halfWin);
        if (s1 <= s0) continue;

        double sum = 0.0;
        for (int s = s0; s < s1; ++s)
            sum += m_samples[s] * m_samples[s];
        double rms = std::sqrt(sum / (s1 - s0));
        rmsCache[x] = rms;
    }

    double maxRms = 0.01;
    for (int x = 0; x < w; ++x)
        if (rmsCache[x] > maxRms) maxRms = rmsCache[x];

    if (maxRms <= 0.0) return;

    painter.setPen(Qt::NoPen);
    QColor topColor(0, 200, 100, 200);
    QColor bottomColor(200, 0, 100, 200);
    int yMid = h / 2;

    for (int x = 0; x < w; ++x) {
        double norm = rmsCache[x] / maxRms;
        if (norm < 0.001) continue;
        int barHeight = static_cast<int>(norm * yMid * 0.8);
        if (barHeight < 1) barHeight = 1;

        double t = regionStart + regionDuration * (x + 0.5) / w;
        double frac = t / totalDuration;
        double ratio = std::clamp(frac, 0.0, 1.0);
        int r = static_cast<int>(bottomColor.red() + (topColor.red() - bottomColor.red()) * ratio);
        int g = static_cast<int>(bottomColor.green() + (topColor.green() - bottomColor.green()) * ratio);
        int b = static_cast<int>(bottomColor.blue() + (topColor.blue() - bottomColor.blue()) * ratio);
        int alpha = static_cast<int>(140 + 80 * norm);
        painter.setPen(QPen(QColor(r, g, b, alpha), 1));
        painter.drawLine(x, yMid - 1, x, yMid - 1 - barHeight);
        painter.drawLine(x, yMid + 1, x, yMid + 1 + barHeight);
    }
}

void PowerWidget::resizeEvent(QResizeEvent *event) {
    rebuildPowerCache();
    AudioChartWidget::resizeEvent(event);
}

void PowerWidget::showEvent(QShowEvent *event) {
    AudioChartWidget::showEvent(event);
    emit visibleStateChanged(true);
}

void PowerWidget::hideEvent(QHideEvent *event) {
    AudioChartWidget::hideEvent(event);
    emit visibleStateChanged(false);
}

} // namespace phonemelabeler
} // namespace dstools