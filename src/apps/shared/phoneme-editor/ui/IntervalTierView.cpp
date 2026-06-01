#include "IntervalTierView.h"
#include "commands/BoundaryCommands.h"
#include "BoundaryDragController.h"

#include <QPainter>
#include <cmath>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QLineEdit>
#include <QEvent>
#include <QInputDialog>
#include <QDebug>

#include <dsfw/Theme.h>

using dstools::chart::MoveBoundaryCommand;

namespace dstools {
namespace phonemelabeler {

IntervalTierView::IntervalTierView(int tierIndex, TextGridDocument *doc,
                                     QUndoStack *undoStack, ViewportController *viewport,
                                     BoundaryDragController *dragController,
                                     QWidget *parent)
    : QWidget(parent),
    m_tierIndex(tierIndex),
    m_doc(doc),
    m_undoStack(undoStack),
    m_viewport(viewport),
    m_dragController(dragController)
{
    setMouseTracking(true);
    setFixedHeight(24);
}

void IntervalTierView::setViewport(const ViewportState &state) {
    m_viewStart = state.startSec;
    m_viewEnd = state.endSec;
    update();
}

void IntervalTierView::setActive(bool active) {
    if (m_active != active) {
        m_active = active;
        update();
    }
}

void IntervalTierView::paintEvent(QPaintEvent * /*event*/) {
    QPainter painter(this);
    const auto &pe = dsfw::Theme::instance().palette().phonemeEditor;
    painter.fillRect(rect(), m_active ? pe.tierBackground : pe.tierBackgroundInactive);

    if (!m_doc || m_tierIndex < 0 || m_tierIndex >= m_doc->tierCount()) {
        painter.setPen(Qt::gray);
        painter.drawText(rect(), Qt::AlignCenter, tr("No tier data"));
        return;
    }

    drawIntervals(painter);
    drawLabels(painter);
    drawSelection(painter);

    // Draw binding lines if dragging with binding enabled
    if (m_dragController && m_dragController->isDragging() && m_dragController->isBindingEnabled()) {
        drawBindingLines(painter);
    }
}

void IntervalTierView::drawIntervals(QPainter &painter) {
    if (!m_doc) return;
    int count = m_doc->intervalCount(m_tierIndex);
    for (int i = 0; i < count; ++i) {
        double xmin = usToSec(m_doc->intervalStart(m_tierIndex, i));
        double xmax = usToSec(m_doc->intervalEnd(m_tierIndex, i));

        int x1 = static_cast<int>(timeToX(xmin));
        int x2 = static_cast<int>(timeToX(xmax));

        if (x2 < 0 || x1 > width()) continue;

        // Alternating colors for readability
        QColor color = (i % 2 == 0) ? dsfw::Theme::instance().palette().phonemeEditor.intervalFillA : dsfw::Theme::instance().palette().phonemeEditor.intervalFillB;
        painter.fillRect(QRect(x1, 0, x2 - x1, height()), color);

        painter.setPen(dsfw::Theme::instance().palette().phonemeEditor.intervalBorder);
        painter.drawRect(x1, 0, x2 - x1, height());
    }
}

void IntervalTierView::drawLabels(QPainter &painter) {
    if (!m_doc) return;
    painter.setPen(dsfw::Theme::instance().palette().phonemeEditor.labelText);
    int count = m_doc->intervalCount(m_tierIndex);
    for (int i = 0; i < count; ++i) {
        double xmin = usToSec(m_doc->intervalStart(m_tierIndex, i));
        double xmax = usToSec(m_doc->intervalEnd(m_tierIndex, i));
        int x1 = static_cast<int>(timeToX(xmin));
        int x2 = static_cast<int>(timeToX(xmax));

        if (x2 < 0 || x1 > width()) continue;

        QRect rect(x1 + 2, 2, x2 - x1 - 4, height() - 4);
        if (rect.width() < 20) continue; // too narrow

        QString text = m_doc->intervalText(m_tierIndex, i);
        painter.drawText(rect, Qt::AlignVCenter, text);
    }
}

void IntervalTierView::drawBindingLines(QPainter &painter) {
    if (!m_dragController || !m_doc)
        return;

    if (m_dragController->partnerCount() == 0)
        return;

    TimePos sourceTime = m_doc->boundaryTime(
        m_dragController->draggedTier(), m_dragController->draggedBoundary());
    int sourceX = static_cast<int>(timeToX(usToSec(sourceTime)));

    QPen linePen(QColor(255, 180, 60, 200), 1, Qt::DashLine);
    painter.setPen(linePen);

    painter.drawLine(sourceX, 0, sourceX, height());

    painter.setPen(QColor(255, 180, 60, 120));
    painter.drawLine(sourceX - 4, height() / 2, sourceX - 1, height() / 2);
    painter.drawLine(sourceX + 4, height() / 2, sourceX + 1, height() / 2);

    painter.setPen(QPen(QColor(255, 180, 60, 80), 1));
    painter.drawLine(sourceX, 0, sourceX, height());
}

int IntervalTierView::hitTestBoundary(int x) const {
    if (!m_doc) return -1;
    int count = m_doc->boundaryCount(m_tierIndex);

    struct Hit {
        int boundary;
        int dist;
        int dragRoom;
    };
    QVector<Hit> hits;

    for (int b = 0; b < count; ++b) {
        int bx = static_cast<int>(timeToX(usToSec(m_doc->boundaryTime(m_tierIndex, b))));
        int dist = std::abs(x - bx);
        if (dist <= kBoundaryHitWidth / 2) {
            TimePos pos = m_doc->boundaryTime(m_tierIndex, b);
            TimePos leftClamp = (b > 0) ? m_doc->boundaryTime(m_tierIndex, b - 1) : 0;
            TimePos rightClamp = (b + 1 < count) ? m_doc->boundaryTime(m_tierIndex, b + 1) : INT64_MAX;
            int room = static_cast<int>(std::min(pos - leftClamp, rightClamp - pos));

            hits.push_back({b, dist, room});
        }
    }

    if (hits.isEmpty())
        return -1;

    int bestIdx = 0;
    for (int i = 1; i < hits.size(); ++i) {
        const auto &cur = hits[i];
        const auto &best = hits[bestIdx];

        if (cur.dist != best.dist) {
            if (cur.dist < best.dist) bestIdx = i;
            continue;
        }

        if (cur.dragRoom != best.dragRoom) {
            if (cur.dragRoom > best.dragRoom) bestIdx = i;
            continue;
        }

        if (cur.boundary < best.boundary) bestIdx = i;
    }

    return hits[bestIdx].boundary;
}

double IntervalTierView::timeToX(double time) const {
    if (m_converter)
        return m_converter->timeToX(time, width());
    return 0.0;
}

double IntervalTierView::xToTime(int x) const {
    if (m_converter)
        return m_converter->xToTime(x, width());
    return 0.0;
}

void IntervalTierView::mousePressEvent(QMouseEvent *event) {
    // Activation only via radio buttons in TierEditWidget.
    // Clicking on a boundary starts drag without switching activation layer.
    if (event->button() == Qt::LeftButton) {
        int boundaryIdx = hitTestBoundary(event->pos().x());
        if (boundaryIdx >= 0 && m_dragController) {
            m_dragController->startDrag(m_tierIndex, boundaryIdx, m_doc);
            setCursor(Qt::SizeHorCursor);
        }
    } else if (event->button() == Qt::RightButton) {
        int intervalIndex = hitTestInterval(event->pos().x());
        if (intervalIndex >= 0 && m_doc) {
            TimePos startTime = m_doc->intervalStart(m_tierIndex, intervalIndex);
            TimePos endTime = m_doc->intervalEnd(m_tierIndex, intervalIndex);
            emit requestPlayback(startTime, endTime);
        }
    }
    QWidget::mousePressEvent(event);
}

void IntervalTierView::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragController && m_dragController->isDragging()) {
        TimePos currentTime = secToUs(xToTime(event->pos().x()));
        m_dragController->updateDrag(currentTime);
        update();
    } else {
        // Hover detection
        int boundaryIdx = hitTestBoundary(event->pos().x());
        if (boundaryIdx != m_hoveredBoundary) {
            m_hoveredBoundary = boundaryIdx;
            emit hoveredBoundaryChanged(boundaryIdx);
            if (boundaryIdx >= 0) {
                setCursor(Qt::SizeHorCursor);
            } else {
                unsetCursor();
            }
            update();
        }
    }
    QWidget::mouseMoveEvent(event);
}

void IntervalTierView::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        if (m_doc && m_doc->isTierReadOnly(m_tierIndex))
            return;

        double clickTime = xToTime(event->pos().x());

        // Find which interval was clicked
        int count = m_doc->intervalCount(m_tierIndex);
        for (int i = 0; i < count; ++i) {
            double xmin = usToSec(m_doc->intervalStart(m_tierIndex, i));
            double xmax = usToSec(m_doc->intervalEnd(m_tierIndex, i));
            if (clickTime >= xmin && clickTime < xmax) {
                // Edit interval text
                bool ok;
                QString currentText = m_doc->intervalText(m_tierIndex, i);
                QString newText = QInputDialog::getText(this, tr("Edit Interval Text"),
                                                      tr("Text:"), QLineEdit::Normal,
                                                      currentText, &ok);
                if (ok && newText != currentText && m_undoStack) {
                    m_undoStack->push(new SetIntervalTextCommand(
                        m_doc, m_tierIndex, i, currentText, newText));
                }
                break;
            }
        }
    }
}

void IntervalTierView::leaveEvent(QEvent *event) {
    if (m_hoveredBoundary >= 0) {
        m_hoveredBoundary = -1;
        emit hoveredBoundaryChanged(-1);
        unsetCursor();
        update();
    }
    QWidget::leaveEvent(event);
}

void IntervalTierView::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && m_dragController && m_dragController->isDragging()) {
        TimePos finalTime = secToUs(xToTime(event->pos().x()));
        m_dragController->endDrag(finalTime, m_undoStack);
        unsetCursor();
        update();
    }
    QWidget::mouseReleaseEvent(event);
}

int IntervalTierView::hitTestInterval(int x) const {
    if (!m_doc) return -1;
    int count = m_doc->intervalCount(m_tierIndex);
    for (int i = 0; i < count; ++i) {
        int x1 = static_cast<int>(timeToX(usToSec(m_doc->intervalStart(m_tierIndex, i))));
        int x2 = static_cast<int>(timeToX(usToSec(m_doc->intervalEnd(m_tierIndex, i))));
        if (x2 - x1 < 1) x2 = x1 + 1;
        if (x >= x1 && x < x2) {
            return i;
        }
    }
    return -1;
}

void IntervalTierView::drawSelection(QPainter &painter) {
    if (m_selectedInterval < 0 || !m_doc) return;
    if (m_selectedInterval >= m_doc->intervalCount(m_tierIndex)) return;

    int x1 = static_cast<int>(timeToX(usToSec(m_doc->intervalStart(m_tierIndex, m_selectedInterval))));
    int x2 = static_cast<int>(timeToX(usToSec(m_doc->intervalEnd(m_tierIndex, m_selectedInterval))));

    painter.setPen(QPen(dsfw::Theme::instance().palette().phonemeEditor.selectionBorder, 2));
    painter.drawRect(x1, 0, x2 - x1, height());
}

void IntervalTierView::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
    case Qt::Key_Tab:
        if (event->modifiers() & Qt::ShiftModifier)
            selectPrevInterval();
        else
            selectNextInterval();
        event->accept();
        return;
    case Qt::Key_Left:
        if (m_selectedInterval >= 0 && m_hoveredBoundary >= 0)
            adjustSelectedBoundary(-1000);
        else
            selectPrevInterval();
        event->accept();
        return;
    case Qt::Key_Right:
        if (m_selectedInterval >= 0 && m_hoveredBoundary >= 0)
            adjustSelectedBoundary(1000);
        else
            selectNextInterval();
        event->accept();
        return;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void IntervalTierView::selectNextInterval() {
    if (!m_doc) return;
    int count = m_doc->intervalCount(m_tierIndex);
    if (count == 0) return;

    m_selectedInterval = (m_selectedInterval < count - 1) ? m_selectedInterval + 1 : 0;
    update();
}

void IntervalTierView::selectPrevInterval() {
    if (!m_doc) return;
    int count = m_doc->intervalCount(m_tierIndex);
    if (count == 0) return;

    m_selectedInterval = (m_selectedInterval > 0) ? m_selectedInterval - 1 : count - 1;
    update();
}

void IntervalTierView::adjustSelectedBoundary(TimePos deltaUs) {
    if (!m_doc || m_hoveredBoundary < 0) return;

    TimePos oldTime = m_doc->boundaryTime(m_tierIndex, m_hoveredBoundary);
    TimePos newTime = oldTime + deltaUs;

    if (m_undoStack) {
        m_undoStack->push(new MoveBoundaryCommand(
            m_doc, m_tierIndex, m_hoveredBoundary, oldTime, newTime));
    }
}

} // namespace phonemelabeler
} // namespace dstools
