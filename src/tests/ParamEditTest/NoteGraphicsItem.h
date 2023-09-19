//
// Created by fluty on 2023/9/16.
//

#ifndef DATASET_TOOLS_NOTEGRAPHICSITEM_H
#define DATASET_TOOLS_NOTEGRAPHICSITEM_H

#include <QGraphicsRectItem>

class NoteGraphicsItem : public QGraphicsRectItem {

public:
    explicit NoteGraphicsItem(QGraphicsItem *parent = nullptr);

    void setText(const QString &text);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

    QString m_text;
};



#endif // DATASET_TOOLS_NOTEGRAPHICSITEM_H
