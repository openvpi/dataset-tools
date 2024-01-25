//
// Created by fluty on 2024/1/25.
//

#include "PitchEditorGraphicsItem.h"


#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>

PitchEditorGraphicsItem::PitchEditorGraphicsItem() {
    setBackgroundColor(Qt::transparent);

    auto loadProjectFile = [](const QString &filename, QJsonObject *jsonObj) {
        QFile loadFile(filename);
        if (!loadFile.open(QIODevice::ReadOnly)) {
            qDebug() << "Failed to open project file";
            return false;
        }
        QByteArray allData = loadFile.readAll();
        loadFile.close();
        QJsonParseError err;
        QJsonDocument json = QJsonDocument::fromJson(allData, &err);
        if (err.error != QJsonParseError::NoError)
            return false;
        if (json.isObject()) {
            *jsonObj = json.object();
        }
        return true;
    };

    auto filename = "E:/Test/Param/小小opensvip.json";
    QJsonObject obj;
    if (loadProjectFile(filename, &obj)) {
        auto arrTracks = obj.value("TrackList").toArray();
        auto firstTrack = arrTracks.first().toObject();
        auto objEditedParams = firstTrack.value("EditedParams").toObject();
        auto objPitch = objEditedParams.value("Pitch").toObject();
        auto arrPointList = objPitch.value("PointList").toArray();
        for (const auto valPoint : qAsConst(arrPointList)) {
            auto arrPoint = valPoint.toArray();
            auto pos = arrPoint.first().toInt();
            auto val = arrPoint.last().toInt();
            auto pair = std::make_pair(pos, val);
            opensvipPitchParam.append(pair);
        }
    }
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
double PitchEditorGraphicsItem::scaleX() const {
    return m_scaleX;
}
void PitchEditorGraphicsItem::setScaleX(double scaleX) {
    m_scaleX = scaleX;
    updateRectAndPos();
}
double PitchEditorGraphicsItem::scaleY() const {
    return m_scaleY;
}
void PitchEditorGraphicsItem::setScaleY(double scaleY) {
    m_scaleY = scaleY;
    updateRectAndPos();
}
void PitchEditorGraphicsItem::setVisibleRect(const QRectF &rect) {
    m_visibleRect = rect;
    updateRectAndPos();
}
void PitchEditorGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    OverlayGraphicsItem::paint(painter, option, widget);

    painter->setRenderHint(QPainter::Antialiasing, false);

    QPen pen;
    pen.setWidthF(1);
    pen.setColor(QColor(150, 150, 150));
    painter->setPen(pen);

    auto sceneXToTick = [&](const double pos) { return 480 * pos / m_scaleX / pixelPerQuarterNote; };
    auto tickToSceneX = [&](const double tick) { return tick * m_scaleX * pixelPerQuarterNote / 480; };
    auto sceneXToItemX = [&](const double x) { return mapFromScene(QPointF(x, 0)).x(); };
    auto pitchToSceneY = [&](const double pitch) { return (12700 - pitch + 50) * m_scaleY * noteHeight / 100; };
    auto sceneYToItemY = [&](const double y) { return mapFromScene(QPointF(0, y)).y(); };

    auto visibleStartTick = sceneXToTick(m_visibleRect.left());
    auto visibleEndTick = sceneXToTick(m_visibleRect.right());

    QPainterPath path;
    bool firstPoint = true;
    for (const auto &point : opensvipPitchParam) {
        auto pos = std::get<0>(point) - 480 * 3; // opensvip's "feature"
        if (pos < visibleStartTick)
            continue;
        if (pos > visibleEndTick)
            break;

        auto value = std::get<1>(point);
        auto x = sceneXToItemX(tickToSceneX(pos));
        auto y = sceneYToItemY(pitchToSceneY(value));

        if (firstPoint) {
            path.moveTo(x, y);
            firstPoint = false;
        } else
            path.lineTo(x, y);

        painter->drawPath(path);
    }
}
void PitchEditorGraphicsItem::updateRectAndPos() {
    auto pos = m_visibleRect.topLeft();
    setPos(pos);
    setRect(QRectF(0, 0, m_visibleRect.width(), m_visibleRect.height()));
    update();
}