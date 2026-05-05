#pragma once

#include <QWidget>
#include <QTimer>

#include <dstools/ViewportController.h>

namespace dstools {
namespace phonemelabeler {

using dstools::widgets::ViewportController;
using dstools::widgets::ViewportState;

class TextGridDocument;
class IBoundaryModel;

class BoundaryOverlayWidget : public QWidget {
    Q_OBJECT

public:
    explicit BoundaryOverlayWidget(ViewportController *viewport, QWidget *parent = nullptr);

    void setDocument(TextGridDocument *doc);
    void setBoundaryModel(IBoundaryModel *model);
    void setTierLabelGeometry(int totalHeight, int rowHeight);
    void trackWidget(QWidget *widget);

public slots:
    void setViewport(const ViewportState &state);
    void setHoveredBoundary(int index);
    void setDraggedBoundary(int index);
    void setPlayhead(double sec);

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    [[nodiscard]] int timeToX(double time) const;
    void repositionOverSplitter();

    ViewportController *m_viewport = nullptr;
    TextGridDocument *m_document = nullptr;
    IBoundaryModel *m_boundaryModel = nullptr;
    QWidget *m_trackedWidget = nullptr;

    double m_viewStart = 0.0;
    double m_viewEnd = 10.0;

    int m_hoveredBoundary = -1;
    int m_draggedBoundary = -1;
    double m_playhead = -1.0;

    int m_tierLabelTotalHeight = 0;
    int m_tierLabelRowHeight = 24;

    QTimer m_playheadHideTimer;
};

} // namespace phonemelabeler
} // namespace dstools
