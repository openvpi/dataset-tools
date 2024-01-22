//
// Created by fluty on 2023/11/16.
//

#ifndef AUDIOCLIPGRAPHICSITEM_H
#define AUDIOCLIPGRAPHICSITEM_H

#include "AudioClipBackgroundWorker.h"
#include "ClipGraphicsItem.h"


#include <sndfile.hh>

class AudioClipGraphicsItem final : public ClipGraphicsItem {
public:
    explicit AudioClipGraphicsItem(int itemId, QGraphicsItem *parent = nullptr);
    ~AudioClipGraphicsItem() override = default;

    void openFile(const QString &path);

    QString path();

    // QString audioFilePath() const;
    // void openAudioFile();

public slots:
    void onLoadComplete(bool success, QString errorMessage);

protected:
    enum RenderResolution { High, Low };

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    // SndfileHandle sf;
    AudioClipBackgroundWorker *m_worker = nullptr;
    QVector<std::tuple<short, short>> m_peakCache;
    QVector<std::tuple<short, short>> m_peakCacheMipmap;
    double m_renderStart = 0;
    double m_renderEnd = 0;
    QPoint m_mouseLastPos;
    int m_rectLastWidth = -1;
    double m_chunksPerTick;
    int m_chunkSize;
    int m_mipmapScale;
    int m_sampleRate;
    int m_channels;
    double m_tempo = 60;
    RenderResolution m_resolution = High;
    bool m_loading = false;
    QString m_path;
};



#endif // AUDIOCLIPGRAPHICSITEM_H
