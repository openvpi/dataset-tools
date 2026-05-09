#include "AudioChartWidget.h"
#include "BoundaryDragController.h"

#include <dstools/TimePos.h>

#include <QContextMenuEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QUndoStack>
#include <QWheelEvent>

namespace dstools {
    namespace phonemelabeler {

        AudioChartWidget::AudioChartWidget(ViewportController *viewport, QWidget *parent)
            : QWidget(parent), m_viewport(viewport) {
            setMouseTracking(true);
        }

        AudioChartWidget::~AudioChartWidget() = default;

        void AudioChartWidget::setBoundaryModel(IBoundaryModel *model) {
            m_boundaryModel = model;
        }

        void AudioChartWidget::setDragController(BoundaryDragController *ctrl) {
            m_dragController = ctrl;
        }

        void AudioChartWidget::setUndoStack(QUndoStack *stack) {
            m_undoStack = stack;
        }

        void AudioChartWidget::setPlayWidget(widgets::PlayWidget *pw) {
            m_playWidget = pw;
        }

        void AudioChartWidget::setViewport(const ViewportState &state) {
            m_viewStart = state.startSec;
            m_viewEnd = state.endSec;
            rebuildCache();
            update();
        }

        void AudioChartWidget::updateBoundaryOverlay() {
            update();
        }

        void AudioChartWidget::onViewportDragStart(double timeSec) {
            Q_UNUSED(timeSec)
        }

        void AudioChartWidget::onVerticalZoom(double factor) {
            Q_UNUSED(factor)
        }

        double AudioChartWidget::xToTime(int x) const {
            double viewDuration = m_viewEnd - m_viewStart;
            if (viewDuration <= 0.0 || width() <= 0)
                return m_viewStart;
            return m_viewStart + viewDuration * x / width();
        }

        int AudioChartWidget::timeToX(double time) const {
            double viewDuration = m_viewEnd - m_viewStart;
            if (viewDuration <= 0.0 || width() <= 0)
                return 0;
            return static_cast<int>((time - m_viewStart) / viewDuration * width());
        }

        void AudioChartWidget::mousePressEvent(QMouseEvent *event) {
            if (event->button() == Qt::LeftButton) {
                int hitTier = -1;
                int boundaryIdx = hitTestBoundary(event->pos().x(), &hitTier);
                if (boundaryIdx >= 0 && hitTier >= 0 && m_boundaryModel && m_dragController) {
                    m_dragController->startDrag(hitTier, boundaryIdx, m_boundaryModel);
                    setCursor(Qt::SizeHorCursor);
                } else {
                    m_dragging = true;
                    m_dragStartPos = event->pos();
                    m_dragStartTime = xToTime(event->pos().x());
                    onViewportDragStart(m_dragStartTime);
                }
            }
            QWidget::mousePressEvent(event);
        }

        void AudioChartWidget::mouseMoveEvent(QMouseEvent *event) {
            if (m_dragController && m_dragController->isDragging()) {
                TimePos currentTime = secToUs(xToTime(event->pos().x()));
                m_dragController->updateDrag(currentTime);
            } else if (m_dragging) {
                double deltaSec = xToTime(event->pos().x()) - m_dragStartTime;
                if (m_viewport) {
                    m_viewport->scrollBy(-deltaSec);
                }
            } else {
                int boundaryIdx = hitTestBoundary(event->pos().x());
                if (boundaryIdx != m_hoveredBoundary) {
                    m_hoveredBoundary = boundaryIdx;
                    setCursor(boundaryIdx >= 0 ? Qt::SizeHorCursor : Qt::ArrowCursor);
                    emit hoveredBoundaryChanged(boundaryIdx);
                    update();
                }
            }
            QWidget::mouseMoveEvent(event);
        }

        void AudioChartWidget::mouseReleaseEvent(QMouseEvent *event) {
            if (event->button() == Qt::LeftButton) {
                if (m_dragController && m_dragController->isDragging()) {
                    TimePos finalTime = secToUs(xToTime(event->pos().x()));
                    m_dragController->endDrag(finalTime, m_undoStack);
                    unsetCursor();
                }
                m_dragging = false;
            }
            QWidget::mouseReleaseEvent(event);
        }

        void AudioChartWidget::wheelEvent(QWheelEvent *event) {
            if (event->modifiers() & Qt::ControlModifier) {
                double delta = event->angleDelta().y() / 120.0;
                double factor = (delta > 0) ? 1.25 : 0.8;
                if (m_viewport) {
                    m_viewport->zoomAt(xToTime(static_cast<int>(event->position().x())), factor);
                }
                event->accept();
            } else if (event->modifiers() & Qt::ShiftModifier) {
                double delta = event->angleDelta().y() / 120.0;
                double factor = (delta > 0) ? 1.25 : 0.8;
                onVerticalZoom(factor);
                event->accept();
            } else {
                if (m_viewport) {
                    double scrollSec = (event->angleDelta().y() > 0) ? -0.5 : 0.5;
                    m_viewport->scrollBy(scrollSec);
                }
                event->accept();
            }
        }

        void AudioChartWidget::contextMenuEvent(QContextMenuEvent *event) {
            if (!m_playWidget) {
                event->accept();
                return;
            }

            double clickTime = xToTime(event->pos().x());

            double segStart, segEnd;
            findSurroundingBoundaries(clickTime, segStart, segEnd);

            if (m_playWidget) {
                m_playWidget->setPlayRange(segStart, segEnd);
                m_playWidget->seek(segStart);
                m_playWidget->setPlaying(true);
            }

            event->accept();
        }

        int AudioChartWidget::hitTestBoundary(int x, int *outTier) const {
            if (!m_boundaryModel)
                return -1;

            struct Hit {
                int tier;
                int boundary;
                int dist;
                int dragRoom;
            };
            QVector<Hit> hits;

            int tierCount = m_boundaryModel->tierCount();
            int activeTier = m_boundaryModel->activeTierIndex();

            for (int t = 0; t < tierCount; ++t) {
                int count = m_boundaryModel->boundaryCount(t);
                for (int b = 0; b < count; ++b) {
                    int bx = timeToX(usToSec(m_boundaryModel->boundaryTime(t, b)));
                    int dist = std::abs(x - bx);
                    if (dist <= kBoundaryHitWidth / 2) {
                        TimePos pos = m_boundaryModel->boundaryTime(t, b);
                        TimePos leftClamp = (b > 0) ? m_boundaryModel->boundaryTime(t, b - 1) : 0;
                        TimePos rightClamp = (b + 1 < count) ? m_boundaryModel->boundaryTime(t, b + 1) : INT64_MAX;
                        int room = static_cast<int>(std::min(pos - leftClamp, rightClamp - pos));

                        hits.push_back({t, b, dist, room});
                    }
                }
            }

            if (hits.isEmpty()) {
                if (outTier)
                    *outTier = -1;
                return -1;
            }

            int bestIdx = 0;
            for (int i = 1; i < hits.size(); ++i) {
                const auto &cur = hits[i];
                const auto &best = hits[bestIdx];

                bool curActive = (cur.tier == activeTier);
                bool bestActive = (best.tier == activeTier);

                if (curActive != bestActive) {
                    if (curActive)
                        bestIdx = i;
                    continue;
                }

                if (cur.dist != best.dist) {
                    if (cur.dist < best.dist)
                        bestIdx = i;
                    continue;
                }

                if (cur.dragRoom != best.dragRoom) {
                    if (cur.dragRoom > best.dragRoom)
                        bestIdx = i;
                    continue;
                }

                if (cur.boundary < best.boundary)
                    bestIdx = i;
            }

            if (outTier)
                *outTier = hits[bestIdx].tier;
            return hits[bestIdx].boundary;
        }

        void AudioChartWidget::drawBoundaryOverlay(QPainter &painter) {
            if (!m_boundaryModel)
                return;

            int tierCount = m_boundaryModel->tierCount();
            int activeTier = m_boundaryModel->activeTierIndex();

            for (int t = 0; t < tierCount; ++t) {
                bool isActive = (t == activeTier);
                int count = m_boundaryModel->boundaryCount(t);
                for (int b = 0; b < count; ++b) {
                    int bx = timeToX(usToSec(m_boundaryModel->boundaryTime(t, b)));
                    if (bx < 0 || bx > width())
                        continue;

                    QColor color = isActive ? QColor(255, 165, 0, 200) : QColor(128, 128, 128, 100);
                    if (b == m_hoveredBoundary && isActive) {
                        color = QColor(255, 255, 0, 255);
                    }

                    painter.setPen(QPen(color, isActive ? 2 : 1));
                    painter.drawLine(bx, 0, bx, height());
                }
            }
        }

        void AudioChartWidget::findSurroundingBoundaries(double timeSec, double &outStart, double &outEnd) const {
            outStart = m_viewStart;
            outEnd = m_viewEnd;

            if (!m_boundaryModel)
                return;

            int activeTier = m_boundaryModel->activeTierIndex();
            if (activeTier < 0 || activeTier >= m_boundaryModel->tierCount())
                return;

            int count = m_boundaryModel->boundaryCount(activeTier);
            for (int b = 0; b < count; ++b) {
                double t = usToSec(m_boundaryModel->boundaryTime(activeTier, b));
                if (t <= timeSec) {
                    outStart = t;
                }
                if (t > timeSec) {
                    outEnd = t;
                    break;
                }
            }
        }

    } // namespace phonemelabeler
} // namespace dstools
