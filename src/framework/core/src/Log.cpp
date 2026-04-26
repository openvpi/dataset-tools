#include <dsfw/Log.h>

#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QTextStream>

#include <chrono>
#include <deque>
#include <memory>

namespace dstools {

const char *logLevelLabel(LogLevel level) {
    switch (level) {
        case LogLevel::Trace:   return "TRACE";
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
    }
    return "?";
}

std::string LogEntry::toString() const {
    // "2026-05-06 14:30:01.234 [WARN audio] message"
    auto time = QDateTime::fromMSecsSinceEpoch(timestampMs);
    return time.toString("yyyy-MM-dd HH:mm:ss.zzz").toStdString()
         + " [" + logLevelLabel(level) + " " + category + "] " + message;
}

// ── Singleton ───────────────────────────────────────────────────────────

Logger &Logger::instance() {
    static Logger s_instance;
    return s_instance;
}

void Logger::addSink(LogSink sink) {
    std::lock_guard lock(m_mutex);
    m_sinks.push_back(std::move(sink));
}

void Logger::clearSinks() {
    std::lock_guard lock(m_mutex);
    m_sinks.clear();
}

void Logger::setMinLevel(LogLevel level) {
    std::lock_guard lock(m_mutex);
    m_minLevel = level;
}

LogLevel Logger::minLevel() const {
    std::lock_guard lock(m_mutex);
    return m_minLevel;
}

void Logger::log(LogLevel level, const std::string &category, const std::string &message) {
    std::vector<LogSink> sinks;
    {
        std::lock_guard lock(m_mutex);
        if (level < m_minLevel)
            return;
        sinks = m_sinks;
    }

    LogEntry entry;
    entry.level = level;
    entry.category = category;
    entry.message = message;
    entry.timestampMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();

    for (auto &sink : sinks)
        sink(entry);
}

void Logger::log(LogLevel level, const char *category, const char *message) {
    log(level, std::string(category), std::string(message));
}

// ── Built-in sinks ──────────────────────────────────────────────────────

LogSink Logger::qtMessageSink() {
    return [](const LogEntry &entry) {
        auto msg = QString("[%1] %2").arg(
            QString::fromStdString(entry.category), QString::fromStdString(entry.message));
        switch (entry.level) {
            case LogLevel::Trace:
            case LogLevel::Debug:
                qDebug().noquote() << msg;
                break;
            case LogLevel::Info:
                qInfo().noquote() << msg;
                break;
            case LogLevel::Warning:
                qWarning().noquote() << msg;
                break;
            case LogLevel::Error:
            case LogLevel::Fatal:
                qCritical().noquote() << msg;
                break;
        }
    };
}

// Ring buffer shared state
static struct RingBufferState {
    std::deque<LogEntry> entries;
    int capacity = 500;
    mutable std::mutex mutex;
} g_ringBuffer;

LogSink Logger::ringBufferSink(int capacity) {
    g_ringBuffer.capacity = capacity;
    return [](const LogEntry &entry) {
        std::lock_guard lock(g_ringBuffer.mutex);
        g_ringBuffer.entries.push_back(entry);
        while (static_cast<int>(g_ringBuffer.entries.size()) > g_ringBuffer.capacity)
            g_ringBuffer.entries.pop_front();
    };
}

LogSink Logger::fileSink(const std::string &filePath) {
    // Capture by value — file path is immutable
    return [filePath](const LogEntry &entry) {
        QFile file(QString::fromStdString(filePath));
        if (file.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream stream(&file);
            stream << QString::fromStdString(entry.toString()) << "\n";
        }
    };
}

std::vector<LogEntry> Logger::recentEntries(int maxCount) const {
    std::lock_guard lock(g_ringBuffer.mutex);
    std::vector<LogEntry> result;
    result.reserve(std::min(maxCount, static_cast<int>(g_ringBuffer.entries.size())));
    // Copy from oldest to newest
    int start = std::max(0, static_cast<int>(g_ringBuffer.entries.size()) - maxCount);
    for (int i = start; i < static_cast<int>(g_ringBuffer.entries.size()); ++i)
        result.push_back(g_ringBuffer.entries[i]);
    return result;
}

void Logger::clearRingBuffer() {
    std::lock_guard lock(g_ringBuffer.mutex);
    g_ringBuffer.entries.clear();
}

} // namespace dstools
