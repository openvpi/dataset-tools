#pragma once
/// @file FileLogSink.h
/// @brief File-based log sink creation and log rotation utilities.

#include <dsfw/Log.h>

#include <QString>

namespace dsfw {

/// @brief Create a log sink that writes to a timestamped file.
/// @param logDir Directory to store log files.
/// @param appName Application name used in the log filename.
/// @return Configured log sink.
dstools::LogSink createFileLogSink(const QString &logDir, const QString &appName);

/// @brief Delete log files older than the specified age.
/// @param logDir Directory containing log files.
/// @param maxAgeDays Maximum age in days before deletion (default 7).
void cleanOldLogFiles(const QString &logDir, int maxAgeDays = 7);

} // namespace dsfw
