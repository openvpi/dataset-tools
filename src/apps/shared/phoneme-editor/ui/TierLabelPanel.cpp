#include "TierLabelPanel.h"
#include "TextGridDocument.h"
#include "IBoundaryModel.h"

#include <dsfw/Theme.h>

#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>

#include <cmath>
#include <algorithm>

namespace dstools {
namespace phonemelabeler {

TierLabelPanel::TierLabelPanel(ViewportController *viewport, QWidget *parent)
    : QWidget(parent)
    , m_viewport(viewport)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAutoFillBackground(false);
}

void TierLabelPanel::setBoundaryModel(IBoundaryModel *model) {
    m_boundaryModel = model;
    update();
}

void TierLabelPanel::setViewportState(const ViewportState &state) {
    m_viewStart = state.startSec;
    m_viewEnd = state.endSec;
    update();
}

void TierLabelPanel::setPixelWidth(int w) {
    m_pixelWidth = w;
    update();
}

void TierLabelPanel::setRows(const std::vector<TierLabelRow> &rows) {
    m_rows = rows;
    update();
}

double TierLabelPanel::timeToX(double timeSec) const {
    double dur = m_viewEnd - m_viewStart;
    if (dur <= 0.0 || m_pixelWidth <= 0) return 0.0;
    return (timeSec - m_viewStart) / dur * m_pixelWidth;
}

void TierLabelPanel::paintEvent(QPaintEvent *) {
    if (m_rows.empty()) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    for (const auto &row : m_rows) {
        drawRow(painter, row);
    }
}

void TierLabelPanel::drawRow(QPainter &painter, const TierLabelRow &row) {
    if (row.tierIndex < 0 || !m_boundaryModel) return;

    int yCenter = static_cast<int>(row.labelY);
    int halfHeight = static_cast<int>(row.rowHeight / 2.0);
    QRect rowRect(0, yCenter - halfHeight, width(), static_cast<int>(row.rowHeight));

    painter.fillRect(rowRect, row.bgColor);

    int count = m_boundaryModel->boundaryCount(row.tierIndex);
    if (count < 2) return;

    auto *doc = dynamic_cast<TextGridDocument *>(m_boundaryModel);

    painter.setFont(row.font);
    painter.setPen(row.textColor);

    for (int i = 0; i < count - 1; ++i) {
        double tStart = usToSec(m_boundaryModel->boundaryTime(row.tierIndex, i));
        double tEnd = usToSec(m_boundaryModel->boundaryTime(row.tierIndex, i + 1));

        if (tEnd < m_viewStart || tStart > m_viewEnd) continue;

        if (tStart > tEnd) continue;

        double xStart = timeToX(tStart);
        double xEnd = timeToX(tEnd);
        double labelWidth = xEnd - xStart;

        if (labelWidth < 2.0) continue;

        QString text;
        if (doc) {
            text = doc->intervalText(row.tierIndex, i);
        } else {
            text = QStringLiteral("--");
        }

        QRectF labelRect(xStart, yCenter - halfHeight, labelWidth, row.rowHeight);
        painter.drawText(labelRect, Qt::AlignCenter, painter.fontMetrics().elidedText(text, Qt::ElideRight, static_cast<int>(labelWidth - 2)));
    }

    const auto &pal = dsfw::Theme::instance().palette();
    QColor borderColor(pal.border.red(), pal.border.green(), pal.border.blue(), 180);
    painter.setPen(QPen(borderColor, 1));
    painter.drawLine(0, yCenter - halfHeight, width(), yCenter - halfHeight);
    painter.drawLine(0, yCenter + halfHeight, width(), yCenter + halfHeight);
}

void TierLabelPanel::mouseMoveEvent(QMouseEvent *event) {
    m_hoverPos = event->pos();

    if (!m_boundaryModel) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    int bestIdx = -1;
    double bestDist = 1e9;
    double clickTime = m_viewStart + (m_viewEnd - m_viewStart) * event->pos().x() / m_pixelWidth;

    int activeTier = m_boundaryModel->activeTierIndex();
    if (activeTier < 0) return;

    int count = m_boundaryModel->boundaryCount(activeTier);
    for (int b = 0; b < count; ++b) {
        double bTime = usToSec(m_boundaryModel->boundaryTime(activeTier, b));
        double dist = std::abs(bTime - clickTime);
        int bx = static_cast<int>(timeToX(bTime));
        int pxDist = std::abs(event->pos().x() - bx);
        if (pxDist <= kBoundaryHitWidth / 2 && dist < bestDist) {
            bestDist = dist;
            bestIdx = b;
        }
    }

    if (bestIdx != m_hoveredBoundary) {
        m_hoveredBoundary = bestIdx;
        setCursor(bestIdx >= 0 ? Qt::SizeHorCursor : Qt::ArrowCursor);
        emit hoveredBoundaryChanged(bestIdx);
    }

    QWidget::mouseMoveEvent(event);
}

void TierLabelPanel::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier && m_viewport) {
        double t = m_viewStart + (m_viewEnd - m_viewStart) * event->position().x() / m_pixelWidth;
        double delta = event->angleDelta().y() / 120.0;
        double factor = (delta > 0) ? 1.25 : 0.8;
        m_viewport->zoomAt(t, factor);
        event->accept();
    } else {
        if (m_viewport) {
            double scrollSec = (event->angleDelta().y() > 0) ? -0.5 : 0.5;
            m_viewport->scrollBy(scrollSec);
        }
        event->accept();
    }
}

void TierLabelPanel::resizeEvent(QResizeEvent *event) {
    m_pixelWidth = width();
    QWidget::resizeEvent(event);
}

} // namespace phonemelabeler
} // namespace dstools