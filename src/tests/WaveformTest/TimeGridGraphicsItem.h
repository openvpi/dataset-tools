//
// Created by fluty on 2024/1/21.
//

#ifndef TIMEGRIDGRAPHICSITEM_H
#define TIMEGRIDGRAPHICSITEM_H

#include <QGraphicsRectItem>

class TimeGridGraphicsItem : public QObject, public QGraphicsRectItem {
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

    double scaleX() const;
    void setScaleX(double scaleX);
    double scaleY() const;
    void setScaleY(double scaleY);

public slots:
    void onVisibleRectChanged(const QRectF &rect);
    void setScale(qreal sx, qreal sy) {
        setScaleX(sx);
        setScaleY(sy);
    }
//     void onTimelineChanged();

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void updateRectAndPos();

    int m_quantization = 8; // 1/8 note
    int m_minimumSpacing = 24; // hide low level grid line when distance < 32 px
    double m_scaleX = 1;
    double m_scaleY = 1;
    const int pixelPerQuarterNote = 64;
    QRectF m_visibleRect;
};



#endif // TIMEGRIDGRAPHICSITEM_H
