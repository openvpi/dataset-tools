#include <dsfw/Log.h>

#include <QDateTime>
#include <QDebug>

#include <chrono>

namespace dstools {

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

} // namespace dstools
