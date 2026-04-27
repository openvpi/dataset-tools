#include <dstools/FileProgressTracker.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>

namespace dstools::widgets {

FileProgressTracker::FileProgressTracker(DisplayStyle style, QWidget *parent)
    : QWidget(parent), m_style(style),
      m_format(QStringLiteral("%1 / %2 (%3%)")),
      m_emptyText(QStringLiteral("No files")) {
    if (m_style == LabelOnly) {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        m_label = new QLabel(this);
        m_label->setStyleSheet(
            "background-color: #22222C; color: #9898A8; font-size: 10px; padding: 4px 8px; border-top: 1px solid #33333E;");
        m_label->setAlignment(Qt::AlignCenter);
        layout->addWidget(m_label);
    } else {
        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);

        m_label = new QLabel(QStringLiteral("progress:"), this);
        layout->addWidget(m_label);

        m_progressBar = new QProgressBar(this);
        m_progressBar->setRange(0, 100);
        m_progressBar->setValue(0);
        layout->addWidget(m_progressBar);
    }

    updateDisplay();
}

FileProgressTracker::~FileProgressTracker() = default;

void FileProgressTracker::setProgress(int completed, int total) {
    m_completed = completed;
    m_total = total;
    updateDisplay();
}

void FileProgressTracker::setDisplayStyle(DisplayStyle style) {
    if (m_style == style)
        return;
    m_style = style;

    // Remove existing layout and widgets
    delete layout();
    delete m_label;
    m_label = nullptr;
    delete m_progressBar;
    m_progressBar = nullptr;

    if (m_style == LabelOnly) {
        auto *newLayout = new QVBoxLayout(this);
        newLayout->setContentsMargins(0, 0, 0, 0);
        newLayout->setSpacing(0);

        m_label = new QLabel(this);
        m_label->setStyleSheet(
            "background-color: #22222C; color: #9898A8; font-size: 10px; padding: 4px 8px; border-top: 1px solid #33333E;");
        m_label->setAlignment(Qt::AlignCenter);
        newLayout->addWidget(m_label);
    } else {
        auto *newLayout = new QHBoxLayout(this);
        newLayout->setContentsMargins(0, 0, 0, 0);

        m_label = new QLabel(QStringLiteral("progress:"), this);
        newLayout->addWidget(m_label);

        m_progressBar = new QProgressBar(this);
        m_progressBar->setRange(0, 100);
        m_progressBar->setValue(0);
        newLayout->addWidget(m_progressBar);
    }

    updateDisplay();
}

void FileProgressTracker::setFormat(const QString &format) {
    m_format = format;
    updateDisplay();
}

void FileProgressTracker::setEmptyText(const QString &text) {
    m_emptyText = text;
    updateDisplay();
}

void FileProgressTracker::updateDisplay() {
    if (m_total == 0) {
        if (m_style == LabelOnly) {
            m_label->setText(m_emptyText);
        } else {
            m_progressBar->setValue(0);
        }
        return;
    }

    const int percent = m_total > 0 ? m_completed * 100 / m_total : 0;

    if (m_style == LabelOnly) {
        m_label->setText(m_format.arg(m_completed).arg(m_total).arg(percent));
    } else {
        m_progressBar->setValue(percent);
    }
}

} // namespace dstools::widgets
