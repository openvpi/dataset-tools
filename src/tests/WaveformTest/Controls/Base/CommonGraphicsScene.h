//
// Created by fluty on 2024/1/23.
//

#ifndef COMMONGRAPHICSSCENE_H
#define COMMONGRAPHICSSCENE_H

#include <QGraphicsScene>

class CommonGraphicsScene : public QGraphicsScene {
public:
    explicit CommonGraphicsScene();
    ~CommonGraphicsScene() override = default;

public slots:
    void setScale(qreal sx, qreal sy);

protected:
    int m_sceneWidth = 1920 / 480 * 64 * 100; // 100 bars
    int m_sceneHeight = 2000;
};

#endif // COMMONGRAPHICSSCENE_H
