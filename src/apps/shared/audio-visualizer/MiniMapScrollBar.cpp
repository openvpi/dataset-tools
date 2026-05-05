#include "MiniMapScrollBar.h"

#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QWheelEvent>

namespace dstools {

using dsfw::widgets::ViewportController;
using dsfw::widgets::ViewportState;

MiniMapScrollBar::MiniMapScrollBar(ViewportController *viewport, QWidget *parent)
    : QWidget(parent), m_viewport(viewport) {
    setFixedHeight(kFixedHeight);
    setCursor(Qt::OpenHandCursor);
    setMouseTracking(true);

    if (m_viewport) {
        m_viewStart = m_viewport->startSec();
        m_viewEnd = m_viewport->endSec();
        m_totalDuration = m_viewport->totalDuration();
    }
}

MiniMapScrollBar::~MiniMapScrollBar() = default;

QSize MiniMapScrollBar::minimumSizeHint() const {
    return {40, kFixedHeight};
}

QSize MiniMapScrollBar::sizeHint() const {
    return {200, kFixedHeight};
}

void MiniMapScrollBar::setAudioData(const std::vector<float> &samples, int sampleRate) {
    m_samples = samples;
    m_sampleRate = sampleRate > 0 ? sampleRate : 44100;
    m_totalDuration = m_samples.empty() ? 0.0
                                        : static_cast<double>(m_samples.size()) / m_sampleRate;
    if (m_viewport)
        m_viewport->setTotalDuration(m_totalDuration);
    rebuildPeakCache();
    update();
}

void MiniMapScrollBar::setViewport(const ViewportState &state) {
    m_viewStart = state.startSec;
    m_viewEnd = state.endSec;
    update();
}

void MiniMapScrollBar::rebuildPeakCache() {
    m_peakCache.clear();
    if (m_samples.empty() || width() <= 0 || m_totalDuration <= 0.0)
        return;

    int numPixels = width();
    m_peakCache.resize(numPixels);

    for (int x = 0; x < numPixels; ++x) {
        double tStart = m_totalDuration * x / numPixels;
        double tEnd = m_totalDuration * (x + 1) / numPixels;

        int sStart = std::max(0, static_cast<int>(tStart * m_sampleRate));
        int sEnd = std::min(static_cast<int>(m_samples.size()),
                            static_cast<int>(tEnd * m_sampleRate));

        if (sStart >= sEnd || sStart >= static_cast<int>(m_samples.size())) {
            m_peakCache[x] = {0.0f, 0.0f};
            continue;
        }

        float mn = m_samples[sStart];
        float mx = m_samples[sStart];
        for (int s = sStart + 1; s < sEnd; ++s) {
            mn = std::min(mn, m_samples[s]);
            mx = std::max(mx, m_samples[s]);
        }
        m_peakCache[x] = {mn, mx};
    }
}

double MiniMapScrollBar::xToTime(int x) const {
    if (width() <= 0) return 0.0;
    return m_totalDuration * x / width();
}

int MiniMapScrollBar::timeToX(double sec) const {
    if (m_totalDuration <= 0.0 || width() <= 0) return 0;
    return static_cast<int>(sec / m_totalDuration * width());
}

void MiniMapScrollBar::paintEvent(QPaintEvent * /*event*/) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    p.fillRect(rect(), QColor(30, 30, 30));

    if (!m_peakCache.empty()) {
        int h = height();
        int midY = h / 2;

        QPen wavePen(QColor(100, 180, 255));
        wavePen.setWidth(1);
        p.setPen(wavePen);

        for (int x = 0; x < static_cast<int>(m_peakCache.size()); ++x) {
            float mn = m_peakCache[x].min;
            float mx = m_peakCache[x].max;
            int y1 = midY - static_cast<int>(mx * midY * 0.9f);
            int y2 = midY - static_cast<int>(mn * midY * 0.9f);
            if (y1 > y2)
                std::swap(y1, y2);
            p.drawLine(x, y1, x, y2);
        }
    }

    if (m_totalDuration > 0.0) {
        int x1 = timeToX(m_viewStart);
        int x2 = timeToX(m_viewEnd);

        QColor windowColor(80, 140, 220, 50);
        QColor borderColor(80, 140, 220, 180);

        p.fillRect(x1, 0, x2 - x1, height(), windowColor);

        QPen borderPen(borderColor);
        borderPen.setWidth(2);
        p.setPen(borderPen);
        p.setBrush(Qt::NoBrush);
        p.drawRect(x1, 0, x2 - x1 - 1, height() - 1);

        QPen gripPen(QColor(200, 220, 255, 200));
        gripPen.setWidth(2);
        p.setPen(gripPen);
        int gripLen = 8;
        int gripY1 = height() / 2 - gripLen / 2;
        int gripY2 = height() / 2 + gripLen / 2;
        p.drawLine(x1 + 3, gripY1, x1 + 3, gripY2);
        p.drawLine(x2 - 3, gripY1, x2 - 3, gripY2);
    }
}

void MiniMapScrollBar::mousePressEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton || !m_viewport || m_totalDuration <= 0.0)
        return;

    int mx = event->pos().x();
    int x1 = timeToX(m_viewStart);
    int x2 = timeToX(m_viewEnd);

    if (std::abs(mx - x1) <= kEdgeWidth) {
        m_dragMode = DragMode::ZoomLeft;
    } else if (std::abs(mx - x2) <= kEdgeWidth) {
        m_dragMode = DragMode::ZoomRight;
    } else if (mx > x1 && mx < x2) {
        m_dragMode = DragMode::Scroll;
    } else {
        double clickSec = xToTime(mx);
        double viewDur = m_viewEnd - m_viewStart;
        double newStart = clickSec - viewDur / 2.0;
        double newEnd = newStart + viewDur;
        if (newStart < 0.0) {
            newStart = 0.0;
            newEnd = viewDur;
        }
        if (m_totalDuration > 0.0 && newEnd > m_totalDuration) {
            newEnd = m_totalDuration;
            newStart = newEnd - viewDur;
            if (newStart < 0.0) newStart = 0.0;
        }
        m_viewport->setViewRange(newStart, newEnd);
        m_dragMode = DragMode::Scroll;
    }

    m_dragStartX = mx;
    m_dragStartViewStart = m_viewStart;
    m_dragStartViewEnd = m_viewEnd;

    if (m_dragMode == DragMode::Scroll)
        setCursor(Qt::ClosedHandCursor);
    else if (m_dragMode != DragMode::None)
        setCursor(Qt::SizeHorCursor);
}

void MiniMapScrollBar::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragMode == DragMode::None) {
        int mx = event->pos().x();
        int x1 = timeToX(m_viewStart);
        int x2 = timeToX(m_viewEnd);
        if (std::abs(mx - x1) <= kEdgeWidth || std::abs(mx - x2) <= kEdgeWidth)
            setCursor(Qt::SizeHorCursor);
        else
            setCursor(Qt::OpenHandCursor);
        return;
    }

    int dx = event->pos().x() - m_dragStartX;
    double dtSec = static_cast<double>(dx) / width() * m_totalDuration;

    switch (m_dragMode) {
    case DragMode::Scroll: {
        double viewDur = m_dragStartViewEnd - m_dragStartViewStart;
        double newStart = m_dragStartViewStart + dtSec;
        double newEnd = newStart + viewDur;
        m_viewport->setViewRange(newStart, newEnd);
        break;
    }
    case DragMode::ZoomLeft: {
        double newStart = m_dragStartViewStart + dtSec;
        if (newStart < 0.0) newStart = 0.0;
        if (newStart >= m_dragStartViewEnd - 0.01)
            newStart = m_dragStartViewEnd - 0.01;
        m_viewport->setViewRange(newStart, m_dragStartViewEnd);
        break;
    }
    case DragMode::ZoomRight: {
        double newEnd = m_dragStartViewEnd + dtSec;
        if (m_totalDuration > 0.0 && newEnd > m_totalDuration)
            newEnd = m_totalDuration;
        if (newEnd <= m_dragStartViewStart + 0.01)
            newEnd = m_dragStartViewStart + 0.01;
        m_viewport->setViewRange(m_dragStartViewStart, newEnd);
        break;
    }
    case DragMode::None:
        break;
    }
}

void MiniMapScrollBar::mouseReleaseEvent(QMouseEvent * /*event*/) {
    m_dragMode = DragMode::None;
    setCursor(Qt::OpenHandCursor);
}

void MiniMapScrollBar::mouseDoubleClickEvent(QMouseEvent *event) {
    if (!m_viewport || m_totalDuration <= 0.0)
        return;

    double clickSec = xToTime(event->pos().x());
    double viewDur = m_viewEnd - m_viewStart;
    double newStart = clickSec - viewDur / 2.0;
    double newEnd = newStart + viewDur;
    if (newStart < 0.0) {
        newStart = 0.0;
        newEnd = viewDur;
    }
    if (m_totalDuration > 0.0 && newEnd > m_totalDuration) {
        newEnd = m_totalDuration;
        newStart = newEnd - viewDur;
        if (newStart < 0.0) newStart = 0.0;
    }
    m_viewport->setViewRange(newStart, newEnd);
}

void MiniMapScrollBar::wheelEvent(QWheelEvent *event) {
    if (!m_viewport)
        return;

    if (event->modifiers() & Qt::ControlModifier) {
        // Ctrl+scroll = zoom
        double sec = xToTime(event->position().toPoint().x());
        double factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
        m_viewport->zoomAt(sec, factor);
    } else {
        // No modifier = horizontal scroll
        double scrollSec = (event->angleDelta().y() > 0) ? -0.5 : 0.5;
        m_viewport->scrollBy(scrollSec);
    }
    event->accept();
}

void MiniMapScrollBar::resizeEvent(QResizeEvent * /*event*/) {
    rebuildPeakCache();
}

} // namespace dstools
