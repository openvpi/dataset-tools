//
// Created by fluty on 2024/1/25.
//

#ifndef PITCHEDITORGRAPHICSITEM_H
#define PITCHEDITORGRAPHICSITEM_H

#include "Controls/Base/OverlayGraphicsItem.h"

class PitchEditorGraphicsItem final : public OverlayGraphicsItem {
public:
    enum EditMode { Free, Anchor, Off };

    class FreeCurve {
    public:
        int start;
        int step = 5;
        QVector<int> values; // cent
    };

    enum AnchorInterpMode { Linear, Hermite, Cubic, None };

    class AnchorNode {
    public:
        AnchorNode(const int pos, const int pitch) : pos(pos), pitch(pitch) {
        }
        int pos; // tick
        int pitch;
        AnchorInterpMode interpMode = Cubic;
    };

    class AnchorCurve {
    public:
        int start;
        QVector<AnchorNode> nodes;
    };

    QList<FreeCurve> pitchParamFree;
    QList<AnchorCurve> pitchParamAnchor;
    QVector<std::tuple<int, int>> opensvipPitchParam;

    explicit PitchEditorGraphicsItem();

    EditMode editMode() const;
    void setEditMode(const EditMode &mode);
    // void loadParam();
    // QList<FreeCurve> mergedPitchParam();

private:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    // void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    // void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    // void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    // void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    // void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    // void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void updateRectAndPos() override;

    EditMode m_editMode = Off;
};

#endif // PITCHEDITORGRAPHICSITEM_H
