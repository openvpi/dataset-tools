//
// Created by fluty on 2023/11/16.
//

#ifndef AUDIOCLIPGRAPHICSITEM_H
#define AUDIOCLIPGRAPHICSITEM_H

#include "ClipGraphicsItem.h"


#include <sndfile.hh>

class AudioClipGraphicsItem final : public ClipGraphicsItem{
public:
    enum RenderMode {
        Peak,
        Line,
        Sample
    };

    explicit AudioClipGraphicsItem(int itemId , QGraphicsItem *parent = nullptr);
    ~AudioClipGraphicsItem();

    bool openFile(const QString &path);

    RenderMode renderMode();
    void setRenderMode(RenderMode mode);

    // QString audioFilePath() const;
    // void openAudioFile();

// public slots:
//     void onFileLoadComplete(bool success, QString errorMessage);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    SndfileHandle sf;
    QVector<std::tuple<double, double>> m_peakCache;
    double m_renderStart = 0;
    double m_renderEnd = 0;
    double m_scale = 1.0;
    QPoint m_mouseLastPos;
    int m_rectLastWidth = -1;
    RenderMode m_renderMode = Peak;
    int chunksPerTick = 200;
};



#endif //AUDIOCLIPGRAPHICSITEM_H
