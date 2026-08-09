#ifndef INFERENCEQUEUECONTROLLER_H
#define INFERENCEQUEUECONTROLLER_H

#include <QFuture>
#include <QObject>
#include <QString>
#include <QVector>

#include <atomic>
#include <functional>

enum class QueueJobStatus {
    Pending,
    Running,
    Completed,
    Failed,
};

struct QueueJob {
    quint64 id = 0;
    QString inputPath;
    QString outputPath;
    int languageId = 0;
    QString languageName;
    double tempo = 120.0;
    QueueJobStatus status = QueueJobStatus::Pending;
    int progress = 0;
    QString error;
};

class InferenceQueueController final : public QObject {
    Q_OBJECT

public:
    using ProgressCallback = std::function<void(int)>;
    using Processor = std::function<bool(const QueueJob &, const ProgressCallback &, QString &)>;

    explicit InferenceQueueController(QObject *parent = nullptr);
    ~InferenceQueueController() override;

    [[nodiscard]] const QVector<QueueJob> &jobs() const;
    [[nodiscard]] bool isRunning() const;

    quint64 addJob(QueueJob job);
    bool updateJob(quint64 id, const QueueJob &job);
    bool removeJob(quint64 id);
    bool moveJob(quint64 id, int offset);
    int applyDefaultsToEditableJobs(int languageId, const QString &languageName, double tempo);
    void resetFailedJobs();

    bool start(Processor processor);
    void stopAfterCurrent();

signals:
    void jobsChanged();
    void runningChanged(bool running);
    void currentJobChanged(quint64 id);
    void queueFinished(bool stopped);

private:
    int indexOf(quint64 id) const;
    void updateJobState(quint64 id, QueueJobStatus status, int progress, const QString &error);
    void updateJobProgress(quint64 id, int progress);

    QVector<QueueJob> m_jobs;
    quint64 m_nextId = 1;
    bool m_running = false;
    std::atomic_bool m_stopRequested{false};
    QFuture<void> m_future;
};

#endif // INFERENCEQUEUECONTROLLER_H
