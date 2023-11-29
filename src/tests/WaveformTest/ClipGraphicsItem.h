//
// Created by fluty on 2023/11/14.
//

#ifndef DATASET_TOOLS_CLIPGRAPHICSITEM_H
#define DATASET_TOOLS_CLIPGRAPHICSITEM_H

#include <QGraphicsRectItem>

class ClipGraphicsItem : public QGraphicsRectItem, public QObject {

public:
    enum MouseMoveBehavior { Move, ResizeRight, ResizeLeft };

    explicit ClipGraphicsItem(QGraphicsItem *parent = nullptr);
    ~ClipGraphicsItem();

    QString name() const;
    void setName(const QString &text);

    // QColor color() const;
    // void setColor(const QColor &color);

    int start() const;
    void setStart(int start);
    int length() const;
    void setLength(const int length);
    int clipStart() const;
    void setClipStart(const int clipStart);
    int clipLen() const;
    void setClipLen(const int clipLen);

    double gain() const;
    void setGain(const double gain);
    // double pan() const;
    // void setPan(double gain);
    bool mute() const;
    void setMute(const bool mute);

    double scaleX() const;
    void setScaleX(const double scaleX);
    double scaleY() const;
    void setScaleY(const double scaleY);
    int trackIndex() const;
    void setTrackIndex(const int index);

    QRectF previewRect() const;

public slots:
    void setVisibleRect(const QRectF &rect);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    // void keyPressEvent(QKeyEvent *event) override;
    // void keyReleaseEvent(QKeyEvent *event) override;

    void updateRectAndPos();

    const double trackHeight = 72;
    const int pixelPerQuarterNote = 64;

    QString m_name;
    int m_start = 0;
    int m_length = 0;
    int m_clipStart = 0;
    int m_clipLen = 0;
    double m_gain = 0;
    // double m_pan = 0;
    bool m_mute = false;
    QRectF m_rect;
    QRectF m_visibleRect;
    int m_resizeTolerance = 8; // px
    // bool m_mouseOnResizeRightArea = false;
    // bool m_mouseOnResizeLeftArea = false;

    MouseMoveBehavior m_mouseMoveBehavior = Move;
    QPointF m_mouseDownPos;
    // QPointF m_mouseDownScenePos;
    int m_mouseDownStart;
    int m_mouseDownClipStart;
    int m_mouseDownLength;
    int m_mouseDownClipLen;

    double m_scaleX = 1;
    double m_scaleY = 1;
    int m_trackIndex = 0;
    int m_quantize = 240; // tick, half quarter note
    bool m_tempQuantizeOff = false;
};



#endif // DATASET_TOOLS_CLIPGRAPHICSITEM_H
