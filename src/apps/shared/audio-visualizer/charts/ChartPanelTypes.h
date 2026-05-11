#pragma once

#include <QColor>
#include <QObject>
#include <QString>
#include <utility>
#include <vector>

#include <dsfw/widgets/ViewportController.h>
#include <dstools/PlayWidget.h>

class QUndoStack;

namespace dstools {
namespace phonemelabeler {

class IBoundaryModel;
class BoundaryDragController;

using dsfw::widgets::ViewportController;
using dsfw::widgets::ViewportState;

struct CoordinateTransformer {
    double viewStart = 0.0;
    double viewEnd = 10.0;
    int resolution = 40;
    int sampleRate = 44100;
    int pixelWidth = 1200;

    [[nodiscard]] double timeToX(double timeSec) const {
        double dur = viewEnd - viewStart;
        if (dur <= 0.0 || pixelWidth <= 0)
            return 0.0;
        return (timeSec - viewStart) / dur * pixelWidth;
    }

    [[nodiscard]] double xToTime(double x) const {
        double dur = viewEnd - viewStart;
        if (pixelWidth <= 0)
            return viewStart;
        return viewStart + dur * x / pixelWidth;
    }

    [[nodiscard]] double pixelsPerSecond() const {
        return sampleRate > 0 ? static_cast<double>(sampleRate) / resolution : 200.0;
    }

    void updateFromState(const ViewportState &state, int width) {
        viewStart = state.startSec;
        viewEnd = state.endSec;
        resolution = state.resolution;
        sampleRate = state.sampleRate;
        pixelWidth = width;
    }
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

    virtual void onViewportUpdate(const CoordinateTransformer &xf) = 0;
    virtual void onBoundaryModelInvalidated() = 0;
    virtual void onPlayheadUpdate(const PlayheadState &state) = 0;

    virtual std::pair<int, int> hitTestBoundary(int x) const = 0;
    virtual std::pair<double, double> findSurrounding(double timeSec) const = 0;

    virtual bool supportsSegmentPlayback() const { return true; }
    virtual void onActiveTierChanged(int /*tierIndex*/) {}

    virtual void setAudioData(const std::vector<float> & /*samples*/, int /*sampleRate*/) {}
    virtual void setBoundaryModel(IBoundaryModel * /*model*/) {}
    virtual void setDragController(BoundaryDragController * /*ctrl*/) {}
    virtual void setPlayWidget(dstools::widgets::PlayWidget * /*pw*/) {}
    virtual void setUndoStack(QUndoStack * /*stack*/) {}
};

} // namespace phonemelabeler
} // namespace dstools