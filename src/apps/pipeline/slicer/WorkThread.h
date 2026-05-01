#ifndef AUDIO_SLICER_WORKTHREAD_H
#define AUDIO_SLICER_WORKTHREAD_H

#include <dsfw/AsyncTask.h>

#include <QString>
#include <QStringList>

#include "Enumerations.h"
#include "SliceJob.h"

class WorkThread : public dstools::AsyncTask {
    Q_OBJECT
public:
    WorkThread(const QString &filename,
               const QString &outPath,
               const SliceJobParams &params);

protected:
    bool execute(QString &msg) override;

private:
    QString m_outPath;
    SliceJobParams m_params;

signals:
    void oneInfo(const QString &infomsg);
    void oneError(const QString &errmsg);
};

#endif //AUDIO_SLICER_WORKTHREAD_H
