//
// Created by fluty on 2024/1/23.
//

#ifndef NOTEGRAPHICSITEM_H
#define NOTEGRAPHICSITEM_H

#include "PianoRollGraphicsView.h"


#include <QGraphicsRectItem>
#include <QObject>

class NoteGraphicsItem final : public QObject, public QGraphicsRectItem {
    Q_OBJECT

public:
    explicit NoteGraphicsItem(int itemId, QGraphicsItem *parent = nullptr);
    explicit NoteGraphicsItem(int itemId, int start, int length, int keyIndex, const QString &lyric,
                              const QString &pronunciation, QGraphicsItem *parent = nullptr);

    int start() const;
    void setStart(int start);
    int length() const;
    void setLength(int length);
    int keyIndex() const;
    void setKeyIndex(int keyIndex);
    QString lyric() const;
    void setLyric(const QString &lyric);
    QString pronunciation() const;
    void setPronunciation(const QString &pronunciation);

    double scaleX() const;
    void setScaleX(double scaleX);
    double scaleY() const;
    void setScaleY(double scaleY);

    PianoRollGraphicsView *graphicsView;

signals:
    void propertyChanged();

public slots:
    void setScale(qreal sx, qreal sy) {
        setScaleX(sx);
        setScaleY(sy);
    }
    void setVisibleRect(const QRectF &rect);

private:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    // void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    // void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    // void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    // void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    // void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void updateRectAndPos();
    void initUi();

    int m_itemId;
    int m_start = 0;
    int m_length = 480;
    int m_keyIndex = 60;
    QString m_lyric;
    QString m_pronunciation = "miao";

    QRectF m_visibleRect;
    const double noteHeight = 24;
    const int pixelPerQuarterNote = 64;
    double m_scaleX = 1;
    double m_scaleY = 1;
};

#endif // NOTEGRAPHICSITEM_H
