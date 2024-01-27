//
// Created by fluty on 2023/11/14.
//

#ifndef DATASET_TOOLS_CLIPGRAPHICSITEM_H
#define DATASET_TOOLS_CLIPGRAPHICSITEM_H

#include "Controls/Base/CommonGraphicsRectItem.h"

class AbstractClipGraphicsItem : public CommonGraphicsRectItem {
    Q_OBJECT

public:
    enum MouseMoveBehavior { Move, ResizeRight, ResizeLeft, None };

    explicit AbstractClipGraphicsItem(int itemId, QGraphicsItem *parent = nullptr);
    ~AbstractClipGraphicsItem() override = default;

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

signals:
    // void propertyChanged(const QString &propertyName, const QString &value);
    void nameChanged(const QString &name);
    void startChanged(int itemId, int start);
    void lengthChanged(int length);
    void clipStartChanged(int clipStart);
    void clipLenChanged(int clipLen);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    virtual void drawPreviewArea(QPainter *painter, const QRectF &previewRect, int opacity) = 0;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void updateRectAndPos() override;
    virtual QString clipTypeName() = 0;
    void setCanResizeLength(bool on);
    double tickToSceneX(double tick) const;
    double sceneXToItemX(double x) const;

private:
    int m_itemId;
    QString m_name;
    int m_start = 0;
    int m_length = 0;
    int m_clipStart = 0;
    int m_clipLen = 0;
    double m_gain = 0;
    // double m_pan = 0;
    bool m_mute = false;
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

    QRectF previewRect() const;
};



#endif // DATASET_TOOLS_CLIPGRAPHICSITEM_H
