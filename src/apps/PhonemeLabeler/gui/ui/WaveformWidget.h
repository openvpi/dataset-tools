#pragma once

#include <QWidget>
#include <vector>
#include <QPoint>

#include "ViewportController.h"
#include "BoundaryBindingManager.h"

#include <dstools/PlayWidget.h>

class QPainter;
class QPixmap;
class QUndoStack;

namespace dstools {
namespace phonemelabeler {

class WaveformWidget : public QWidget {
    Q_OBJECT

public:
    explicit WaveformWidget(ViewportController *viewport, QWidget *parent = nullptr);
    ~WaveformWidget() override;

    void setAudioData(const std::vector<float> &samples, int sampleRate);
    void loadAudio(const QString &path);
    void setDocument(class TextGridDocument *doc);
    void setBindingManager(BoundaryBindingManager *mgr) { m_bindingMgr = mgr; }
    void setUndoStack(QUndoStack *stack) { m_undoStack = stack; }
    void setPlayWidget(dstools::widgets::PlayWidget *pw) { m_playWidget = pw; }

    void setViewport(const ViewportState &state);
    void setPlayhead(double sec);

    // Boundary overlay: draw active tier boundaries through this widget
    void setBoundaryOverlayEnabled(bool enabled);
    void updateBoundaryOverlay();

signals:
    void positionClicked(double sec);
    void boundaryDragStarted(int tierIndex, int boundaryIndex);
    void boundaryDragging(int tierIndex, int boundaryIndex, double newTime);
    void boundaryDragFinished(int tierIndex, int boundaryIndex, double newTime);
    void hoveredBoundaryChanged(int boundaryIndex);
    void entryScrollRequested(int delta);

public slots:
    void onPlayheadChanged(double sec);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void drawWaveform(QPainter &painter);
    void drawPlayCursor(QPainter &painter);
    void drawBoundaryOverlay(QPainter &painter);
    void rebuildMinMaxCache();

    // Boundary interaction
    [[nodiscard]] int hitTestBoundary(int x) const;
    void startBoundaryDrag(int boundaryIndex, double time);
    void updateBoundaryDrag(double currentTime);
    void endBoundaryDrag(double finalTime);

    // Context menu helpers
    void findSurroundingBoundaries(double timeSec, double &outStart, double &outEnd) const;

    // Coordinate conversion
    [[nodiscard]] double xToTime(int x) const;
    [[nodiscard]] int timeToX(double time) const;

    ViewportController *m_viewport = nullptr;
    TextGridDocument *m_document = nullptr;
    BoundaryBindingManager *m_bindingMgr = nullptr;
    QUndoStack *m_undoStack = nullptr;
    dstools::widgets::PlayWidget *m_playWidget = nullptr;

    std::vector<float> m_samples;
    int m_sampleRate = 44100;

    // Min/max cache for fast rendering
    struct MinMaxPair {
        float min;
        float max;
    };
    std::vector<MinMaxPair> m_minMaxCache;
    double m_cachePixelsPerSecond = 0.0;

    double m_viewStart = 0.0;
    double m_viewEnd = 10.0;
    double m_pixelsPerSecond = 200.0;

    double m_playhead = -1.0; // -1 = not playing
    bool m_boundaryOverlayEnabled = true;

    // Drag state - viewport scrolling
    bool m_dragging = false;
    QPoint m_dragStartPos;
    double m_dragStartTime = 0.0;

    // Drag state - boundary dragging
    bool m_boundaryDragging = false;
    int m_draggedBoundary = -1;
    int m_draggedTier = -1;
    double m_boundaryDragStartTime = 0.0;
    std::vector<AlignedBoundary> m_dragAligned;
    std::vector<double> m_dragAlignedStartTimes;

    static constexpr int kBoundaryHitWidth = 8; // pixels
    int m_hoveredBoundary = -1;
};

} // namespace phonemelabeler
} // namespace dstools
