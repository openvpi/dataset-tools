#include "IntervalTierView.h"
#include "commands/MoveBoundaryCommand.h"
#include "commands/SetIntervalTextCommand.h"
#include "commands/InsertBoundaryCommand.h"
#include "BoundaryBindingManager.h"

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
                                     BoundaryBindingManager *bindingMgr,
                                     QWidget *parent)
    : QWidget(parent),
    m_tierIndex(tierIndex),
    m_doc(doc),
    m_undoStack(undoStack),
    m_viewport(viewport),
    m_bindingMgr(bindingMgr)
{
    setMouseTracking(true);
    setMinimumHeight(40);
    setMaximumHeight(60);
}

void IntervalTierView::setViewport(const ViewportState &state) {
    m_viewStart = state.startSec;
    m_viewEnd = state.endSec;
    m_pixelsPerSecond = state.pixelsPerSecond;
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
    if (m_state == State::Dragging && m_bindingMgr && m_bindingMgr->isEnabled()) {
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
        if (m_state == State::Dragging && b == m_draggedBoundary) {
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
    if (m_dragAligned.empty()) return;

    painter.setPen(QPen(dsfw::Theme::instance().palette().phonemeEditor.boundaryBindingLine, 1, Qt::DashLine));

    // Draw vertical lines at aligned boundary positions in other tiers
    // The parent widget (TierEditWidget) will handle cross-tier drawing
    // by coordinating with other IntervalTierViews
}

int IntervalTierView::hitTestBoundary(int x) const {
    if (!m_doc) return -1;
    int count = m_doc->boundaryCount(m_tierIndex);
    for (int b = 0; b < count; ++b) {
        int bx = timeToX(usToSec(m_doc->boundaryTime(m_tierIndex, b)));
        if (std::abs(x - bx) <= kBoundaryHitWidth / 2) {
            return b;
        }
    }
    return -1;
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
        if (boundaryIdx >= 0) {
            startDrag(boundaryIdx, m_doc->boundaryTime(m_tierIndex, boundaryIdx));
        }
    } else if (event->button() == Qt::RightButton) {
        // Context menu
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
    if (m_state == State::Dragging) {
        TimePos currentTime = secToUs(xToTime(event->pos().x()));
        updateDrag(currentTime);
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

void IntervalTierView::startDrag(int boundaryIndex, TimePos startTime) {
    m_state = State::Dragging;
    m_draggedBoundary = boundaryIndex;
    m_dragStartTime = startTime;

    // Get aligned boundaries if binding is enabled
    if (m_bindingMgr && m_bindingMgr->isEnabled()) {
        m_dragAligned = m_bindingMgr->findAlignedBoundaries(m_tierIndex, boundaryIndex);
        m_dragAlignedStartTimes.clear();
        for (const auto &ab : m_dragAligned) {
            m_dragAlignedStartTimes.push_back(ab.time);
        }
    } else {
        m_dragAligned.clear();
        m_dragAlignedStartTimes.clear();
    }

    setCursor(Qt::SizeHorCursor);
}

void IntervalTierView::updateDrag(TimePos currentTime) {
    if (m_state != State::Dragging) return;

    // Direct update for visual feedback (not through undo stack during drag)
    m_doc->moveBoundary(m_tierIndex, m_draggedBoundary, currentTime);

    // Update aligned boundaries
    TimePos delta = currentTime - m_dragStartTime;
    for (size_t i = 0; i < m_dragAligned.size(); ++i) {
        m_doc->moveBoundary(m_dragAligned[i].tierIndex,
                            m_dragAligned[i].boundaryIndex,
                            m_dragAlignedStartTimes[i] + delta);
    }

    update();
}

void IntervalTierView::endDrag(TimePos finalTime) {
    if (m_state != State::Dragging) return;

    // Restore to start state, then create undo command
    m_doc->moveBoundary(m_tierIndex, m_draggedBoundary, m_dragStartTime);
    for (size_t i = 0; i < m_dragAligned.size(); ++i) {
        m_doc->moveBoundary(m_dragAligned[i].tierIndex,
                            m_dragAligned[i].boundaryIndex,
                            m_dragAlignedStartTimes[i]);
    }

    // Create and push undo command
    if (m_undoStack) {
        if (!m_dragAligned.empty() && m_bindingMgr) {
            // Use linked move command
            auto *cmd = m_bindingMgr->createLinkedMoveCommand(
                m_tierIndex, m_draggedBoundary, finalTime, m_doc);
            m_undoStack->push(cmd);
        } else {
            m_undoStack->push(new MoveBoundaryCommand(
                m_doc, m_tierIndex, m_draggedBoundary,
                m_dragStartTime, finalTime));
        }
    }

    m_state = State::Idle;
    m_draggedBoundary = -1;
    m_dragAligned.clear();
    m_dragAlignedStartTimes.clear();
    unsetCursor();
    update();
}

void IntervalTierView::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && m_state == State::Dragging) {
        TimePos finalTime = secToUs(xToTime(event->pos().x()));
        endDrag(finalTime);
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
