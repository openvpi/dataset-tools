#include "BoundaryOverlayWidget.h"
#include "TextGridDocument.h"
#include "IBoundaryModel.h"

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

    m_playheadHideTimer.setSingleShot(true);
    m_playheadHideTimer.setInterval(200);
    connect(&m_playheadHideTimer, &QTimer::timeout, this, [this]() {
        m_playhead = -1.0;
        update();
    });
}

void BoundaryOverlayWidget::setDocument(TextGridDocument *doc) {
    m_document = doc;
    m_boundaryModel = nullptr;
    update();
}

void BoundaryOverlayWidget::setBoundaryModel(IBoundaryModel *model) {
    m_boundaryModel = model;
    m_document = nullptr;
    update();
}

void BoundaryOverlayWidget::setTierLabelGeometry(int totalHeight, int rowHeight) {
    m_tierLabelTotalHeight = totalHeight;
    m_tierLabelRowHeight = rowHeight;
    update();
}

void BoundaryOverlayWidget::setExtraTopOffset(int pixels) {
    if (m_extraTopOffset != pixels) {
        m_extraTopOffset = pixels;
        repositionOverSplitter();
    }
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
    m_playheadHideTimer.stop();
    if (m_playhead != sec) {
        m_playhead = sec;
        update();
    }
}

void BoundaryOverlayWidget::paintEvent(QPaintEvent * /*event*/) {
    QPainter painter(this);
    int h = height();
    int w = width();

    IBoundaryModel *model = m_boundaryModel ? m_boundaryModel : m_document;
    if (!model) return;

    int tiers = model->tierCount();
    int activeTier = model->activeTierIndex();

    int tierLabelH = m_tierLabelTotalHeight;
    int tierRowH = m_tierLabelRowHeight;
    if (tiers > 1 && tierLabelH > 0)
        tierRowH = tierLabelH / tiers;

    int fullBottom = h;

    for (int t = 0; t < tiers; ++t) {
        int count = model->boundaryCount(t);
        if (count == 0) continue;

        bool isActive = (t == activeTier);

        int lineTop = t * tierRowH;
        int lineBottom;

        if (isActive) {
            // D-05: Active tier boundary lines extend through ALL charts
            lineBottom = fullBottom;
        } else {
            // D-05: Non-active tier boundary lines stay within the tier label area
            lineBottom = tiers * tierRowH;
        }

        for (int b = 0; b < count; ++b) {
            double tSec = usToSec(model->boundaryTime(t, b));
            int x = timeToX(tSec);
            if (x < 0 || x > w) continue;

            if (isActive) {
                if (b == m_draggedBoundary) {
                    painter.setPen(QPen(dsfw::Theme::instance().palette().phonemeEditor.boundaryDragged, 2));
                } else if (b == m_hoveredBoundary) {
                    painter.setPen(QPen(dsfw::Theme::instance().palette().phonemeEditor.boundaryHovered, 2));
                } else {
                    painter.setPen(QPen(dsfw::Theme::instance().palette().phonemeEditor.boundaryNormal, 1, Qt::SolidLine));
                }
            } else {
                painter.setPen(QPen(QColor(100, 100, 120, 120), 1, Qt::DashLine));
            }
            painter.drawLine(x, lineTop, x, lineBottom);
        }
    }

    if (m_playhead >= 0.0) {
        int px = timeToX(m_playhead);
        if (px >= 0 && px <= w) {
            painter.setPen(QPen(QColor(255, 80, 80), 2));
            painter.drawLine(px, 0, px, fullBottom);
        }
    }
}

bool BoundaryOverlayWidget::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_trackedWidget &&
        (event->type() == QEvent::Resize || event->type() == QEvent::Move ||
         event->type() == QEvent::Show)) {
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
    QPoint topLeft = m_trackedWidget->mapTo(parentWidget(), QPoint(0, 0));

    int top = topLeft.y();
    int left = topLeft.x();
    int widgetWidth = m_trackedWidget->width();
    int widgetHeight = m_trackedWidget->height();

    int totalTopOffset = m_tierLabelTotalHeight + m_extraTopOffset;
    if (totalTopOffset > 0) {
        top -= totalTopOffset;
        widgetHeight += totalTopOffset;
    }

    setGeometry(left, top, widgetWidth, widgetHeight);
    raise();
}

} // namespace phonemelabeler
} // namespace dstools
