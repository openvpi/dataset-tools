#pragma once

#include <dsfw/Log.h>

#include <QString>

namespace dsfw {

dstools::LogSink createFileLogSink(const QString &logDir, const QString &appName);

void cleanOldLogFiles(const QString &logDir, int maxAgeDays = 7);

} // namespace dsfw
