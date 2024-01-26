//
// Created by fluty on 2024/1/26.
//

#ifndef COMMONGRAPHICSRECTITEM_H
#define COMMONGRAPHICSRECTITEM_H

#include <QGraphicsRectItem>

class CommonGraphicsRectItem : public QObject, public QGraphicsRectItem {
    Q_OBJECT
public:
    explicit CommonGraphicsRectItem(QGraphicsItem *parent = nullptr);

    double scaleX() const;
    void setScaleX(double scaleX);
    double scaleY() const;
    void setScaleY(double scaleY);
    QRectF visibleRect() const;

public slots:
    void setScale(qreal sx, qreal sy) {
        setScaleX(sx);
        setScaleY(sy);
    }
    void setVisibleRect(const QRectF &rect);

protected:
    virtual void updateRectAndPos() = 0;

private:
    double m_scaleX = 1;
    double m_scaleY = 1;
    QRectF m_visibleRect;
};



#endif // COMMONGRAPHICSRECTITEM_H
