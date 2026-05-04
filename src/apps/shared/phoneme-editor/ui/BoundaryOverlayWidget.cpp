#include "BoundaryOverlayWidget.h"
#include "TextGridDocument.h"

#include <dstools/TimePos.h>

#include <QPainter>
#include <QPen>
#include <QEvent>
#include <QResizeEvent>

#include <dsfw/Theme.h>

namespace dstools {
namespace phonemelabeler {

BoundaryOverlayWidget::BoundaryOverlayWidget(ViewportController *viewport, QWidget *parent)
    : QWidget(parent), m_viewport(viewport)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_AlwaysStackOnTop, true);
    setFocusPolicy(Qt::NoFocus);
}

void BoundaryOverlayWidget::setDocument(TextGridDocument *doc) {
    m_document = doc;
    update();
}

void BoundaryOverlayWidget::trackWidget(QWidget *widget) {
    if (m_trackedWidget) {
        m_trackedWidget->removeEventFilter(this);
    }
    m_trackedWidget = widget;
    if (m_trackedWidget) {
        m_trackedWidget->installEventFilter(this);
        repositionOverSplitter();
    }
}

void BoundaryOverlayWidget::setViewport(const ViewportState &state) {
    m_viewStart = state.startSec;
    m_viewEnd = state.endSec;
    update();
}

void BoundaryOverlayWidget::setHoveredBoundary(int index) {
    if (m_hoveredBoundary != index) {
        m_hoveredBoundary = index;
        update();
    }
}

void BoundaryOverlayWidget::setDraggedBoundary(int index) {
    if (m_draggedBoundary != index) {
        m_draggedBoundary = index;
        update();
    }
}

void BoundaryOverlayWidget::setPlayhead(double sec) {
    if (m_playhead != sec) {
        m_playhead = sec;
        update();
    }
}

void BoundaryOverlayWidget::paintEvent(QPaintEvent * /*event*/) {
    QPainter painter(this);
    int h = height();
    int w = width();

    // Draw playhead cursor (across all charts)
    if (m_playhead >= 0.0) {
        int px = timeToX(m_playhead);
        if (px >= 0 && px <= w) {
            painter.setPen(QPen(QColor(255, 80, 80), 2));
            painter.drawLine(px, 0, px, h);
        }
    }

    if (!m_document) return;

    int activeTier = m_document->activeTierIndex();
    if (activeTier < 0 || activeTier >= m_document->tierCount()) return;

    int count = m_document->boundaryCount(activeTier);
    if (count == 0) return;

    for (int b = 0; b < count; ++b) {
        double t = usToSec(m_document->boundaryTime(activeTier, b));
        int x = timeToX(t);
        if (x < 0 || x > w) continue;

        if (b == m_draggedBoundary) {
            painter.setPen(QPen(dsfw::Theme::instance().palette().phonemeEditor.boundaryDragged, 2));
        } else if (b == m_hoveredBoundary) {
            painter.setPen(QPen(dsfw::Theme::instance().palette().phonemeEditor.boundaryHovered, 2));
        } else {
            painter.setPen(QPen(dsfw::Theme::instance().palette().phonemeEditor.boundaryNormal, 1, Qt::SolidLine));
        }
        painter.drawLine(x, 0, x, h);
    }
}

bool BoundaryOverlayWidget::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_trackedWidget &&
        (event->type() == QEvent::Resize || event->type() == QEvent::Move)) {
        repositionOverSplitter();
    }
    return QWidget::eventFilter(watched, event);
}

int BoundaryOverlayWidget::timeToX(double time) const {
    double viewDuration = m_viewEnd - m_viewStart;
    if (viewDuration <= 0.0 || width() <= 0) return 0;
    return static_cast<int>((time - m_viewStart) / viewDuration * width());
}

void BoundaryOverlayWidget::repositionOverSplitter() {
    if (!m_trackedWidget) return;
    // Map the tracked widget's geometry to our parent's coordinate space
    QPoint topLeft = m_trackedWidget->mapTo(parentWidget(), QPoint(0, 0));
    setGeometry(topLeft.x(), topLeft.y(), m_trackedWidget->width(), m_trackedWidget->height());
    raise();
}

} // namespace phonemelabeler
} // namespace dstools
