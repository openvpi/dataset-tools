//
// Created by fluty on 2024/1/23.
//

#ifndef COMMONGRAPHICSVIEW_H
#define COMMONGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QPropertyAnimation>
#include <QTimer>

class CommonGraphicsView : public QGraphicsView {
    Q_OBJECT
    Q_PROPERTY(double scaleX READ scaleX WRITE setScaleX)
    Q_PROPERTY(double scaleY READ scaleY WRITE setScaleY)
    Q_PROPERTY(double horizontalScrollBarValue READ hBarValue WRITE setHBarValue)
    Q_PROPERTY(double verticalScrollBarValue READ vBarValue WRITE setVBarValue)

public:
    explicit CommonGraphicsView(QWidget *parent = nullptr);
    ~CommonGraphicsView() override = default;

    qreal scaleX() const;
    void setScaleX(qreal sx);
    qreal scaleY() const;
    void setScaleY(qreal sy);
    void setScale(const qreal sx, const qreal sy) {
        setScaleX(sx);
        setScaleY(sy);
    }
    qreal scaleXMax() const;
    void setScaleXMax(qreal max);
    int hBarValue() const;
    void setHBarValue(int value);
    int vBarValue() const;
    void setVBarValue(int value);
    QRectF visibleRect() const;

signals:
    void scaleChanged(qreal sx, qreal sy);
    void visibleRectChanged(const QRectF &rect);

public slots:
    void notifyVisibleRectChanged();

protected:
    bool event(QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    // bool eventFilter(QObject *object, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    bool isMouseWheel(QWheelEvent *event);

    double m_hZoomingStep = 0.4;
    double m_vZoomingStep = 0.3;
    qreal m_scaleX = 1;
    qreal m_scaleY = 1;
    // qreal m_scaleXMin = 0.1;
    qreal m_scaleXMax = 3; // 3x
    qreal m_scaleYMin = 0.5;
    qreal m_scaleYMax = 8;

    QPropertyAnimation m_scaleXAnimation;
    QPropertyAnimation m_scaleYAnimation;
    QPropertyAnimation m_hBarAnimation;
    QPropertyAnimation m_vBarAnimation;

    QTimer m_timer;
    bool m_touchPadLock = false;
};

#endif // COMMONGRAPHICSVIEW_H
