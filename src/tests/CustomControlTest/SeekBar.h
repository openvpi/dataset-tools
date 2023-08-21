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
    void resizeEvent(QResizeEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    void calculateParams();
    int m_padding = 0;
    int m_trackPenWidth = 0;
    QRect m_rect;
    int m_actualStart = 0;
    int m_actualEnd = 0;
    int m_actualLength = 0;
    QPoint m_trackStartPoint;
    QPoint m_trackEndPoint;
    int m_activeStartPos = 0;
    QPoint m_activeStartPoint;
    QPoint m_activeEndPoint;
    int m_valuePos = 0;
    int m_handlePenWidth = 0;
    int m_handleRadius = 0;

    double m_value = 0;
    double m_defaultValue = 0;
    double m_max = 100;
    double m_min = -100;
    double m_trackActiveStartValue = 0;
    bool mouseOnHandle(const QPoint &mousePos) const;
    bool handleHover = false;
    bool handlePressed = false;
    QTimer *timer;
    bool doubleClickLocked = false;

private:
    SeekBarPrivate *d;
};

#endif // DATASET_TOOLS_SEEKBAR_H
