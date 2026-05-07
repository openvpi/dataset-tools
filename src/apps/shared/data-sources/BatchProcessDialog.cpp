#include "BatchProcessDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTextEdit>
#include <QTime>
#include <QToolButton>
#include <QVBoxLayout>

namespace dstools {

BatchProcessDialog::BatchProcessDialog(const QString &title, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(title);
    setMinimumSize(520, 420);

    auto *mainLayout = new QVBoxLayout(this);

    m_paramLayout = new QVBoxLayout();
    mainLayout->addLayout(m_paramLayout);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(100);
    m_progressBar->setValue(0);
    mainLayout->addWidget(m_progressBar);

    m_statusLabel = new QLabel(this);
    mainLayout->addWidget(m_statusLabel);

    m_logOutput = new QTextEdit(this);
    m_logOutput->setReadOnly(true);
    mainLayout->addWidget(m_logOutput, 1);

    auto *btnLayout = new QHBoxLayout();
    m_startBtn = new QPushButton(tr("Start"), this);
    m_cancelBtn = new QPushButton(tr("Cancel"), this);
    m_closeBtn = new QPushButton(tr("Close"), this);
    m_cancelBtn->setEnabled(false);
    m_closeBtn->setEnabled(false);
    btnLayout->addStretch(1);
    btnLayout->addWidget(m_startBtn);
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_closeBtn);
    mainLayout->addLayout(btnLayout);

    connect(m_startBtn, &QPushButton::clicked, this, [this]() {
        m_running = true;
        m_cancelled = false;
        m_startBtn->setEnabled(false);
        m_cancelBtn->setEnabled(true);
        m_closeBtn->setEnabled(false);
        m_progressBar->setValue(0);
        m_logOutput->clear();
        m_statusLabel->setText(tr("Processing..."));
        emit started();
    });

    connect(m_cancelBtn, &QPushButton::clicked, this, &BatchProcessDialog::onCancelClicked);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

BatchProcessDialog::~BatchProcessDialog() = default;

void BatchProcessDialog::addParamWidget(QWidget *widget) {
    m_paramLayout->addWidget(widget);
    auto *separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    m_paramLayout->addWidget(separator);
}

void BatchProcessDialog::addParamRow(const QString &label, QWidget *widget) {
    auto *row = new QHBoxLayout();
    auto *lbl = new QLabel(label, this);
    lbl->setBuddy(widget);
    row->addWidget(lbl);
    row->addWidget(widget, 1);
    m_paramLayout->addLayout(row);
    auto *separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    m_paramLayout->addWidget(separator);
}

void BatchProcessDialog::addParamGroup(const QString &title) {
    auto *content = new QWidget(this);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);

    auto *toggleBtn = new QToolButton(this);
    toggleBtn->setText(title);
    toggleBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toggleBtn->setArrowType(Qt::DownArrow);
    toggleBtn->setCheckable(true);
    toggleBtn->setChecked(true);
    toggleBtn->setStyleSheet(QStringLiteral("QToolButton { border: none; font-weight: bold; }"));

    m_paramLayout->addWidget(toggleBtn);
    m_paramLayout->addWidget(content);

    connect(toggleBtn, &QToolButton::clicked, this, [this, toggleBtn, content]() {
        toggleParamGroup(toggleBtn, content);
    });
}

void BatchProcessDialog::toggleParamGroup(QToolButton *toggleBtn, QWidget *content) {
    bool expanded = toggleBtn->isChecked();
    content->setVisible(expanded);
    toggleBtn->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
}

void BatchProcessDialog::appendLog(const QString &message) {
    appendLog(LogLevel::Info, message);
}

void BatchProcessDialog::appendLog(LogLevel level, const QString &message) {
    const QString timestamp = QTime::currentTime().toString(QStringLiteral("[HH:MM:SS]"));

    QString levelStr;
    QString color;
    switch (level) {
    case LogLevel::Warning:
        levelStr = QStringLiteral("[WARN]");
        color = QStringLiteral("#B8860B");
        break;
    case LogLevel::Error:
        levelStr = QStringLiteral("[ERROR]");
        color = QStringLiteral("#CC0000");
        break;
    case LogLevel::Info:
    default:
        levelStr = QStringLiteral("[INFO]");
        color = QStringLiteral("#333333");
        break;
    }

    const QString html = QStringLiteral("<span style=\"color:%1\">%2 %3 %4</span>")
                             .arg(color, timestamp, levelStr, message.toHtmlEscaped());
    m_logOutput->append(html);
}

void BatchProcessDialog::setProgress(int current, int total) {
    if (total > 0)
        m_progressBar->setValue(current * 100 / total);
    m_statusLabel->setText(tr("Processing %1 / %2...").arg(current).arg(total));
}

void BatchProcessDialog::finish(int processed, int skipped, int errors) {
    m_running = false;
    m_startBtn->setEnabled(true);
    m_cancelBtn->setEnabled(false);
    m_closeBtn->setEnabled(true);
    m_progressBar->setValue(100);

    QString msg = tr("Completed: %1 processed").arg(processed);
    if (skipped > 0)
        msg += tr(", %1 skipped").arg(skipped);
    if (errors > 0)
        msg += tr(", %1 errors").arg(errors);
    m_statusLabel->setText(msg);
    appendLog(msg);
}

bool BatchProcessDialog::isCancelled() const {
    return m_cancelled;
}

void BatchProcessDialog::onCancelClicked() {
    m_cancelled = true;
    m_cancelBtn->setEnabled(false);
    m_statusLabel->setText(tr("Cancelling..."));
    appendLog(tr("Cancellation requested..."));
    emit cancelled();
}

void BatchProcessDialog::reject() {
    if (m_running) {
        onCancelClicked();
        return;
    }
    QDialog::reject();
}

} // namespace dstools
