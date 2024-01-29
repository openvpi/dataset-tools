//
// Created by fluty on 2023/11/16.
//

#ifndef AUDIOCLIPGRAPHICSITEM_H
#define AUDIOCLIPGRAPHICSITEM_H

#include "AbstractClipGraphicsItem.h"
#include "AudioClipBackgroundWorker.h"

class AudioClipGraphicsItem final : public AbstractClipGraphicsItem {
public:
    explicit AudioClipGraphicsItem(int itemId, QGraphicsItem *parent = nullptr);
    ~AudioClipGraphicsItem() override = default;

    QString path() const;
    void setPath(const QString &path);
    double tempo() const;
    void setTempo(double tempo);

public slots:
    void onLoadComplete(bool success, QString errorMessage);
    void onTempoChange(double tempo);

private:
    enum RenderResolution { High, Low };

    // void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void drawPreviewArea(QPainter *painter, const QRectF &previewRect, int opacity) override;
    QString clipTypeName() override {
        return "[Audio] ";
    }
    void updateLength();

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
