#include "SliceNumberLayer.h"

#include <QPainter>
#include <QPaintEvent>

namespace dstools {

SliceNumberLayer::SliceNumberLayer(ViewportController *viewport, QWidget *parent)
    : QWidget(parent), m_viewport(viewport) {
    setFixedHeight(28);

    if (m_viewport) {
        connect(m_viewport, &ViewportController::viewportChanged, this,
                [this](const ViewportState &state) { setViewport(state); });
        m_viewStart = m_viewport->state().startSec;
        m_viewEnd = m_viewport->state().endSec;
    }
}

void SliceNumberLayer::setSlicePoints(const std::vector<double> &points) {
    m_slicePoints = points;
    update();
}

void SliceNumberLayer::setViewport(const ViewportState &state) {
    m_viewStart = state.startSec;
    m_viewEnd = state.endSec;
    update();
}

int SliceNumberLayer::timeToX(double time) const {
    double viewDuration = m_viewEnd - m_viewStart;
    if (viewDuration <= 0.0 || width() <= 0)
        return 0;
    return static_cast<int>((time - m_viewStart) / viewDuration * width());
}

void SliceNumberLayer::paintEvent(QPaintEvent * /*event*/) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor(40, 40, 45));

    if (m_slicePoints.empty()) {
        painter.setPen(Qt::gray);
        painter.drawText(rect(), Qt::AlignCenter, tr("No slices"));
        return;
    }

    QFont font = painter.font();
    font.setPixelSize(12);
    font.setBold(true);
    painter.setFont(font);

    std::vector<double> boundaries;
    boundaries.push_back(0.0);
    for (double p : m_slicePoints)
        boundaries.push_back(p);

    for (size_t i = 0; i + 1 < boundaries.size(); ++i) {
        double segStart = boundaries[i];
        double segEnd = boundaries[i + 1];

        int x1 = timeToX(segStart);
        int x2 = timeToX(segEnd);

        if (x2 < 0 || x1 > width())
            continue;

        painter.setPen(QPen(QColor(255, 200, 100), 1));
        if (i > 0)
            painter.drawLine(x1, 0, x1, height());

        int segWidth = x2 - x1;
        if (segWidth > 20) {
            painter.setPen(QColor(200, 200, 220));
            QString label = QString("%1").arg(i + 1, 3, 10, QChar('0'));
            QRect textRect(x1 + 2, 0, segWidth - 4, height());
            painter.drawText(textRect, Qt::AlignCenter, label);
        }
    }

    if (!m_slicePoints.empty()) {
        int x = timeToX(m_slicePoints.back());
        painter.setPen(QPen(QColor(255, 200, 100), 1));
        painter.drawLine(x, 0, x, height());
    }

    painter.setPen(QPen(QColor(80, 80, 100), 1));
    painter.drawLine(0, 0, width(), 0);
    painter.drawLine(0, height() - 1, width(), height() - 1);
}

} // namespace dstools
