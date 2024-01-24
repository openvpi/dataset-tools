//
// Created by fluty on 2024/1/23.
//

#ifndef NOTEGRAPHICSITEM_H
#define NOTEGRAPHICSITEM_H

#include <QGraphicsRectItem>
#include <QObject>

class NoteGraphicsItem final : public QObject, public QGraphicsRectItem {
    Q_OBJECT

public:
    explicit NoteGraphicsItem(int itemId, QGraphicsItem *parent = nullptr);

    int start() const;
    void setStart(int start);
    int length() const;
    void setLength(int length);
    int keyIndex() const;
    void setKeyIndex(int keyIndex);
    QString lyric() const;
    void setLyric(const QString &lyric);

    double scaleX() const;
    void setScaleX(double scaleX);
    double scaleY() const;
    void setScaleY(double scaleY);

public slots:
    void setScale(qreal sx, qreal sy) {
        setScaleX(sx);
        setScaleY(sy);
    }
    void setVisibleRect(const QRectF &rect);

private:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void updateRectAndPos();

    int m_itemId;
    int m_start = 0;
    int m_length = 480;
    int m_keyIndex = 60;
    QString m_lyric;

    QRectF m_visibleRect;
    const double noteHeight = 24;
    const int pixelPerQuarterNote = 64;
    double m_scaleX = 1;
    double m_scaleY = 1;
};

#endif // NOTEGRAPHICSITEM_H
