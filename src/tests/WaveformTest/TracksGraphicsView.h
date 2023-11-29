//
// Created by fluty on 2023/11/14.
//

#ifndef DATASET_TOOLS_TRACKSGRAPHICSVIEW_H
#define DATASET_TOOLS_TRACKSGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QPropertyAnimation>

class TracksGraphicsView : public QGraphicsView {
    Q_OBJECT
    Q_PROPERTY(double scaleX READ scaleX WRITE setScaleX)
    Q_PROPERTY(double scaleY READ scaleY WRITE setScaleY)
    Q_PROPERTY(double horizontalScrollBarValue READ horizontalScrollBarValue WRITE setHorizontalScrollBarValue)
    Q_PROPERTY(double verticalScrollBarValue READ verticalScrollBarValue WRITE setVerticalScrollBarValue)

public:
    explicit TracksGraphicsView(QWidget *parent = nullptr);
    ~TracksGraphicsView();

    qreal scaleX();
    void setScaleX(const qreal sx);
    qreal scaleY();
    void setScaleY(const qreal sy);
    void setScale(const qreal sx, const qreal sy) {
        setScaleX(sx);
        setScaleY(sy);
    }
    int horizontalScrollBarValue();
    void setHorizontalScrollBarValue(int value);
    int verticalScrollBarValue();
    void setVerticalScrollBarValue(int value);

signals:
    void scaleChanged(qreal sx, qreal sy);

protected:
    bool event(QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    bool eventFilter(QObject *object, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    qreal m_scaleX = 1;
    qreal m_scaleY = 1;

    QPropertyAnimation m_scaleXAnimation;
    QPropertyAnimation m_scaleYAnimation;
    QPropertyAnimation m_horizontalScrollBarAnimation;
    QPropertyAnimation m_verticalScrollBarAnimation;
};



#endif // DATASET_TOOLS_TRACKSGRAPHICSVIEW_H
