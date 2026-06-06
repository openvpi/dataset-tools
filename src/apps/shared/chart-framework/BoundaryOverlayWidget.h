#pragma once

#include "ChartCoordinate.h"

#include <QPair>
#include <QTimer>
#include <QVector>
#include <QWidget>
#include <dsfw/widgets/ViewportController.h>

namespace dstools {
    namespace dstools {

        using dsfw::widgets::ViewportController;
        using dsfw::widgets::ViewportState;

        class IBoundaryModel;

        class BoundaryOverlayWidget : public QWidget {
            Q_OBJECT

        public:
            explicit BoundaryOverlayWidget(ViewportController *viewport, QWidget *parent = nullptr);

            void setBoundaryModel(IBoundaryModel *model);
            void setTierLabelGeometry(int totalHeight, int rowHeight);
            void setExtraTopOffset(int pixels);
            void setExcludedYRanges(const QVector<QPair<int, int>> &ranges) {
                m_excludedYRanges = ranges;
            }
            void setCoordConverter(const ChartCoordinate *conv) {
                m_converter = conv;
            }
            void updateLabelAreaHeight(int totalHeight);
            void trackWidget(QWidget *widget);

            void forceReposition();

        public slots:
            void setViewport(const ViewportState &state);
            void setHoveredBoundary(int index);
            void setDraggedBoundary(int index);
            void setPlayhead(double sec);

        protected:
            void paintEvent(QPaintEvent *event) override;
            bool eventFilter(QObject *watched, QEvent *event) override;

        private:
            [[nodiscard]] double timeToX(double time) const;
            void repositionOverSplitter();

            ViewportController *m_viewport = nullptr;
            IBoundaryModel *m_boundaryModel = nullptr;
            QWidget *m_trackedWidget = nullptr;

            double m_viewStart = 0.0;
            double m_viewEnd = 10.0;

            int m_hoveredBoundary = -1;
            int m_draggedBoundary = -1;
            double m_playhead = -1.0;

            int m_tierLabelTotalHeight = 0;
            int m_tierLabelRowHeight = 24;
            int m_extraTopOffset = 0;
            QVector<QPair<int, int>> m_excludedYRanges;
            const ChartCoordinate *m_converter = nullptr;

            QTimer m_playheadHideTimer;
        };

    } // namespace dstools
} // namespace dstools