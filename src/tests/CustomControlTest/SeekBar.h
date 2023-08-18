//
// Created by fluty on 2023/8/16.
//

#ifndef DATASET_TOOLS_SEEKBAR_H
#define DATASET_TOOLS_SEEKBAR_H

#include "QSlider"
#include "WidgetsGlobal/QMWidgetsGlobal.h"
#include "QMouseEvent"

class SeekBarPrivate;

class QMWIDGETS_EXPORT SeekBar : public QWidget {
    Q_OBJECT

public:
    explicit SeekBar(QWidget *parent = nullptr);
    ~SeekBar();

    void setValue(double value);
    void setDefaultValue(double value);
    void setMax(double max);
    void setMin(double min);
    void setRange(double min, double max);
    void setTrackActiveStartValue(double pos);
    void reset();

signals:
    void valueChanged(double value);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    double m_value = 0;
    double m_defaultValue = 0;
    double m_max = 100;
    double m_min = -100;
    double m_trackActiveStartValue = 0;
    bool mouseOnHandle(const QPoint &mousePos) const;
    bool handleHover = false;
    bool handlePressed = false;
    int actualLeft = 0;
    int actualRight = 0;
    int actualWidth = 0;
    QTimer *timer;
    bool doubleClickLocked = false;

private:
    SeekBarPrivate *d;
};

#endif // DATASET_TOOLS_SEEKBAR_H
