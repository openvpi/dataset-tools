#include "IntervalTierView.h"
#include "commands/BoundaryCommands.h"
#include "BoundaryDragController.h"

#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QLineEdit>
#include <QEvent>
#include <QInputDialog>
#include <QMenu>
#include <QDebug>

#include <dsfw/Theme.h>

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
    setMinimumHeight(40);
    setMaximumHeight(60);
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
    drawBoundaries(painter);
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

        int x1 = timeToX(xmin);
        int x2 = timeToX(xmax);

        if (x2 < 0 || x1 > width()) continue;

        // Alternating colors for readability
        QColor color = (i % 2 == 0) ? dsfw::Theme::instance().palette().phonemeEditor.intervalFillA : dsfw::Theme::instance().palette().phonemeEditor.intervalFillB;
        painter.fillRect(QRect(x1, 0, x2 - x1, height()), color);

        // Border
        painter.setPen(dsfw::Theme::instance().palette().phonemeEditor.intervalBorder);
        painter.drawRect(x1, 0, x2 - x1 - 1, height() - 1);
    }
}

void IntervalTierView::drawBoundaries(QPainter &painter) {
    if (!m_doc) return;
    int count = m_doc->boundaryCount(m_tierIndex);
    for (int b = 0; b < count; ++b) {
        int bx = timeToX(usToSec(m_doc->boundaryTime(m_tierIndex, b)));

        // Choose pen based on state
        if (m_dragController && m_dragController->isDragging()
            && m_dragController->draggedTier() == m_tierIndex
            && b == m_dragController->draggedBoundary()) {
            painter.setPen(QPen(dsfw::Theme::instance().palette().phonemeEditor.boundaryDragged, 2));
        } else if (b == m_hoveredBoundary && m_hoveredBoundary >= 0) {
            painter.setPen(QPen(dsfw::Theme::instance().palette().phonemeEditor.boundaryHovered, 2));
        } else {
            painter.setPen(QPen(dsfw::Theme::instance().palette().phonemeEditor.boundaryNormal, 1));
        }

        painter.drawLine(bx, 0, bx, height());
    }
}

void IntervalTierView::drawLabels(QPainter &painter) {
    if (!m_doc) return;
    painter.setPen(dsfw::Theme::instance().palette().phonemeEditor.labelText);
    int count = m_doc->intervalCount(m_tierIndex);
    for (int i = 0; i < count; ++i) {
        double xmin = usToSec(m_doc->intervalStart(m_tierIndex, i));
        double xmax = usToSec(m_doc->intervalEnd(m_tierIndex, i));
        int x1 = timeToX(xmin);
        int x2 = timeToX(xmax);

        if (x2 < 0 || x1 > width()) continue;

        QRect rect(x1 + 2, 2, x2 - x1 - 4, height() - 4);
        if (rect.width() < 20) continue; // too narrow

        QString text = m_doc->intervalText(m_tierIndex, i);
        painter.drawText(rect, Qt::AlignVCenter, text);
    }
}

void IntervalTierView::drawBindingLines(QPainter &painter) {
    Q_UNUSED(painter)
    // TODO: draw cross-tier binding indicator lines during drag
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
        int bx = timeToX(usToSec(m_doc->boundaryTime(m_tierIndex, b)));
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

int IntervalTierView::timeToX(double time) const {
    double viewDuration = m_viewEnd - m_viewStart;
    if (viewDuration <= 0.0 || width() <= 0) return 0;
    return static_cast<int>((time - m_viewStart) / viewDuration * width());
}

double IntervalTierView::xToTime(int x) const {
    double viewDuration = m_viewEnd - m_viewStart;
    if (viewDuration <= 0.0 || width() <= 0) return m_viewStart;
    return m_viewStart + viewDuration * x / width();
}

void IntervalTierView::mousePressEvent(QMouseEvent *event) {
    // Activate this tier on any click
    emit activated(m_tierIndex);

    if (event->button() == Qt::LeftButton) {
        int boundaryIdx = hitTestBoundary(event->pos().x());
        if (boundaryIdx >= 0 && m_dragController) {
            m_dragController->startDrag(m_tierIndex, boundaryIdx, m_doc);
            setCursor(Qt::SizeHorCursor);
        }
    } else if (event->button() == Qt::RightButton) {
        if (m_doc && m_doc->isTierReadOnly(m_tierIndex)) {
            QWidget::mousePressEvent(event);
            return;
        }

        QMenu menu(this);
        int boundaryIdx = hitTestBoundary(event->pos().x());

        if (boundaryIdx >= 0) {
            // Insert boundary before this one
            QAction *insertAction = menu.addAction(tr("Insert Boundary Here"));
            connect(insertAction, &QAction::triggered, [this, boundaryIdx]() {
                TimePos time = m_doc->boundaryTime(m_tierIndex, boundaryIdx);
                if (m_undoStack) {
                    m_undoStack->push(new InsertBoundaryCommand(m_doc, m_tierIndex, time));
                }
            });
        }

        TimePos clickTime = secToUs(xToTime(event->pos().x()));
        QAction *insertAtTime = menu.addAction(tr("Insert Boundary at %1s").arg(usToSec(clickTime), 0, 'f', 3));
        connect(insertAtTime, &QAction::triggered, [this, clickTime]() {
            if (m_undoStack) {
                m_undoStack->push(new InsertBoundaryCommand(m_doc, m_tierIndex, clickTime));
            }
        });

        menu.exec(event->globalPosition().toPoint());
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
        int x1 = timeToX(usToSec(m_doc->intervalStart(m_tierIndex, i)));
        int x2 = timeToX(usToSec(m_doc->intervalEnd(m_tierIndex, i)));
        if (x >= x1 && x < x2) {
            return i;
        }
    }
    return -1;
}

void IntervalTierView::drawSelection(QPainter &painter) {
    if (m_selectedInterval < 0 || !m_doc) return;
    if (m_selectedInterval >= m_doc->intervalCount(m_tierIndex)) return;

    int x1 = timeToX(usToSec(m_doc->intervalStart(m_tierIndex, m_selectedInterval)));
    int x2 = timeToX(usToSec(m_doc->intervalEnd(m_tierIndex, m_selectedInterval)));

    painter.setPen(QPen(dsfw::Theme::instance().palette().phonemeEditor.selectionBorder, 2));
    painter.drawRect(x1, 0, x2 - x1 - 1, height() - 1);
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
