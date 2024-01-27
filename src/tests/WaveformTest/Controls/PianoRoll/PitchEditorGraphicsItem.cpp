//
// Created by fluty on 2024/1/25.
//

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>

#include "PianoRollGlobal.h"
#include "PitchEditorGraphicsItem.h"

using namespace PianoRollGlobal;

PitchEditorGraphicsItem::PitchEditorGraphicsItem() {
    setBackgroundColor(Qt::transparent);

    // auto loadProjectFile = [](const QString &filename, QJsonObject *jsonObj) {
    //     QFile loadFile(filename);
    //     if (!loadFile.open(QIODevice::ReadOnly)) {
    //         qDebug() << "Failed to open project file";
    //         return false;
    //     }
    //     QByteArray allData = loadFile.readAll();
    //     loadFile.close();
    //     QJsonParseError err;
    //     QJsonDocument json = QJsonDocument::fromJson(allData, &err);
    //     if (err.error != QJsonParseError::NoError)
    //         return false;
    //     if (json.isObject()) {
    //         *jsonObj = json.object();
    //     }
    //     return true;
    // };
    //
    // auto filename = "E:/Test/Param/小小opensvip.json";
    // QJsonObject obj;
    // if (loadProjectFile(filename, &obj)) {
    //     auto arrTracks = obj.value("TrackList").toArray();
    //     auto firstTrack = arrTracks.first().toObject();
    //     auto objEditedParams = firstTrack.value("EditedParams").toObject();
    //     auto objPitch = objEditedParams.value("Pitch").toObject();
    //     auto arrPointList = objPitch.value("PointList").toArray();
    //     for (const auto valPoint : qAsConst(arrPointList)) {
    //         auto arrPoint = valPoint.toArray();
    //         auto pos = arrPoint.first().toInt();
    //         auto val = arrPoint.last().toInt();
    //         auto pair = std::make_pair(pos, val);
    //         opensvipPitchParam.append(pair);
    //     }
    // }
}
PitchEditorGraphicsItem::EditMode PitchEditorGraphicsItem::editMode() const {
    return m_editMode;
}
void PitchEditorGraphicsItem::setEditMode(const EditMode &mode) {
    if (mode == Off)
        setTransparentForMouseEvents(true);
    else {
        setTransparentForMouseEvents(false);
    }
    m_editMode = mode;
}
void PitchEditorGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    OverlayGraphicsItem::paint(painter, option, widget);

    // painter->setRenderHint(QPainter::Antialiasing, false);

    if (scaleX() < 0.6)
        return;
    if (opensvipPitchParam.isEmpty())
        return;

    QPen pen;
    pen.setWidthF(1);
    auto colorAlpha = scaleX() < 0.8 ? 255 * (scaleX() - 0.6) / (0.8 - 0.6) : 255;
    pen.setColor(QColor(130, 134, 138, colorAlpha));
    painter->setPen(pen);

    auto sceneXToTick = [&](const double pos) { return 480 * pos / scaleX() / pixelsPerQuarterNote; };
    auto tickToSceneX = [&](const double tick) { return tick * scaleX() * pixelsPerQuarterNote / 480; };
    auto sceneXToItemX = [&](const double x) { return mapFromScene(QPointF(x, 0)).x(); };
    auto pitchToSceneY = [&](const double pitch) { return (12700 - pitch + 50) * scaleY() * noteHeight / 100; };
    auto sceneYToItemY = [&](const double y) { return mapFromScene(QPointF(0, y)).y(); };

    auto visibleStartTick = sceneXToTick(visibleRect().left());
    auto visibleEndTick = sceneXToTick(visibleRect().right());

    QPainterPath path;
    bool firstPoint = true;
    int prevPos = 0;
    int prevValue = 0;
    for (const auto &point : opensvipPitchParam) {
        auto pos = std::get<0>(point) - 480 * 3; // opensvip's "feature"
        auto value = std::get<1>(point);

        if (pos < visibleStartTick) {
            prevPos = pos;
            prevValue = value;
            continue;
        }

        if (firstPoint) {
            path.moveTo(sceneXToItemX(tickToSceneX(prevPos)), sceneYToItemY(pitchToSceneY(prevValue)));
            path.lineTo(sceneXToItemX(tickToSceneX(pos)), sceneYToItemY(pitchToSceneY(value)));
            firstPoint = false;
        } else
            path.lineTo(sceneXToItemX(tickToSceneX(pos)), sceneYToItemY(pitchToSceneY(value)));

        if (pos > visibleEndTick) {
            path.lineTo(sceneXToItemX(tickToSceneX(pos)), sceneYToItemY(pitchToSceneY(value)));
            break;
        }
    }
    painter->drawPath(path);
}
void PitchEditorGraphicsItem::updateRectAndPos() {
    auto pos = visibleRect().topLeft();
    setPos(pos);
    setRect(QRectF(0, 0, visibleRect().width(), visibleRect().height()));
    update();
}