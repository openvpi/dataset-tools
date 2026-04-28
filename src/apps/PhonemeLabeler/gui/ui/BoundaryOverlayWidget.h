#pragma once

#include <QWidget>

#include "ViewportController.h"

namespace dstools {
namespace phonemelabeler {

class TextGridDocument;

class BoundaryOverlayWidget : public QWidget {
    Q_OBJECT

public:
    explicit BoundaryOverlayWidget(ViewportController *viewport, QWidget *parent = nullptr);

    void setDocument(TextGridDocument *doc);
    void trackWidget(QWidget *widget);

public slots:
    void setViewport(const ViewportState &state);
    void setHoveredBoundary(int index);
    void setDraggedBoundary(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    [[nodiscard]] int timeToX(double time) const;
    void repositionOverSplitter();

    ViewportController *m_viewport = nullptr;
    TextGridDocument *m_document = nullptr;
    QWidget *m_trackedWidget = nullptr; // the splitter we overlay

    double m_viewStart = 0.0;
    double m_viewEnd = 10.0;

    int m_hoveredBoundary = -1;
    int m_draggedBoundary = -1;
};

} // namespace phonemelabeler
} // namespace dstools
