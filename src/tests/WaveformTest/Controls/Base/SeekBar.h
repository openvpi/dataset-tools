//
// Created by fluty on 2023/8/16.
//

#ifndef DATASET_TOOLS_SEEKBAR_H
#define DATASET_TOOLS_SEEKBAR_H

#include "QMouseEvent"
#include "QSlider"
#include <QPropertyAnimation>


class SeekBar : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QColor trackInactiveColor READ trackInactiveColor WRITE setTrackInactiveColor)
    Q_PROPERTY(QColor trackActiveColor READ trackActiveColor WRITE setTrackActiveColor)
    Q_PROPERTY(QColor thumbBorderColor READ thumbBorderColor WRITE setThumbBorderColor)
    Q_PROPERTY(int thumbBorderRatio READ thumbBorderRatio WRITE setThumbBorderRatio)

public:
    explicit SeekBar(QWidget *parent = nullptr);

    double value();
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
    bool eventFilter(QObject *object, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void calculateParams();
    int m_padding = 0;
    int m_trackPenWidth = 0;
    QRect m_rect;
    int m_halfHeight;
    int m_actualStart = 0;
    int m_actualEnd = 0;
    int m_actualLength = 0;
    QPoint m_trackStartPoint;
    QPoint m_trackEndPoint;
    int m_activeStartPos = 0;
    QPointF m_activeStartPoint;
    QPointF m_activeEndPoint;
    int m_valuePos = 0;
    float m_handlePenWidth = 0;
    float m_handleRadius = 0;

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
    QPropertyAnimation *m_thumbHoverAnimation;
    QColor m_trackInactiveColor = QColor(0, 0, 0, 32);
    QColor trackInactiveColor() const;
    void setTrackInactiveColor(const QColor &color);
    QColor m_trackActiveColor = QColor(112, 156, 255);
    QColor trackActiveColor() const;
    void setTrackActiveColor(const QColor &color);
    QColor m_thumbBorderColor = QColor(255, 255, 255);
    QColor thumbBorderColor() const;
    void setThumbBorderColor(const QColor &color);

    // Animation
    int m_thumbBorderRatio = 102; // ratio max = 255;
    int thumbBorderRatio() const;
    void setThumbBorderRatio(int ratio);
};

#endif // DATASET_TOOLS_SEEKBAR_H
