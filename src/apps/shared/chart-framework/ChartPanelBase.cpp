#include "ChartPanelBase.h"

#include "BoundaryDragController.h"
#include "IBoundaryModel.h"
#include "YAxisLabelWidget.h"

#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QUndoStack>

#include <algorithm>
#include <QWheelEvent>
#include <cmath>
#include <dsfw/Theme.h>
#include <limits>

namespace dstools {
    namespace dstools {

        ChartPanelBase::ChartPanelBase(const QString &id, ViewportController *viewport, QWidget *parent) :
            QWidget(parent), m_id(id), m_viewport(viewport) {
            setMouseTracking(true);
        }

        void ChartPanelBase::setAudioData(const std::vector<float> &samples, int sampleRate) {
            m_samples = samples;
            m_sampleRate = sampleRate;
            onAudioDataChanged();
            m_cacheDirty = true;
            update();
        }

        void ChartPanelBase::onAudioDataChanged() {
        }

        void ChartPanelBase::drawEmptyState(QPainter &painter, const QString &msg) {
            painter.setPen(Qt::gray);
            painter.drawText(rect(), Qt::AlignCenter, msg);
        }

        void ChartPanelBase::onViewportUpdate(const ChartCoordinate &conv, int pixelWidth) {
            m_converter = &conv;
            m_dataPixelWidth = pixelWidth;
            m_cacheDirty = true;
            update();
        }

        QWidget *ChartPanelBase::createYAxisLabelWidget(QWidget *parent) {
            auto *self = this;
            return new YAxisLabelWidget(
                0, height(), [self](QPainter &painter, const QRect &rect) { self->paintYAxisContent(painter, rect); },
                parent);
        }

        void ChartPanelBase::onPlayheadUpdate(const PlayheadState &state) {
            Q_UNUSED(state)
        }

        void ChartPanelBase::onActiveTierChanged(int tierIndex) {
            Q_UNUSED(tierIndex)
            update();
        }

        void ChartPanelBase::paintYAxisTicks(QPainter &painter, const QRect &chartRect, double minValue,
                                              double maxValue, int tickCount, int decimals, const QString &suffix) const {
            QFont font = painter.font();
            font.setPixelSize(9);
            painter.setFont(font);

            for (int i = 0; i <= tickCount; ++i) {
                double val = minValue + (maxValue - minValue) * static_cast<double>(i) / tickCount;
                double ratio = (val - minValue) / (maxValue - minValue);
                int y = chartRect.bottom() - static_cast<int>(ratio * chartRect.height());

                painter.setPen(QColor(140, 140, 140));
                QString label = QString::number(val, 'f', decimals);
                if (!suffix.isEmpty())
                    label += suffix;
                int textTop = std::clamp(y - 8, 0, chartRect.height() - 16);
                painter.drawText(QRect(0, textTop, chartRect.width() - 4, 16), Qt::AlignRight | Qt::AlignVCenter, label);
            }
        }

        void ChartPanelBase::paintOutOfBoundsOverlay(QPainter &painter, const QRect &clipRect,
                                                      const ChartCoordinate &coord) const {
            if (!m_boundaryModel || coord.pixelWidth <= 0)
                return;

            int activeTier = m_boundaryModel->activeTierIndex();
            if (activeTier < 0)
                return;

            auto ranges = m_boundaryModel->getOutOfBoundsRanges(activeTier);
            if (ranges.empty())
                return;

            double viewDuration = coord.viewEnd - coord.viewStart;
            if (viewDuration <= 0)
                return;

            painter.save();
            painter.setClipRect(clipRect);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(128, 128, 128, 100));

            for (const auto &range : ranges) {
                double clampedStart = std::max(range.startSec, coord.viewStart);
                double clampedEnd = std::min(range.endSec, coord.viewEnd);
                if (clampedEnd <= clampedStart)
                    continue;

                int x1 = clipRect.x() + static_cast<int>((clampedStart - coord.viewStart) / viewDuration * coord.pixelWidth);
                int x2 = clipRect.x() + static_cast<int>((clampedEnd - coord.viewStart) / viewDuration * coord.pixelWidth);
                painter.drawRect(QRect(x1, clipRect.y(), x2 - x1, clipRect.height()));
            }

            painter.restore();
        }

        std::pair<int, int> ChartPanelBase::hitTestBoundary(int x) const {
            if (!m_boundaryModel)
                return {-1, -1};

            double clickTime = clientXToTime(x);
            double halfWidthTime = clientXToTime(x + kBoundaryHitWidth / 2) - clientXToTime(x - kBoundaryHitWidth / 2);
            double pixelRange = std::abs(halfWidthTime);

            int bestTier = -1, bestIdx = -1;
            double bestDist = std::numeric_limits<double>::max();

            int tierCount = m_boundaryModel->tierCount();
            for (int t = 0; t < tierCount; ++t) {
                int count = m_boundaryModel->boundaryCount(t);
                for (int b = 0; b < count; ++b) {
                    double bTime = usToSec(m_boundaryModel->boundaryTime(t, b));
                    double dist = std::abs(bTime - clickTime);
                    if (dist < pixelRange / 2.0) {
                        if (dist < bestDist || (dist == bestDist && (t > bestTier || b > bestIdx))) {
                            bestDist = dist;
                            bestTier = t;
                            bestIdx = b;
                        }
                    }
                }
            }

            return {bestTier, bestIdx};
        }

        std::pair<double, double> ChartPanelBase::findSurrounding(double timeSec) const {
            double outStart = m_converter ? m_converter->startSec() : 0.0;
            double outEnd = m_converter ? m_converter->endSec() : 10.0;

            if (!m_boundaryModel)
                return {outStart, outEnd};

            int activeTier = m_boundaryModel->activeTierIndex();
            if (activeTier < 0 || activeTier >= m_boundaryModel->tierCount())
                return {outStart, outEnd};

            int count = m_boundaryModel->boundaryCount(activeTier);
            for (int b = 0; b < count; ++b) {
                double t = usToSec(m_boundaryModel->boundaryTime(activeTier, b));
                if (t <= timeSec)
                    outStart = t;
                if (t > timeSec) {
                    outEnd = t;
                    break;
                }
            }

            return {outStart, outEnd};
        }

        void ChartPanelBase::paintEvent(QPaintEvent *) {
            if (m_cacheDirty) {
                rebuildCache(m_pendingRegion);
                m_cacheDirty = false;
            }

            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing, false);

            auto cr = chartContentRect();
            if (cr.width() <= 0 && cr.height() <= 0)
                return;

            painter.save();
            painter.setClipRect(cr);
            ChartCoordinate coord;
            if (m_converter) {
                coord.viewStart = m_converter->startSec();
                coord.viewEnd = m_converter->endSec();
                coord.pixelWidth = m_dataPixelWidth;
            }
            drawContent(painter, coord);
            paintOutOfBoundsOverlay(painter, cr, coord);
            painter.restore();

            QRect axisRect(0, kYAxisPadding, yAxisWidth(), height() - 2 * kYAxisPadding);
            if (axisRect.width() > 0) {
                painter.save();
                painter.setClipRect(axisRect);
                paintYAxisContent(painter, axisRect);
                painter.restore();
            }
        }

        void ChartPanelBase::resizeEvent(QResizeEvent *event) {
            m_dataPixelWidth = width();
            m_cacheDirty = true;
            QWidget::resizeEvent(event);
        }

        void ChartPanelBase::mousePressEvent(QMouseEvent *event) {
            if (event->button() != Qt::LeftButton) {
                QWidget::mousePressEvent(event);
                return;
            }

            auto [tier, idx] = hitTestBoundary(event->pos().x());
            if (tier >= 0 && idx >= 0 && m_dragController && m_boundaryModel) {
                m_dragController->startDrag(tier, idx, m_boundaryModel);
                setCursor(Qt::SizeHorCursor);
                event->accept();
                return;
            }

            emit positionClicked(clientXToTime(event->pos().x()));
            event->accept();
        }

        void ChartPanelBase::mouseMoveEvent(QMouseEvent *event) {
            if (m_dragController && m_dragController->isDragging()) {
                TimePos currentTime = secToUs(clientXToTime(event->pos().x()));
                m_dragController->updateDrag(currentTime);
                emit boundaryDragging();
                return;
            }

            auto [tier, idx] = hitTestBoundary(event->pos().x());
            int activeTier = m_boundaryModel ? m_boundaryModel->activeTierIndex() : -1;

            if (tier >= 0) {
                if (m_hoveredBoundary.first != tier || m_hoveredBoundary.second != idx) {
                    m_hoveredBoundary = {tier, idx};
                    setCursor(Qt::SizeHorCursor);
                    emit hoveredBoundaryChanged(tier == activeTier ? idx : -1);
                    update();
                }
            } else {
                if (m_hoveredBoundary.first >= 0) {
                    m_hoveredBoundary = {-1, -1};
                    setCursor(Qt::ArrowCursor);
                    emit hoveredBoundaryChanged(-1);
                    update();
                }
            }
        }

        void ChartPanelBase::mouseReleaseEvent(QMouseEvent *event) {
            if (event->button() != Qt::LeftButton)
                return;

            if (m_dragController && m_dragController->isDragging()) {
                int draggedTier = m_dragController->draggedTier();
                int draggedBoundary = m_dragController->draggedBoundary();
                TimePos finalTime = secToUs(clientXToTime(event->pos().x()));
                m_dragController->endDrag(finalTime, m_undoStack);
                unsetCursor();
                emit boundaryDragFinished(draggedTier, draggedBoundary, finalTime);
            }

            setCursor(hitTestBoundary(event->pos().x()).first >= 0 ? Qt::SizeHorCursor : Qt::ArrowCursor);
        }

        void ChartPanelBase::contextMenuEvent(QContextMenuEvent *event) {
            if (!m_playWidget || !m_boundaryModel) {
                event->accept();
                return;
            }

            double clickTime = clientXToTime(event->pos().x());
            auto [segStart, segEnd] = findSurrounding(clickTime);
            playSegmentBetween(segStart, segEnd);

            event->accept();
        }

        void ChartPanelBase::wheelEvent(QWheelEvent *event) {
            if (event->modifiers() & Qt::ControlModifier) {
                event->ignore();
                return;
            }
            if (event->modifiers() & Qt::ShiftModifier) {
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

        void ChartPanelBase::onVerticalZoom(double factor) {
            if (!supportsVerticalZoom())
                return;
            m_amplitudeScale = std::clamp(m_amplitudeScale * factor, amplitudeMinForZoom(), amplitudeMaxForZoom());
            update();
        }

        void ChartPanelBase::playSegmentBetween(double startSec, double endSec) {
            if (!m_playWidget)
                return;

            m_playWidget->setPlayRange(startSec, endSec);
            m_playWidget->seek(startSec);
            m_playWidget->setPlaying(true);
        }

    } // namespace dstools
} // namespace dstools
