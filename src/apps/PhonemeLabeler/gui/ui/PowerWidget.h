#pragma once

#include <QWidget>
#include <vector>
#include <QPoint>

#include "ViewportController.h"
#include "BoundaryBindingManager.h"

class QPainter;
class QUndoStack;

namespace dstools {
namespace phonemelabeler {

class TextGridDocument;

class PowerWidget : public QWidget {
    Q_OBJECT

public:
    explicit PowerWidget(ViewportController *viewport, QWidget *parent = nullptr);
    ~PowerWidget() override;

    void setAudioData(const std::vector<float> &samples, int sampleRate);
    void setViewport(const ViewportState &state);
    void setDocument(TextGridDocument *doc) { m_document = doc; }
    void setBindingManager(BoundaryBindingManager *mgr) { m_bindingMgr = mgr; }
    void setUndoStack(QUndoStack *stack) { m_undoStack = stack; }

    void updateBoundaryOverlay();

signals:
    void boundaryDragStarted(int tierIndex, int boundaryIndex);
    void boundaryDragging(int tierIndex, int boundaryIndex, double newTime);
    void boundaryDragFinished(int tierIndex, int boundaryIndex, double newTime);
    void hoveredBoundaryChanged(int boundaryIndex);
    void entryScrollRequested(int delta);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void rebuildPowerCache();
    void drawPower(QPainter &painter);
    void drawReferenceLines(QPainter &painter);
    void drawBoundaryOverlay(QPainter &painter);

    // Boundary interaction
    [[nodiscard]] int hitTestBoundary(int x) const;
    void startBoundaryDrag(int boundaryIndex, double time);
    void updateBoundaryDrag(double currentTime);
    void endBoundaryDrag(double finalTime);

    [[nodiscard]] double xToTime(int x) const;
    [[nodiscard]] int timeToX(double time) const;

    ViewportController *m_viewport = nullptr;
    TextGridDocument *m_document = nullptr;
    BoundaryBindingManager *m_bindingMgr = nullptr;
    QUndoStack *m_undoStack = nullptr;

    std::vector<float> m_samples;
    int m_sampleRate = 44100;

    // Power data: dB values per pixel column
    std::vector<float> m_powerCache;

    double m_viewStart = 0.0;
    double m_viewEnd = 10.0;
    double m_pixelsPerSecond = 200.0;

    // vLabeler reference parameters
    static constexpr int kUnitSize = 60;
    static constexpr int kWindowSize = 300;
    static constexpr float kMinPower = -48.0f;
    static constexpr float kMaxPower = 0.0f;
    static constexpr float kRefValue = 32768.0f; // 2^15

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

    static constexpr int kBoundaryHitWidth = 8;
    int m_hoveredBoundary = -1;
};

} // namespace phonemelabeler
} // namespace dstools
