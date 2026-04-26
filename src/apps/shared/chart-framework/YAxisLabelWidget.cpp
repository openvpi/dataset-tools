#include "YAxisLabelWidget.h"

#include <QPaintEvent>
#include <QPainter>

namespace dstools {

    YAxisLabelWidget::YAxisLabelWidget(int yOffset, int height, PaintFunc paintFn, QWidget *parent) :
        QWidget(parent), m_paintFn(std::move(paintFn)) {
        setFixedWidth(52);
        move(0, yOffset);
        setFixedHeight(height);
    }

    void YAxisLabelWidget::setChartGeometry(int yOffset, int height) {
        move(0, yOffset);
        setFixedHeight(height);
    }

    void YAxisLabelWidget::setLabelWidth(int width) {
        setFixedWidth(width);
    }

    void YAxisLabelWidget::paintEvent(QPaintEvent *event) {
        Q_UNUSED(event)
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        if (m_paintFn)
            m_paintFn(painter, rect());
    }

} // namespace dstools