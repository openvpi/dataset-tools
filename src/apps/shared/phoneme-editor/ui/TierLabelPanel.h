#pragma once

#include <QWidget>
#include <QString>
#include <QColor>
#include <QFont>
#include <vector>

#include <dstools/ViewportController.h>
#include <dstools/TimePos.h>

namespace dstools {
namespace phonemelabeler {

using dsfw::widgets::ViewportController;
using dsfw::widgets::ViewportState;

class IBoundaryModel;

struct TierLabelRow {
    int tierIndex = -1;
    double labelY = 0.0;
    double rowHeight = 24.0;
    QColor textColor = QColor(220, 220, 220);
    QColor bgColor = QColor(40, 40, 45);
    QFont font;
};

class TierLabelPanel : public QWidget {
    Q_OBJECT

public:
    explicit TierLabelPanel(ViewportController *viewport, QWidget *parent = nullptr);

    void setBoundaryModel(IBoundaryModel *model);

    void setViewportState(const ViewportState &state);
    void setPixelWidth(int w);

    void setRows(const std::vector<TierLabelRow> &rows);
    const std::vector<TierLabelRow> &rows() const { return m_rows; }

signals:
    void hoveredBoundaryChanged(int boundaryIndex);
    void entryScrollRequested(int delta);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void drawRow(QPainter &painter, const TierLabelRow &row);
    [[nodiscard]] double timeToX(double timeSec) const;

    ViewportController *m_viewport = nullptr;
    IBoundaryModel *m_boundaryModel = nullptr;

    double m_viewStart = 0.0;
    double m_viewEnd = 10.0;
    int m_pixelWidth = 1200;

    std::vector<TierLabelRow> m_rows;

    int m_hoveredBoundary = -1;
    QPoint m_hoverPos;

    static constexpr int kBoundaryHitWidth = 12;
    static constexpr double kSnapThresholdSec = 0.03;
};

} // namespace phonemelabeler
} // namespace dstools