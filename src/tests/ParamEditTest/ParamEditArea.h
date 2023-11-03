//
// Created by fluty on 2023/9/5.
//

#ifndef DATASET_TOOLS_PARAMEDITAREA_H
#define DATASET_TOOLS_PARAMEDITAREA_H

#include <QFrame>

#include <ParamModel.h>

class HandDrawCurve {
public:
    explicit HandDrawCurve();
    explicit HandDrawCurve(int pos, int step = 5);
    ~HandDrawCurve();

    int pos() const;
    int step() const;
    QList<int> values() const;

    void setPos(int tick);
//    void setStep(int step);
    void drawPoint(int tick, int value);
    void drawEnd();
    int valueAt(int tick);
    int elemAt(int index);
    void insert(int tick, int value);
    bool remove(int tick);
    bool merge(const HandDrawCurve &curve);
    void clear();

protected:
    int m_pos = 0;
    int m_step = 10;
    QList<int> m_values;

    QPoint m_prevDrawPos;
    bool m_drawing = false;
};

class ParamEditArea : public QFrame {
    Q_OBJECT

public:
    explicit ParamEditArea(QWidget *parent = nullptr);
    ~ParamEditArea() override;

    void loadParam(const ParamModel::RealParamCurve &param);
    void saveParam(ParamModel::RealParamCurve &param);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

//    ParamModel::RealParamCurve m_param;
//    QVector<QPoint> m_points;
    QList<HandDrawCurve> m_curves;
    HandDrawCurve curve;
    bool firstDraw = true;
    QPoint m_prevPos;
};



#endif // DATASET_TOOLS_PARAMEDITAREA_H
