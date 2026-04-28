#pragma once

#include <QWidget>
#include <QPoint>
#include <QVector>

#include <dstools/ViewportController.h>
#include "TextGridDocument.h"
#include "BoundaryBindingManager.h"

class QPainter;
class QUndoStack;

namespace dstools {
namespace phonemelabeler {

using dstools::widgets::ViewportController;
using dstools::widgets::ViewportState;

class IntervalTierView : public QWidget {
    Q_OBJECT

public:
    explicit IntervalTierView(int tierIndex, TextGridDocument *doc,
                               QUndoStack *undoStack, ViewportController *viewport,
                               BoundaryBindingManager *bindingMgr,
                               QWidget *parent = nullptr);
    ~IntervalTierView() override = default;

    void setViewport(const ViewportState &state);
    int tierIndex() const { return m_tierIndex; }

    void setActive(bool active);
    [[nodiscard]] bool isActive() const { return m_active; }

    // Keyboard navigation
    void selectNextInterval();
    void selectPrevInterval();
    void adjustSelectedBoundary(double deltaSec);

signals:
    void intervalClicked(int intervalIndex, double startTime, double endTime);
    void intervalDoubleClicked(int intervalIndex);
    void boundaryDragStarted(int boundaryIndex);
    void requestPlayback(double startTime, double endTime);
    void activated(int tierIndex);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    // Coordinate conversion
    [[nodiscard]] int timeToX(double time) const;
    [[nodiscard]] double xToTime(int x) const;
    [[nodiscard]] int hitTestBoundary(int x) const;
    [[nodiscard]] int hitTestInterval(int x) const;

    // Drawing
    void drawIntervals(QPainter &painter);
    void drawBoundaries(QPainter &painter);
    void drawLabels(QPainter &painter);
    void drawBindingLines(QPainter &painter);
    void drawSelection(QPainter &painter);

    // Interaction
    void startDrag(int boundaryIndex, double startTime);
    void updateDrag(double currentTime);
    void endDrag(double finalTime);

    int m_tierIndex;
    TextGridDocument *m_doc = nullptr;
    QUndoStack *m_undoStack = nullptr;
    ViewportController *m_viewport = nullptr;
    BoundaryBindingManager *m_bindingMgr = nullptr;

    double m_viewStart = 0.0;
    double m_viewEnd = 10.0;
    double m_pixelsPerSecond = 200.0;

    // State
    enum class State { Idle, Hovering, Dragging };
    State m_state = State::Idle;
    int m_hoveredBoundary = -1;
    int m_draggedBoundary = -1;
    int m_selectedInterval = -1;  // For keyboard navigation

    // Drag state
    double m_dragStartTime = 0.0;
    std::vector<AlignedBoundary> m_dragAligned;
    std::vector<double> m_dragAlignedStartTimes;

    static constexpr int kBoundaryHitWidth = 6; // pixels
    bool m_active = false;
};

} // namespace phonemelabeler
} // namespace dstools
