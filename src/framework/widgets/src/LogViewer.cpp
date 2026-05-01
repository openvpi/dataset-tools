#include <dsfw/widgets/LogViewer.h>

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QVBoxLayout>

namespace dsfw::widgets {

static QString levelToString(LogViewer::LogLevel level) {
    switch (level) {
    case LogViewer::Debug:   return QStringLiteral("[DEBUG]");
    case LogViewer::Info:    return QStringLiteral("[INFO]");
    case LogViewer::Warning: return QStringLiteral("[WARN]");
    case LogViewer::Error:   return QStringLiteral("[ERROR]");
    }
    return QStringLiteral("[???]");
}

static QString levelColor(LogViewer::LogLevel level) {
    switch (level) {
    case LogViewer::Debug:   return QStringLiteral("gray");
    case LogViewer::Info:    return QStringLiteral("white");
    case LogViewer::Warning: return QStringLiteral("yellow");
    case LogViewer::Error:   return QStringLiteral("red");
    }
    return QStringLiteral("white");
}

LogViewer::LogViewer(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);

    auto *toolbar = new QHBoxLayout();

    m_filterCombo = new QComboBox(this);
    m_filterCombo->addItems({tr("Debug"), tr("Info"), tr("Warning"), tr("Error")});
    m_filterCombo->setCurrentIndex(0);
    toolbar->addWidget(m_filterCombo);

    m_autoScrollCheck = new QCheckBox(tr("Auto-scroll"), this);
    m_autoScrollCheck->setChecked(true);
    toolbar->addWidget(m_autoScrollCheck);

    toolbar->addStretch();
    layout->addLayout(toolbar);

    m_textEdit = new QPlainTextEdit(this);
    m_textEdit->setReadOnly(true);
    m_textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    layout->addWidget(m_textEdit);

    connect(m_filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LogViewer::setFilterLevel);
    connect(m_autoScrollCheck, &QCheckBox::toggled, this, &LogViewer::setAutoScroll);
}

void LogViewer::appendLog(LogLevel level, const QString &message) {
    m_entries.append({level, message});

    if (level < m_minLevel)
        return;

    QString html = QStringLiteral("<span style=\"color:%1;\">%2 %3</span>")
                       .arg(levelColor(level), levelToString(level), message.toHtmlEscaped());
    m_textEdit->appendHtml(html);

    if (m_autoScroll) {
        QScrollBar *bar = m_textEdit->verticalScrollBar();
        bar->setValue(bar->maximum());
    }
}

void LogViewer::clear() {
    m_entries.clear();
    m_textEdit->clear();
}

void LogViewer::setAutoScroll(bool enabled) {
    m_autoScroll = enabled;
    m_autoScrollCheck->setChecked(enabled);
}

bool LogViewer::autoScroll() const {
    return m_autoScroll;
}

void LogViewer::setMinimumLevel(LogLevel level) {
    if (m_minLevel == level)
        return;
    m_minLevel = level;
    m_filterCombo->setCurrentIndex(static_cast<int>(level));
    rebuildDisplay();
}

LogViewer::LogLevel LogViewer::minimumLevel() const {
    return m_minLevel;
}

int LogViewer::count() const {
    return m_entries.size();
}

void LogViewer::setFilterLevel(int level) {
    m_minLevel = static_cast<LogLevel>(level);
    rebuildDisplay();
}

void LogViewer::rebuildDisplay() {
    m_textEdit->clear();
    for (const auto &entry : m_entries) {
        if (entry.level < m_minLevel)
            continue;
        QString html = QStringLiteral("<span style=\"color:%1;\">%2 %3</span>")
                           .arg(levelColor(entry.level), levelToString(entry.level),
                                entry.message.toHtmlEscaped());
        m_textEdit->appendHtml(html);
    }
}

} // namespace dsfw::widgets
