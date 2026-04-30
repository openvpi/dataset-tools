#ifndef DSFW_ASYNCTASK_H
#define DSFW_ASYNCTASK_H

/// @file AsyncTask.h
/// @brief QRunnable+QObject base class for thread-pool async tasks with signal-based completion.

#include <QObject>
#include <QRunnable>
#include <QString>

namespace dstools {

    /// @brief Abstract base for tasks executed on QThreadPool with signal-based results.
    ///
    /// Subclass and override execute(). The run() method calls execute() and emits
    /// succeeded() or failed() depending on the return value.
    ///
    /// @note The task is auto-deleted by QThreadPool after run() completes.
    class AsyncTask : public QObject, public QRunnable {
        Q_OBJECT
    public:
        /// @brief Construct an async task.
        /// @param identifier Unique identifier for this task instance.
        /// @param parent Optional QObject parent.
        explicit AsyncTask(QString identifier, QObject *parent = nullptr);
        ~AsyncTask() override = default;

        /// @brief Entry point called by QThreadPool. Calls execute() internally.
        void run() override final;

        /// @brief Return the task's identifier.
        /// @return Task identifier string.
        const QString &identifier() const;

    protected:
        /// @brief Implement the task's work here.
        /// @param msg Output message (success description or error detail).
        /// @return True on success, false on failure.
        virtual bool execute(QString &msg) = 0;

    signals:
        /// @brief Emitted when execute() returns true.
        /// @param identifier Task identifier.
        /// @param msg Success message.
        void succeeded(const QString &identifier, const QString &msg);
        /// @brief Emitted when execute() returns false.
        /// @param identifier Task identifier.
        /// @param msg Error message.
        void failed(const QString &identifier, const QString &msg);

    private:
        QString m_identifier;
    };

} // namespace dstools

#endif // DSFW_ASYNCTASK_H
