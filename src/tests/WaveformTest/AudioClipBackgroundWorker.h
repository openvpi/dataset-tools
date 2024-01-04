//
// Created by FlutyDeer on 2023/12/4.
//

#ifndef BACKGROUNDWORKER_H
#define BACKGROUNDWORKER_H

#include <QObject>
#include <QRunnable>
#include <sndfile.hh>

class AudioClipBackgroundWorker : public QObject, public QRunnable {
    Q_OBJECT

public:
    explicit AudioClipBackgroundWorker(const QString &path);
    void setPath(const QString &path);
    void run() override;

    QVector<std::tuple<short, short>> peakCache;

signals:
    // void progressChanged(int progress);
    void finished(bool success, QString errorMessage);

private:
    QString m_path;
    SndfileHandle sf;
    // QVector<std::tuple<double, double>> m_peakCache;
};



#endif // BACKGROUNDWORKER_H
