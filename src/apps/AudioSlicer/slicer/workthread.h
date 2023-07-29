#ifndef AUDIO_SLICER_WORKTHREAD_H
#define AUDIO_SLICER_WORKTHREAD_H

#include <QObject>
#include <QThread>
#include <QRunnable>
#include <QString>
#include <QStringList>
#include <QPluginLoader>

#include <Api/IAudioDecoder.h>
#include <Api/interfaces/IAudioDecoderPlugin.h>

#include "waveformat.h"

class WorkThread : public QObject, public QRunnable {
Q_OBJECT
public:
    WorkThread(QString filename,
               QString outPath,
               double threshold,
               qint64 minLength,
               qint64 minInterval,
               qint64 hopSize,
               qint64 maxSilKept,
               int outputWaveFormat = WF_INT16_PCM);
    void run() override;

private:
    QString m_filename;
    QString m_outPath;
    double m_threshold;
    qint64 m_minLength;
    qint64 m_minInterval;
    qint64 m_hopSize;
    qint64 m_maxSilKept;
    int m_outputWaveFormat;

    // QPluginLoader m_decoderLoader;

    // QsApi::IAudioDecoderPlugin *m_decoderPlugin;
    QsApi::IAudioDecoder *m_decoder;

    void initPlugins();
    void uninitPlugins();

signals:
    void oneFinished(const QString &filename);
    void oneInfo(const QString &infomsg);
    void oneError(const QString &errmsg);
};


#endif //AUDIO_SLICER_WORKTHREAD_H
