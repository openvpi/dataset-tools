#pragma once

#include <QObject>
#include <QString>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ChartPanelTypes.h"

class QUndoStack;

namespace dstools {
    namespace phonemelabeler {

        class ChartCoordinator : public QObject {
            Q_OBJECT

        public:
            explicit ChartCoordinator(QObject *parent = nullptr);

            void addPanel(IChartPanel *panel);
            void removePanel(const QString &chartId);

            IChartPanel *panel(const QString &chartId);

            std::vector<QString> chartIds() const;

            void onViewportStateUpdate(const ViewportState &state, int pixelWidth);
            void onPlayheadUpdate(const PlayheadState &state);
            void onActiveTierChanged(int tierIndex);
            void setBoundaryModel(IBoundaryModel *model);
            void notifyAll();

            void setDragController(BoundaryDragController *ctrl);
            void setPlayWidget(dstools::widgets::PlayWidget *pw);
            void setUndoStack(QUndoStack *stack);

        signals:
            void viewportChanged(const CoordinateTransformer &xf);
            void playheadChanged(const PlayheadState &state);

        private:
            CoordinateTransformer m_xf;
            PlayheadState m_playhead;
            std::vector<IChartPanel *> m_panels;
            std::unordered_map<QString, IChartPanel *> m_panelMap;
            IBoundaryModel *m_boundaryModel = nullptr;
            BoundaryDragController *m_dragController = nullptr;
            dstools::widgets::PlayWidget *m_playWidget = nullptr;
            QUndoStack *m_undoStack = nullptr;
        };

    } // namespace phonemelabeler
} // namespace dstools