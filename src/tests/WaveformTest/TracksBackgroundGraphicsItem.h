//
// Created by fluty on 2024/1/22.
//

#ifndef TRACKSBACKGROUNDGRAPHICSITEM_H
#define TRACKSBACKGROUNDGRAPHICSITEM_H

#include "TimeGridGraphicsItem.h"

class TracksBackgroundGraphicsItem :public TimeGridGraphicsItem{
protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    const double trackHeight = 72;
};



#endif //TRACKSBACKGROUNDGRAPHICSITEM_H
