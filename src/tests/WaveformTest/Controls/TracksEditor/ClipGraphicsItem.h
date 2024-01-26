//
// Created by fluty on 2023/11/14.
//

#ifndef DATASET_TOOLS_CLIPGRAPHICSITEM_H
#define DATASET_TOOLS_CLIPGRAPHICSITEM_H

#include "Controls/Base/CommonGraphicsRectItem.h"

class ClipGraphicsItem : public CommonGraphicsRectItem {
    Q_OBJECT

public:
    enum MouseMoveBehavior { Move, ResizeRight, ResizeLeft, None };

    explicit ClipGraphicsItem(int itemId, QGraphicsItem *parent = nullptr);
    ~ClipGraphicsItem() override;

    int itemId();
    QString name() const;
    void setName(const QString &text);

    // QColor color() const;
    // void setColor(const QColor &color);

    int start() const;
    void setStart(int start);
    int length() const;
    void setLength(int length);
    int clipStart() const;
    void setClipStart(int clipStart);
    int clipLen() const;
    void setClipLen(int clipLen);

    double gain() const;
    void setGain(double gain);
    // double pan() const;
    // void setPan(double gain);
    bool mute() const;
    void setMute(bool mute);

    int trackIndex() const;
    void setTrackIndex(int index);

    QRectF previewRect() const;

signals:
    // void propertyChanged(const QString &propertyName, const QString &value);
    void nameChanged(const QString &name);
    void startChanged(int itemId, int start);
    void lengthChanged(int length);
    void clipStartChanged(int clipStart);
    void clipLenChanged(int clipLen);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;

    void updateRectAndPos() override;

    const double trackHeight = 72;
    const int pixelPerQuarterNote = 64;

    int m_itemId;
    QString m_name;
    int m_start = 0;
    int m_length = 0;
    int m_clipStart = 0;
    int m_clipLen = 0;
    double m_gain = 0;
    // double m_pan = 0;
    bool m_mute = false;
    QString m_clipTypeStr = "Clip";
    QRectF m_rect;
    int m_resizeTolerance = 8; // px
    bool m_canResizeLength = false;
    // bool m_mouseOnResizeRightArea = false;
    // bool m_mouseOnResizeLeftArea = false;

    MouseMoveBehavior m_mouseMoveBehavior = Move;
    QPointF m_mouseDownPos;
    // QPointF m_mouseDownScenePos;
    int m_mouseDownStart;
    int m_mouseDownClipStart;
    int m_mouseDownLength;
    int m_mouseDownClipLen;

    int m_trackIndex = 0;
    int m_quantize = 240; // tick, half quarter note
    bool m_tempQuantizeOff = false;
};



#endif // DATASET_TOOLS_CLIPGRAPHICSITEM_H
