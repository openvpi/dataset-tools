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
    QSizeF sceneSize() const;
    void setSceneSize(const QSizeF &size);

public slots:
    void setScale(qreal sx, qreal sy);

private:
    QSizeF m_sceneSize = QSizeF(1920, 1080);
    double m_scaleX = 1;
    double m_scaleY = 1;

    void updateSceneRect();
};

#endif // COMMONGRAPHICSSCENE_H
