#pragma once

#include "ChartCoordinate.h"

#include <QColor>
#include <QObject>
#include <QString>
#include <dsfw/widgets/ViewportController.h>
#include <dsfw/widgets/PlayWidget.h>
#include <utility>
#include <vector>

class QUndoStack;

namespace dstools {

        class IBoundaryModel;
        class BoundaryDragController;

        using dsfw::widgets::ViewportController;
        using dsfw::widgets::ViewportState;

        struct RegionUpdate {
            bool fullRebuild = true;
            int dirtyStartCol = 0;
            int dirtyEndCol = 0;
            int colShift = 0;
        };

        struct PlayheadState {
            double positionSec = -1.0;
            QColor color = QColor(255, 60, 60, 220);
            int lineWidth = 2;
        };

        class IChartPanel {
        public:
            virtual ~IChartPanel() = default;

            virtual QString chartId() const = 0;
            virtual QWidget *widget() = 0;

            virtual void onViewportUpdate(const ChartCoordinate &conv, int pixelWidth) = 0;
            virtual void onBoundaryModelInvalidated() = 0;
            virtual void onPlayheadUpdate(const PlayheadState &state) = 0;

            virtual std::pair<int, int> hitTestBoundary(int x) const = 0;
            virtual std::pair<double, double> findSurrounding(double timeSec) const = 0;

            virtual bool supportsSegmentPlayback() const {
                return true;
            }
            virtual void onActiveTierChanged(int /*tierIndex*/) {
            }

            virtual void setAudioData(const std::vector<float> & /*samples*/, int /*sampleRate*/) {
            }
            virtual void setBoundaryModel(IBoundaryModel * /*model*/) {
            }
            virtual void setDragController(BoundaryDragController * /*ctrl*/) {
            }
            virtual void setPlayWidget(dsfw::widgets::PlayWidget * /*pw*/) {
            }
            virtual void setUndoStack(QUndoStack * /*stack*/) {
            }

            [[nodiscard]] virtual int yAxisWidth() const {
                return 0;
            }

            virtual QWidget *createYAxisLabelWidget(QWidget *parent) {
                Q_UNUSED(parent)
                return nullptr;
            }
        };

    } // namespace dstools