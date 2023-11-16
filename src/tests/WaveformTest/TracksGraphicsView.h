//
// Created by fluty on 2023/11/14.
//

#ifndef DATASET_TOOLS_TRACKSGRAPHICSVIEW_H
#define DATASET_TOOLS_TRACKSGRAPHICSVIEW_H

#include <QGraphicsView>

class TracksGraphicsView : public QGraphicsView {
    Q_OBJECT

public:
    explicit TracksGraphicsView(QWidget *parent = nullptr);
    ~TracksGraphicsView();

    void setScaleX(const qreal sx);
    void setScaleY(const qreal sy);
    void setScale(const qreal sx, const qreal sy) {
        setScaleX(sx);
        setScaleY(sy);
    }

signals:
    void scaleChanged(qreal sx, qreal sy);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    bool eventFilter(QObject *object, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    qreal m_scaleX = 1;
    qreal m_scaleY = 1;
};



#endif // DATASET_TOOLS_TRACKSGRAPHICSVIEW_H
