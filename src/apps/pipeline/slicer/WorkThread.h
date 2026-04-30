#ifndef AUDIO_SLICER_WORKTHREAD_H
#define AUDIO_SLICER_WORKTHREAD_H

#include <dsfw/AsyncTask.h>

#include <QString>
#include <QStringList>

#include "Enumerations.h"

class WorkThread : public dstools::AsyncTask {
    Q_OBJECT
public:
    WorkThread(const QString &filename,
               const QString &outPath,
               double threshold,
               qint64 minLength,
               qint64 minInterval,
               qint64 hopSize,
               qint64 maxSilKept,
               int outputWaveFormat = WF_INT16_PCM,
               bool saveAudio = true,
               bool saveMarkers = false,
               bool loadMarkers = false,
               bool overwriteMarkers = false,
               int minimumDigits = 3);

protected:
    bool execute(QString &msg) override;

private:
    QString m_outPath;
    double m_threshold;
    qint64 m_minLength;
    qint64 m_minInterval;
    qint64 m_hopSize;
    qint64 m_maxSilKept;
    int m_outputWaveFormat;
    bool m_saveAudio;
    bool m_saveMarkers;
    bool m_loadMarkers;
    bool m_overwriteMarkers;
    int m_minimumDigits;

signals:
    void oneInfo(const QString &infomsg);
    void oneError(const QString &errmsg);
};

#endif //AUDIO_SLICER_WORKTHREAD_H
