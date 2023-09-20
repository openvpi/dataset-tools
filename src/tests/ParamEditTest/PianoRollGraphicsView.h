//
// Created by fluty on 2023/9/16.
//

#ifndef DATASET_TOOLS_PIANOROLLGRAPHICSVIEW_H
#define DATASET_TOOLS_PIANOROLLGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QTimer>

class PianoRollGraphicsView : public QGraphicsView {
    Q_OBJECT

public:
    PianoRollGraphicsView(QWidget *parent = nullptr);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    bool eventFilter(QObject *object, QEvent *event) override;

    bool isWhiteKey(const int &midiKey);
    QString toNoteName(const int &midiKey);

//    QTimer m_timer;
};



#endif // DATASET_TOOLS_PIANOROLLGRAPHICSVIEW_H
