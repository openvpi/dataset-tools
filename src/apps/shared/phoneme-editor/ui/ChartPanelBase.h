#pragma once

#include <QPoint>
#include <QWidget>
#include <utility>

#include <dstools/PlayWidget.h>
#include <dstools/TimePos.h>

#include "ChartPanelTypes.h"

class QUndoStack;

namespace dstools {
    namespace phonemelabeler {

        using dsfw::widgets::ViewportController;

        class IBoundaryModel;
        class BoundaryDragController;

        class ChartPanelBase : public QWidget, public IChartPanel {
            Q_OBJECT

        public:
            explicit ChartPanelBase(const QString &id, ViewportController *viewport, QWidget *parent = nullptr);

            QString chartId() const override {
                return m_id;
            }
            QWidget *widget() override {
                return this;
            }

            void onViewportUpdate(const CoordinateTransformer &xf) override;
            void onBoundaryModelInvalidated() override {
                update();
            }
            void onPlayheadUpdate(const PlayheadState &state) override;

            std::pair<int, int> hitTestBoundary(int x) const override;
            std::pair<double, double> findSurrounding(double timeSec) const override;

            void onActiveTierChanged(int tierIndex) override;

            void setBoundaryModel(IBoundaryModel *model) {
                m_boundaryModel = model;
            }
            void setDragController(BoundaryDragController *ctrl) {
                m_dragController = ctrl;
            }
            void setPlayWidget(dstools::widgets::PlayWidget *pw) {
                m_playWidget = pw;
            }
            void setUndoStack(QUndoStack *stack) {
                m_undoStack = stack;
            }

            void setShowBoundaryOverlay(bool show) {
                m_showBoundaryOverlay = show;
            }

            void setViewport(const ViewportState &state);
            CoordinateTransformer m_xf;
        signals:
            void visibleStateChanged(bool visible);
            void hoveredBoundaryChanged(int boundaryIndex);
            void boundaryDragging();
            void entryScrollRequested(int delta);
            void positionClicked(double sec);
            void boundaryDragFinished(int tierIndex, int boundaryIndex, dstools::TimePos time);

        protected:
            virtual void rebuildCache() {
            }
            virtual void drawChartImage(QPainter &painter) {
                Q_UNUSED(painter)
            }
            virtual void drawContent(QPainter &painter) = 0;
            virtual void onVerticalZoom(double factor) {
                Q_UNUSED(factor)
            }
            virtual void onViewportDragStart(double timeSec) {
                Q_UNUSED(timeSec)
            }

            void paintEvent(QPaintEvent *event) override;
            void resizeEvent(QResizeEvent *event) override;
            void mousePressEvent(QMouseEvent *event) override;
            void mouseMoveEvent(QMouseEvent *event) override;
            void mouseReleaseEvent(QMouseEvent *event) override;
            void contextMenuEvent(QContextMenuEvent *event) override;
            void wheelEvent(QWheelEvent *event) override;

            bool m_cacheDirty = true;

        private:
            void drawBoundaries(QPainter &painter);
            void playSegmentBetween(double startSec, double endSec);

            QString m_id;
            ViewportController *m_viewport = nullptr;
            IBoundaryModel *m_boundaryModel = nullptr;
            BoundaryDragController *m_dragController = nullptr;
            dstools::widgets::PlayWidget *m_playWidget = nullptr;
            QUndoStack *m_undoStack = nullptr;

            bool m_showBoundaryOverlay = true;
            bool m_viewportDragging = false;
            QPoint m_dragStartPos;
            double m_dragStartXfStart = 0.0;
            std::pair<int, int> m_hoveredBoundary = {-1, -1};

            static constexpr int kBoundaryHitWidth = 12;
            static constexpr double kSnapThresholdSec = 0.03;
        };

    } // namespace phonemelabeler
} // namespace dstools