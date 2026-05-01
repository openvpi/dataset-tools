#pragma once
/// @file LogViewer.h
/// @brief Filterable log message viewer widget with auto-scroll support.

#include <dsfw/widgets/WidgetsGlobal.h>

#include <QWidget>

class QPlainTextEdit;
class QComboBox;
class QCheckBox;

namespace dsfw::widgets {

/// @brief Filterable log viewer widget with level filtering and auto-scroll.
class DSFW_WIDGETS_API LogViewer : public QWidget {
    Q_OBJECT
public:
    /// @brief Log severity level.
    enum LogLevel { Debug, Info, Warning, Error };

    /// @brief Construct a LogViewer.
    /// @param parent Parent widget.
    explicit LogViewer(QWidget *parent = nullptr);

    /// @brief Append a log message.
    /// @param level Severity level.
    /// @param message Log message text.
    void appendLog(LogLevel level, const QString &message);

    /// @brief Clear all log entries.
    void clear();

    /// @brief Enable or disable auto-scrolling to the latest entry.
    /// @param enabled True to enable auto-scroll.
    void setAutoScroll(bool enabled);

    /// @brief Check whether auto-scroll is enabled.
    /// @return True if auto-scroll is on.
    bool autoScroll() const;

    /// @brief Set the minimum severity level to display.
    /// @param level Minimum log level.
    void setMinimumLevel(LogLevel level);

    /// @brief Get the current minimum display level.
    /// @return Minimum log level.
    LogLevel minimumLevel() const;

    /// @brief Get the total number of log entries.
    /// @return Entry count.
    int count() const;

public slots:
    /// @brief Set the filter level from a combo box index.
    /// @param level Level index.
    void setFilterLevel(int level);

private:
    void rebuildDisplay();

    QPlainTextEdit *m_textEdit;
    QComboBox *m_filterCombo;
    QCheckBox *m_autoScrollCheck;

    struct LogEntry {
        LogLevel level;
        QString message;
    };
    QList<LogEntry> m_entries;
    LogLevel m_minLevel = Debug;
    bool m_autoScroll = true;
};

} // namespace dsfw::widgets
