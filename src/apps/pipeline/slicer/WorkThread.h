/// @file WorkThread.h
/// @brief Async task wrapper for audio slicing operations.

#pragma once
#include <dsfw/AsyncTask.h>

#include <QString>
#include <QStringList>

#include "Enumerations.h"
#include "SliceJob.h"

/// @brief AsyncTask subclass that runs a SliceJob in the background.
class WorkThread : public dstools::AsyncTask {
    Q_OBJECT
public:
    /// @param filename Path to the input audio file.
    /// @param outPath Output directory for sliced chunks.
    /// @param params Slicing parameters.
    WorkThread(const QString &filename,
               const QString &outPath,
               const SliceJobParams &params);

protected:
    /// @brief Execute the slice job.
    /// @param[out] msg Status or error message.
    /// @return True if the job completed successfully.
    bool execute(QString &msg) override;

private:
    QString m_outPath;       ///< Output directory path.
    SliceJobParams m_params; ///< Slicing parameters.

signals:
    /// @brief Emitted with informational messages during slicing.
    void oneInfo(const QString &infomsg);
    /// @brief Emitted with error messages during slicing.
    void oneError(const QString &errmsg);
};
