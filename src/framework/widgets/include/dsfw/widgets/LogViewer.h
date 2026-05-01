#pragma once

#include <dsfw/widgets/WidgetsGlobal.h>

#include <QWidget>

class QPlainTextEdit;
class QComboBox;
class QCheckBox;

namespace dsfw::widgets {

class DSFW_WIDGETS_API LogViewer : public QWidget {
    Q_OBJECT
public:
    enum LogLevel { Debug, Info, Warning, Error };

    explicit LogViewer(QWidget *parent = nullptr);

    void appendLog(LogLevel level, const QString &message);
    void clear();

    void setAutoScroll(bool enabled);
    bool autoScroll() const;

    void setMinimumLevel(LogLevel level);
    LogLevel minimumLevel() const;

    int count() const;

public slots:
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
