//
// Created by fluty on 2024/1/22.
//

#ifndef SINGINGCLIPGRAPHICSITEM_H
#define SINGINGCLIPGRAPHICSITEM_H

#include "AbstractClipGraphicsItem.h"

class SingingClipGraphicsItem final : public AbstractClipGraphicsItem {
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

private:
    // void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void drawPreviewArea(QPainter *painter, const QRectF &previewRect, int opacity) override;
    QString clipTypeName() override {
        return "[Singing] ";
    }

    QString m_audioCachePath;
};



#endif //SINGINGCLIPGRAPHICSITEM_H
