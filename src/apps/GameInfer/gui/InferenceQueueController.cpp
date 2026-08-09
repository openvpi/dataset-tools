#include "InferenceQueueController.h"

#include <QMetaObject>
#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>
#include <exception>
#include <utility>

InferenceQueueController::InferenceQueueController(QObject *parent) : QObject(parent) {}

InferenceQueueController::~InferenceQueueController() {
    m_stopRequested.store(true);
    if (m_future.isRunning()) {
        m_future.waitForFinished();
    }
}

const QVector<QueueJob> &InferenceQueueController::jobs() const { return m_jobs; }

bool InferenceQueueController::isRunning() const { return m_running; }

quint64 InferenceQueueController::addJob(QueueJob job) {
    if (m_running) {
        return 0;
    }

    job.id = m_nextId++;
    job.status = QueueJobStatus::Pending;
    job.progress = 0;
    job.error.clear();
    m_jobs.push_back(std::move(job));
    emit jobsChanged();
    return m_jobs.constLast().id;
}

bool InferenceQueueController::updateJob(const quint64 id, const QueueJob &job) {
    if (m_running) {
        return false;
    }

    const int index = indexOf(id);
    if (index < 0) {
        return false;
    }

    QueueJob updated = job;
    updated.id = id;
    updated.status = m_jobs[index].status == QueueJobStatus::Completed ? QueueJobStatus::Completed
                                                                      : QueueJobStatus::Pending;
    updated.progress = updated.status == QueueJobStatus::Completed ? 100 : 0;
    updated.error.clear();
    m_jobs[index] = std::move(updated);
    emit jobsChanged();
    return true;
}

bool InferenceQueueController::removeJob(const quint64 id) {
    if (m_running) {
        return false;
    }

    const int index = indexOf(id);
    if (index < 0) {
        return false;
    }
    m_jobs.removeAt(index);
    emit jobsChanged();
    return true;
}

bool InferenceQueueController::moveJob(const quint64 id, const int offset) {
    if (m_running || offset == 0) {
        return false;
    }

    const int from = indexOf(id);
    const int to = from + offset;
    if (from < 0 || to < 0 || to >= m_jobs.size()) {
        return false;
    }
    m_jobs.move(from, to);
    emit jobsChanged();
    return true;
}

int InferenceQueueController::applyDefaultsToEditableJobs(const int languageId,
                                                          const QString &languageName,
                                                          const double tempo) {
    if (m_running) {
        return 0;
    }

    int updatedCount = 0;
    for (auto &job : m_jobs) {
        if (job.status != QueueJobStatus::Pending && job.status != QueueJobStatus::Failed) {
            continue;
        }
        job.languageId = languageId;
        job.languageName = languageName;
        job.tempo = tempo;
        ++updatedCount;
    }
    if (updatedCount > 0) {
        emit jobsChanged();
    }
    return updatedCount;
}

void InferenceQueueController::resetFailedJobs() {
    if (m_running) {
        return;
    }

    bool changed = false;
    for (auto &job : m_jobs) {
        if (job.status == QueueJobStatus::Failed) {
            job.status = QueueJobStatus::Pending;
            job.progress = 0;
            job.error.clear();
            changed = true;
        }
    }
    if (changed) {
        emit jobsChanged();
    }
}

bool InferenceQueueController::start(Processor processor) {
    if (m_running || m_future.isRunning() || !processor) {
        return false;
    }

    QVector<QueueJob> pendingJobs;
    for (const auto &job : m_jobs) {
        if (job.status == QueueJobStatus::Pending) {
            pendingJobs.push_back(job);
        }
    }
    if (pendingJobs.isEmpty()) {
        return false;
    }

    m_stopRequested.store(false);
    m_running = true;
    emit runningChanged(true);

    m_future = QtConcurrent::run([this, pendingJobs = std::move(pendingJobs), processor = std::move(processor)] {
        bool stopped = false;

        for (const auto &job : pendingJobs) {
            QMetaObject::invokeMethod(
                this,
                [this, id = job.id] {
                    updateJobState(id, QueueJobStatus::Running, 0, {});
                    emit currentJobChanged(id);
                },
                Qt::QueuedConnection);

            QString error;
            bool success = false;
            try {
                success = processor(
                    job,
                    [this, id = job.id](const int progress) {
                        QMetaObject::invokeMethod(
                            this, [this, id, progress] { updateJobProgress(id, progress); }, Qt::QueuedConnection);
                    },
                    error);
            } catch (const std::exception &exception) {
                error = QString::fromLocal8Bit(exception.what());
            } catch (...) {
                error = tr("Unknown conversion error");
            }
            if (!success && error.trimmed().isEmpty()) {
                error = tr("The conversion failed without an error message.");
            }

            QMetaObject::invokeMethod(
                this,
                [this, id = job.id, success, error] {
                    updateJobState(id, success ? QueueJobStatus::Completed : QueueJobStatus::Failed,
                                   success ? 100 : 0, error);
                },
                Qt::QueuedConnection);

            if (m_stopRequested.load()) {
                stopped = true;
                break;
            }
        }

        QMetaObject::invokeMethod(
            this,
            [this, stopped] {
                m_running = false;
                emit currentJobChanged(0);
                emit runningChanged(false);
                emit queueFinished(stopped);
            },
            Qt::QueuedConnection);
    });

    return true;
}

void InferenceQueueController::stopAfterCurrent() {
    if (m_running) {
        m_stopRequested.store(true);
    }
}

int InferenceQueueController::indexOf(const quint64 id) const {
    for (int index = 0; index < m_jobs.size(); ++index) {
        if (m_jobs[index].id == id) {
            return index;
        }
    }
    return -1;
}

void InferenceQueueController::updateJobState(const quint64 id, const QueueJobStatus status, const int progress,
                                              const QString &error) {
    const int index = indexOf(id);
    if (index < 0) {
        return;
    }
    m_jobs[index].status = status;
    m_jobs[index].progress = std::clamp(progress, 0, 100);
    m_jobs[index].error = error;
    emit jobsChanged();
}

void InferenceQueueController::updateJobProgress(const quint64 id, const int progress) {
    const int index = indexOf(id);
    if (index < 0 || m_jobs[index].status != QueueJobStatus::Running) {
        return;
    }
    m_jobs[index].progress = std::clamp(progress, 0, 100);
    emit jobsChanged();
}
