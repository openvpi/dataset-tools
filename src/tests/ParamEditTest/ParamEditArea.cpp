//
// Created by fluty on 2023/9/5.
//

#include "ParamEditArea.h"
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>

ParamEditArea::ParamEditArea(QWidget *parent) {
}

ParamEditArea::~ParamEditArea() {
}

void ParamEditArea::loadParam(const ParamModel::RealParamCurve &param) {
    //    m_param = param;
    update();
}

void ParamEditArea::saveParam(ParamModel::RealParamCurve &param) {
    //    param = m_param;
}

void ParamEditArea::paintEvent(QPaintEvent *event) {
    auto rectWidth = rect().width();
    auto rectHeight = rect().height();
    auto colorPrimary = QColor(112, 156, 255);
    auto colorPrimaryDarker = QColor(81, 135, 255);
    auto colorAccent = QColor(255, 175, 95);
    auto colorAccentDarker = QColor(255, 159, 63);

    QLinearGradient gradient(0, 0, 0, rectHeight);
    gradient.setColorAt(0, QColor(112, 156, 255, 240));
    gradient.setColorAt(1, QColor(112, 156, 255, 30));

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPen pen;
    pen.setColor(colorPrimary);
    pen.setWidthF(1.5);

    QBrush brush;
    //    brush.setColor(QColor(112, 156, 255, 127));

    painter.setPen(pen);
    //    painter.setBrush(brush);

    QPainterPath path;
    //    for (const auto curve : m_curves){
    //
    //
    //    }
    if (curve.values().count() > 0) {
        auto firstValue = curve.values().first();
        path.moveTo(curve.pos(), rectHeight);
        int i = 0;
        painter.setBrush(gradient);
        for (const auto value : curve.values()) {
            auto x = curve.pos() + i * curve.step();
            auto y = value;
            path.lineTo(x, y);
            i++;
        }
        int lastX = curve.pos() + (i - 1) * curve.step();
        path.lineTo(lastX, rectHeight);
        painter.drawPath(path);

        pen.setColor(colorAccent);
        painter.setPen(pen);
        painter.drawLine(QPoint(curve.pos(), rectHeight), QPoint(curve.pos(), firstValue));

        i = 0;
        pen.setColor(colorPrimary);
        painter.setPen(pen);
        painter.setBrush(colorPrimary);
        for (const auto value : curve.values()) {
            auto x = curve.pos() + i * curve.step();
            auto y = value;
            painter.drawEllipse(x - 2, y - 2, 4, 4);
            i++;
        }
    }

    //    if (m_points.count() > 0) {
    //        auto firstValue = m_points.first();
    //        path.moveTo(firstValue);
    //        for (int i = 0; i < m_points.size() - 1; ++i) {
    //            QPointF sp = m_points[i];
    //            QPointF ep = m_points[i + 1];
    //            QPointF c1 = QPointF((sp.x() + ep.x()) / 2, sp.y());
    //            QPointF c2 = QPointF((sp.x() + ep.x()) / 2, ep.y());
    //            path.cubicTo(c1, c2, ep);
    //        }
    //        painter.drawPath(path);
    //
    //        painter.setBrush(Qt::white);
    //        for (const auto point : qAsConst(m_points)) {
    //            //            path.lineTo(point.x(), point.y());
    //            painter.drawEllipse(point.x() - 2, point.y() - 2, 4, 4);
    //        }
    //        painter.end();
    //    }

    QFrame::paintEvent(event);
}

void ParamEditArea::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
}


bool cmpy(QPoint const &a, QPoint const &b) {
    return a.x() < b.x();
}

void ParamEditArea::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        auto tick = event->x();
        auto value = event->y();
        if (firstDraw) {
            curve.setPos(tick);
            firstDraw = false;
        }
        curve.drawPoint(tick, value);
        update();
    }
    QWidget::mousePressEvent(event);
}

void ParamEditArea::mouseMoveEvent(QMouseEvent *event) {
    auto tick = event->x();
    auto value = event->y();
    curve.drawPoint(tick, value);
    update();
    QWidget::mouseMoveEvent(event);
}

void ParamEditArea::mouseReleaseEvent(QMouseEvent *event) {
    curve.drawEnd();
    QWidget::mouseReleaseEvent(event);
}

HandDrawCurve::HandDrawCurve() {
}

HandDrawCurve::HandDrawCurve(int pos, int step) {
}

HandDrawCurve::~HandDrawCurve() {
}

int HandDrawCurve::pos() const {
    return m_pos;
}

QList<int> HandDrawCurve::values() const {
    return m_values;
}

void HandDrawCurve::setPos(int tick) {
    qDebug() << "set pos" << tick;
    m_pos = tick;
}

void HandDrawCurve::drawPoint(int tick, int value) {
    qDebug() << "draw point" << tick << value;
    // snap tick to grid
    auto roundPos = [](int i, int step) {
        int times = i / step;
        int mod = i % step;
        if (mod > step / 2)
            return step * (times + 1);
        else
            return step * times;
    };
    tick = roundPos(tick, m_step);

    if (m_values.empty()) {
        setPos(tick);
        m_values.append(value);
    }

    if (!m_drawing) {
        m_prevDrawPos.setX(tick);
        m_prevDrawPos.setY(value);
        m_drawing = true;
    }

    auto valueAt = [](int x1, int y1, int x2, int y2, int x) {
        double dx = x2 - x1;
        double dy = y2 - y1;
        double ratio = dy / dx;
        qDebug() << "ratio" << ratio;
        return int(y1 + (x - x1) * ratio);
    };

    if (tick > m_pos) {
        int prevPos = m_prevDrawPos.x();
        int prevValue = m_prevDrawPos.y();
        int curPos = qMin(prevPos, tick);
        int right = qMax(prevPos, tick);
        int curveEnd = m_pos + m_values.count() * m_step;

        while (curPos < right) {
            auto curValue = valueAt(prevPos, prevValue, tick, value, curPos);
            if (curPos < curveEnd) { // Draw in curve
                int index = (curPos - m_pos) / m_step;
                m_values.replace(index, curValue);
            } else {
                m_values.append(curValue);
            }
            curPos += m_step;
            qDebug() << "paint" << curPos << curValue;
        }
//        m_values.replace((tick - m_pos) / m_step, value);
    } else if (tick < m_pos) {
        qDebug() << "<pos";
        int curPos = tick;
        int i = 0;
        while (curPos < m_pos) {
            auto curValue = valueAt(tick, value, m_pos, m_values.first(), curPos);
            m_values.insert(i, curValue);
            curPos += m_step;
            i++;
        }
        m_pos = tick;
    } else {
        qDebug() << "=pos";
        m_values.replace(0, value);
    }

    m_prevDrawPos.setX(tick);
    m_prevDrawPos.setY(value);
}

int HandDrawCurve::valueAt(int tick) {
    int curveEnd = m_pos + m_values.count() * m_step;
    if (tick < m_pos || tick > curveEnd)
        return 0;
    else {
        int index = (tick - m_pos) / m_step;
        int value = m_values.at(index);
        return value;
    }
}
int HandDrawCurve::step() const {
    return m_step;
}

void HandDrawCurve::drawEnd() {
    m_drawing = false;
}
