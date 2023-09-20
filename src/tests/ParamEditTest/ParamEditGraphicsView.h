//
// Created by fluty on 2023/9/19.
//

#ifndef DATASET_TOOLS_PARAMEDITGRAPHICSVIEW_H
#define DATASET_TOOLS_PARAMEDITGRAPHICSVIEW_H

#include <QGraphicsView>

class ParamEditGraphicsView : public QGraphicsView {
    Q_OBJECT

public:
    ParamEditGraphicsView(QWidget *parent = nullptr);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    bool eventFilter(QObject *object, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
};



#endif // DATASET_TOOLS_PARAMEDITGRAPHICSVIEW_H
