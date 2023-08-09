#ifndef AUDIO_SLICER_WORKTHREAD_H
#define AUDIO_SLICER_WORKTHREAD_H

#include <QObject>
#include <QThread>
#include <QRunnable>
#include <QString>
#include <QStringList>

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
               int outputWaveFormat = WF_INT16_PCM,
               int listIndex = -1);
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
    int m_listIndex;

signals:
    void oneFinished(const QString &filename, int listIndex);
    void oneInfo(const QString &infomsg);
    void oneError(const QString &errmsg);
    void oneFailed(const QString &filename, int listIndex);
};


#endif //AUDIO_SLICER_WORKTHREAD_H
