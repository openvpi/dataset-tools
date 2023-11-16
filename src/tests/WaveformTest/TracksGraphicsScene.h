//
// Created by fluty on 2023/11/14.
//

#ifndef DATASET_TOOLS_TRACKSGRAPHICSSCENE_H
#define DATASET_TOOLS_TRACKSGRAPHICSSCENE_H

#include <QGraphicsScene>


class TracksGraphicsScene : public QGraphicsScene{
public:
    explicit TracksGraphicsScene();
    ~TracksGraphicsScene();

public slots:
    void setScale(qreal sx, qreal sy);

public:
//    void setViewportRect(const QRectF &rect);
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    int m_sceneWidth = 1920 * 8; // tick
    int m_sceneHeight = 2000;
};



#endif // DATASET_TOOLS_TRACKSGRAPHICSSCENE_H
