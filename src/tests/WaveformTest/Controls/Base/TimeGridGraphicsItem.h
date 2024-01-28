//
// Created by fluty on 2024/1/21.
//

#ifndef TIMEGRIDGRAPHICSITEM_H
#define TIMEGRIDGRAPHICSITEM_H

#include "CommonGraphicsRectItem.h"

class TimeGridGraphicsItem : public CommonGraphicsRectItem {
    Q_OBJECT

public:
    // class TimeSignature {
    // public:
    //     int position = 0;
    //     int numerator = 4;
    //     int denominator = 4;
    // };

    explicit TimeGridGraphicsItem(QGraphicsItem *parent = nullptr);
    ~TimeGridGraphicsItem() override = default;

    void setPixelsPerQuarterNote(int px);

public slots:
    void onTimeSignatureChanged(int numerator, int denominator);
    //     void onTimelineChanged();

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void updateRectAndPos() override;

    int m_numerator = 3;
    int m_denominator = 4;
    int m_quantization = 8;    // 1/8 note
    int m_minimumSpacing = 24; // hide low level grid line when distance < 32 px
    int m_pixelsPerQuarterNote = 64;
};



#endif // TIMEGRIDGRAPHICSITEM_H
