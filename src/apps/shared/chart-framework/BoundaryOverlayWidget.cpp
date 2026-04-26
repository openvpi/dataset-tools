#include "BoundaryOverlayWidget.h"
#include "IBoundaryModel.h"

#include <dstools/TimePos.h>

#include <algorithm>
#include <cmath>
#include <QPainter>
#include <QPen>
#include <QEvent>
#include <QResizeEvent>

#include <dsfw/Theme.h>

namespace dstools {
namespace chart {

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

void BoundaryOverlayWidget::setBoundaryModel(IBoundaryModel *model) {
    m_boundaryModel = model;
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

void BoundaryOverlayWidget::updateLabelAreaHeight(int totalHeight) {
    if (totalHeight <= 0)
        return;
    if (m_tierLabelTotalHeight != totalHeight) {
        m_tierLabelTotalHeight = totalHeight;
        update();
        repositionOverSplitter();
    }
}

void BoundaryOverlayWidget::forceReposition() {
    repositionOverSplitter();
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

    IBoundaryModel *model = m_boundaryModel;
    if (!model) return;

    int tiers = model->tierCount();
    int activeTier = model->activeTierIndex();

    int tierLabelH = m_tierLabelTotalHeight;
    int tierRowH = m_tierLabelRowHeight;
    if (tiers > 1 && tierLabelH > 0)
        tierRowH = tierLabelH / tiers;

    bool hasLabelArea = (tierLabelH > 0);
    int fullBottom = h;

    for (int p = 0; p < 2; ++p) {
        for (int t = 0; t < tiers; ++t) {
            bool isActive = (t == activeTier);
            if (p == 0 && isActive) continue;
            if (p == 1 && !isActive) continue;

            int count = model->boundaryCount(t);

            int lineTop = t * tierRowH;
            int lineBottom;

            if (isActive) {
                lineBottom = fullBottom;
            } else if (!hasLabelArea) {
                lineBottom = fullBottom;
            } else {
                lineBottom = std::min(tiers * tierRowH, tierLabelH);
            }

            for (int b = 0; b < count; ++b) {
                double tSec = usToSec(model->boundaryTime(t, b));
                int x = static_cast<int>(timeToX(tSec));
                if (x < 0 || x > w) continue;

                bool skip = false;
                for (const auto &range : m_excludedYRanges) {
                    if (lineTop >= range.first && lineBottom <= range.second) {
                        skip = true;
                        break;
                    }
                }
                if (skip) continue;

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
    }

    if (m_playhead >= 0.0) {
        int px = std::clamp(static_cast<int>(timeToX(m_playhead)), 0, w);
        painter.setPen(QPen(QColor(255, 80, 80), 2));
        painter.drawLine(px, 0, px, fullBottom);
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

double BoundaryOverlayWidget::timeToX(double time) const {
    if (m_converter)
        return m_converter->timeToX(time, width());
    return 0.0;
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

} // namespace chart
} // namespace dstools