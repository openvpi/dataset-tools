//
// Created by fluty on 2024/1/22.
//

#ifndef SINGINGCLIPGRAPHICSITEM_H
#define SINGINGCLIPGRAPHICSITEM_H

#include "ClipGraphicsItem.h"

class SingingClipGraphicsItem final : public ClipGraphicsItem {
public:
    class Note {
    public:
        int start;
        int length;
        int keyIndex;
        QString lyric;
    };

    explicit SingingClipGraphicsItem(int itemId, QGraphicsItem *parent = nullptr);
    ~SingingClipGraphicsItem() override = default;

    QString audioCachePath() const;
    void setAudioCachePath(const QString &path);

    QVector<Note> notes;

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QString m_audioCachePath;
};



#endif //SINGINGCLIPGRAPHICSITEM_H
