//
// Created by fluty on 2023/9/16.
//

#ifndef DATASET_TOOLS_PARAMGRAPHICSVIEW_H
#define DATASET_TOOLS_PARAMGRAPHICSVIEW_H

#include <QGraphicsView>

class ParamGraphicsView : public QGraphicsView {
    Q_OBJECT

protected:
    void wheelEvent(QWheelEvent *event) override;
};



#endif // DATASET_TOOLS_PARAMGRAPHICSVIEW_H
