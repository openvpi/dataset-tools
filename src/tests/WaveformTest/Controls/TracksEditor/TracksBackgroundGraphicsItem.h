//
// Created by fluty on 2024/1/22.
//

#ifndef TRACKSBACKGROUNDGRAPHICSITEM_H
#define TRACKSBACKGROUNDGRAPHICSITEM_H

#include "../Base/TimeGridGraphicsItem.h"

class TracksBackgroundGraphicsItem final : public TimeGridGraphicsItem {
protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};

#endif // TRACKSBACKGROUNDGRAPHICSITEM_H
