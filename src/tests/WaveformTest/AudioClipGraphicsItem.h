//
// Created by fluty on 2023/11/16.
//

#ifndef AUDIOCLIPGRAPHICSITEM_H
#define AUDIOCLIPGRAPHICSITEM_H

#include "ClipGraphicsItem.h"

class AudioClipGraphicsItem final : public ClipGraphicsItem{
public:
    explicit AudioClipGraphicsItem(QGraphicsItem *parent = nullptr);
    ~AudioClipGraphicsItem();

    // QString audioFilePath() const;
    // void openAudioFile();

// public slots:
//     void onFileLoadComplete(bool success, QString errorMessage);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
};



#endif //AUDIOCLIPGRAPHICSITEM_H
