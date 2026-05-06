#pragma once

/// @file LogNotifier.h
/// @brief QObject bridge that emits Qt signals when log entries are received.

#include <dsfw/Log.h>

#include <QObject>

namespace dstools {

/// @brief QObject wrapper that receives log entries and emits them as Qt signals.
///
/// Install as a log sink to bridge non-Qt Logger into the Qt signal/slot system.
/// The UI (LogPanelWidget) connects to entryAdded() to display live log updates.
///
/// @code
/// Logger::instance().addSink(LogNotifier::instance().sink());
/// connect(&LogNotifier::instance(), &LogNotifier::entryAdded, ...);
/// @endcode
class LogNotifier : public QObject {
    Q_OBJECT

public:
    /// @brief Get the singleton instance.
    static LogNotifier &instance();

    /// @brief Return a LogSink compatible with Logger::addSink().
    [[nodiscard]] LogSink sink();

signals:
    /// @brief Emitted on the receiver's thread when a log entry arrives.
    void entryAdded(const dstools::LogEntry &entry);

private:
    explicit LogNotifier(QObject *parent = nullptr);
    ~LogNotifier() override = default;
};

} // namespace dstools
