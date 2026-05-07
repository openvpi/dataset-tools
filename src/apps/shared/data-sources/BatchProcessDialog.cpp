#include "BatchProcessDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
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

    m_logOutput = new QPlainTextEdit(this);
    m_logOutput->setReadOnly(true);
    m_logOutput->setMaximumBlockCount(5000);
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
}

void BatchProcessDialog::addParamRow(const QString &label, QWidget *widget) {
    auto *row = new QHBoxLayout();
    auto *lbl = new QLabel(label, this);
    lbl->setBuddy(widget);
    row->addWidget(lbl);
    row->addWidget(widget, 1);
    m_paramLayout->addLayout(row);
}

void BatchProcessDialog::appendLog(const QString &message) {
    m_logOutput->appendPlainText(message);
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
