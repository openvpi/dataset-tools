//
// Created by fluty on 2023/9/5.
//

#include "ParamEditArea.h"
#include <QPainter>
#include <QPainterPath>

ParamEditArea::ParamEditArea(QWidget *parent) {
}

ParamEditArea::~ParamEditArea() {
}

void ParamEditArea::loadParam(const ParamModel::RealParamCurve &param) {
    m_param = param;
    update();
}

void ParamEditArea::saveParam(ParamModel::RealParamCurve &param) {
    param = m_param;
}

void ParamEditArea::paintEvent(QPaintEvent *event) {
    auto rectWidth = rect().width();
    auto rectHeight = rect().height();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPen pen;
    pen.setColor(QColor(112, 156, 255));
    pen.setWidthF(1.5);

    QBrush brush;
    brush.setColor(QColor(112, 156, 255, 127));

    painter.setPen(pen);
    painter.setBrush(brush);

    QPainterPath path;
    auto firstValue = m_param.values.first();
    path.moveTo(0, firstValue < 1 ? rectHeight * (1 - firstValue) : 0);
    int i = 0;
    for (const auto value : qAsConst(m_param.values)) {
        auto x = 1.0 * i / m_param.values.count() * rectWidth;
        auto y = value < 1 ? rectHeight * (1 - value) : 0;
        path.lineTo(x, y);
        i++;
    }

    painter.drawPath(path);
    painter.end();

    QFrame::paintEvent(event);
}

void ParamEditArea::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
}
