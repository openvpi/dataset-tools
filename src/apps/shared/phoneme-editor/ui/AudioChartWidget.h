#pragma once

#include "BoundaryDragController.h"


#include <QPoint>
#include <QWidget>

#include "IBoundaryModel.h"
#include <dstools/TimePos.h>
#include <dstools/ViewportController.h>

#include <dstools/PlayWidget.h>

class QUndoStack;

namespace dstools {
    namespace phonemelabeler {

        using dstools::widgets::ViewportController;
        using dstools::widgets::ViewportState;


        class AudioChartWidget : public QWidget {
            Q_OBJECT

        public:
            explicit AudioChartWidget(ViewportController *viewport, QWidget *parent = nullptr);
            ~AudioChartWidget() override;

            void setBoundaryModel(IBoundaryModel *model);
            void setDragController(BoundaryDragController *ctrl);
            void setUndoStack(QUndoStack *stack);
            void setPlayWidget(dstools::widgets::PlayWidget *pw);
            void setViewport(const ViewportState &state);
            void updateBoundaryOverlay();

            virtual void rebuildCache() = 0;

        signals:
            void boundaryDragStarted(int tierIndex, int boundaryIndex);
            void boundaryDragging(int tierIndex, int boundaryIndex, TimePos newTime);
            void boundaryDragFinished(int tierIndex, int boundaryIndex, TimePos newTime);
            void hoveredBoundaryChanged(int boundaryIndex);
            void entryScrollRequested(int delta);

        protected:
            void mousePressEvent(QMouseEvent *event) override;
            void mouseMoveEvent(QMouseEvent *event) override;
            void mouseReleaseEvent(QMouseEvent *event) override;
            void wheelEvent(QWheelEvent *event) override;
            void contextMenuEvent(QContextMenuEvent *event) override;

            virtual void onViewportDragStart(double timeSec);
            virtual void onVerticalZoom(double factor);

            [[nodiscard]] int hitTestBoundary(int x, int *outTier = nullptr) const;
            void drawBoundaryOverlay(QPainter &painter);
            void findSurroundingBoundaries(double timeSec, double &outStart, double &outEnd) const;

            [[nodiscard]] double xToTime(int x) const;
            [[nodiscard]] int timeToX(double time) const;

            ViewportController *m_viewport = nullptr;
            IBoundaryModel *m_boundaryModel = nullptr;
            BoundaryDragController *m_dragController = nullptr;
            QUndoStack *m_undoStack = nullptr;
            dstools::widgets::PlayWidget *m_playWidget = nullptr;

            double m_viewStart = 0.0;
            double m_viewEnd = 10.0;

            bool m_dragging = false;
            QPoint m_dragStartPos;
            double m_dragStartTime = 0.0;

            static constexpr int kBoundaryHitWidth = 12;
            int m_hoveredBoundary = -1;
        };

    } // namespace phonemelabeler
} // namespace dstools
