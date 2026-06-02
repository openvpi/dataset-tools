#pragma once
/// @file AsyncTask.h
/// @brief QRunnable+QObject base class for thread-pool async tasks with signal-based completion.

#include <QObject>
#include <QRunnable>
#include <QString>
#include <atomic>
#include <chrono>

namespace dstools {

    /// @brief Abstract base for tasks executed on QThreadPool with signal-based results.
    ///
    /// Subclass and override execute(). The run() method calls execute() and emits
    /// succeeded() or failed() depending on the return value.
    ///
    /// @note autoDelete is set to false to avoid double-free with QObject parent.
    ///       The task calls deleteLater() on itself after emitting succeeded/failed.
    class AsyncTask : public QObject, public QRunnable {
        Q_OBJECT
    public:
        /// @brief Default timeout for async tasks (30 seconds).
        static constexpr auto kDefaultTimeout = std::chrono::milliseconds(30000);

        /// @brief Construct an async task.
        /// @param identifier Unique identifier for this task instance.
        /// @param parent Optional QObject parent.
        explicit AsyncTask(QString identifier, QObject *parent = nullptr) :
            QObject(parent), m_identifier(std::move(identifier)) {
            setAutoDelete(false);
        }
        ~AsyncTask() override = default;

        /// @brief Entry point called by QThreadPool. Calls execute() internally.
        void run() final {
            m_startTime = std::chrono::steady_clock::now();
            QString msg;
            if (execute(msg)) {
                Q_EMIT succeeded(m_identifier, msg);
            } else {
                Q_EMIT failed(m_identifier, msg);
            }
            m_finished.store(true, std::memory_order_release);
            deleteLater();
        }

        /// @brief Return the task's identifier.
        const QString &identifier() const { return m_identifier; }

        /// @brief Set the timeout duration for this task.
        void setTimeout(std::chrono::milliseconds timeout) { m_timeout = timeout; }

        /// @brief Get the current timeout setting.
        std::chrono::milliseconds timeout() const { return m_timeout; }

        /// @brief Request cancellation. Subclasses should check isCanceled() periodically.
        void requestCancel() { m_canceled.store(true, std::memory_order_release); }

        /// @brief Check if cancellation has been requested.
        [[nodiscard]] bool isCanceled() const { return m_canceled.load(std::memory_order_acquire); }

        /// @brief Check if the task has completed.
        [[nodiscard]] bool isFinished() const { return m_finished.load(std::memory_order_acquire); }

        /// @brief Check if the task has exceeded its timeout. Subclasses should call this periodically.
        [[nodiscard]] bool hasTimedOut() const {
            if (m_timeout == std::chrono::milliseconds::zero())
                return false;
            return std::chrono::steady_clock::now() - m_startTime > m_timeout;
        }

    protected:
        /// @brief Implement the task's work here.
        /// @param msg Output message (success description or error detail).
        /// @return True on success, false on failure.
        virtual bool execute(QString &msg) = 0;

    signals:
        /// @brief Emitted when execute() returns true.
        void succeeded(const QString &identifier, const QString &msg);
        /// @brief Emitted when execute() returns false.
        void failed(const QString &identifier, const QString &msg);
        /// @brief Emitted when the task exceeds its timeout.
        void timedOut(const QString &identifier);

    private:
        QString m_identifier;
        std::atomic<bool> m_canceled{false};
        std::atomic<bool> m_finished{false};
        std::chrono::steady_clock::time_point m_startTime{};
        std::chrono::milliseconds m_timeout{kDefaultTimeout};
    };

} // namespace dstools
