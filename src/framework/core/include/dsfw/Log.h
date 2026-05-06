#pragma once

/// @file Log.h
/// @brief Lightweight structured logging with severity levels, categories, and pluggable sinks.
///
/// Provides a thread-safe singleton Logger with an extensible sink architecture.
/// Built-in sinks include ring-buffer (UI), Qt message handler, and file output.

#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include <cstdint>

namespace dstools {

/// @brief Log severity levels.
enum class LogLevel { Trace, Debug, Info, Warning, Error, Fatal };

/// @brief A single log entry.
struct LogEntry {
    LogLevel level;
    std::string category; ///< e.g. "infer", "audio", "ui"
    std::string message;
    int64_t timestampMs;  ///< Milliseconds since epoch.

    /// @brief Convert log entry to a single-line display string.
    [[nodiscard]] std::string toString() const;
};

/// @brief Log severity to human-readable label.
[[nodiscard]] const char *logLevelLabel(LogLevel level);

/// @brief Sink callback type — receives log entries for output.
using LogSink = std::function<void(const LogEntry &)>;

/// @brief Global logger with pluggable sinks.
///
/// Thread-safe. Add sinks to receive log output (console, file, memory ring, etc.).
/// @code
/// Logger::instance().addSink(Logger::qtMessageSink());
/// Logger::instance().addSink(Logger::ringBufferSink(500));
/// DSFW_LOG_INFO("audio", "Loaded file");
/// @endcode
class Logger {
public:
    /// @brief Get the singleton instance.
    static Logger &instance();

    /// @brief Add a log sink.
    void addSink(LogSink sink);
    /// @brief Remove all sinks.
    void clearSinks();
    /// @brief Set minimum severity level (entries below are discarded).
    void setMinLevel(LogLevel level);
    /// @brief Return current minimum level.
    LogLevel minLevel() const;

    /// @brief Log a message.
    void log(LogLevel level, const std::string &category, const std::string &message);
    /// @brief Log a message (C-string overload).
    void log(LogLevel level, const char *category, const char *message);

    // ── Built-in Sinks ──────────────────────────────────────────────────

    /// @brief Create a sink that forwards to Qt message handler (qDebug/qWarning/etc).
    static LogSink qtMessageSink();

    /// @brief Create a sink that stores entries in a thread-safe ring buffer.
    /// The buffer is accessible via recentEntries() and emits via LogNotifier.
    /// Only call once (repeated calls share the same buffer).
    /// @param capacity Maximum number of entries retained.
    static LogSink ringBufferSink(int capacity = 500);

    /// @brief Create a sink that writes entries to a plain-text file.
    /// Appends to the file; does NOT rotate.
    /// @param filePath Absolute path to the log file.
    static LogSink fileSink(const std::string &filePath);

    // ── Recent entries (for UI consumption) ─────────────────────────────

    /// @brief Snapshot of recent log entries from the ring buffer.
    /// @param maxCount Maximum entries to return.
    /// @return Copy of recent entries (newest last).
    [[nodiscard]] std::vector<LogEntry> recentEntries(int maxCount = 500) const;

    /// @brief Clear the ring buffer.
    void clearRingBuffer();

private:
    Logger() = default;
    std::vector<LogSink> m_sinks;
    LogLevel m_minLevel = LogLevel::Debug;
    mutable std::mutex m_mutex;
};

} // namespace dstools

// ── Convenience macros ──────────────────────────────────────────────────
#define DSFW_LOG(level, cat, msg) ::dstools::Logger::instance().log(level, cat, msg)
#define DSFW_LOG_TRACE(cat, msg) DSFW_LOG(::dstools::LogLevel::Trace, cat, msg)
#define DSFW_LOG_DEBUG(cat, msg) DSFW_LOG(::dstools::LogLevel::Debug, cat, msg)
#define DSFW_LOG_INFO(cat, msg)  DSFW_LOG(::dstools::LogLevel::Info, cat, msg)
#define DSFW_LOG_WARN(cat, msg)  DSFW_LOG(::dstools::LogLevel::Warning, cat, msg)
#define DSFW_LOG_ERROR(cat, msg) DSFW_LOG(::dstools::LogLevel::Error, cat, msg)
