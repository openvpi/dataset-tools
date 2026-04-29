#include <dstools/RunProgressRow.h>

#include <QHBoxLayout>
#include <QProgressBar>
#include <QPushButton>

namespace dstools::widgets {

RunProgressRow::RunProgressRow(const QString &buttonText, QWidget *parent)
    : QWidget(parent), m_runText(buttonText) {
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_button = new QPushButton(m_runText, this);
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setVisible(false);

    layout->addWidget(m_button);
    layout->addWidget(m_progressBar, 1);

    connect(m_button, &QPushButton::clicked, this, [this]() {
        if (m_running)
            emit cancelClicked();
        else
            emit runClicked();
    });
}

void RunProgressRow::setProgress(int percent) {
    m_progressBar->setValue(qBound(0, percent, 100));
}

void RunProgressRow::setRunning(bool running) {
    m_running = running;
    m_button->setText(running ? tr("Cancel") : m_runText);
    m_progressBar->setVisible(running);
    if (!running)
        m_progressBar->setValue(0);
}

void RunProgressRow::setButtonEnabled(bool enabled) {
    m_button->setEnabled(enabled);
}

void RunProgressRow::reset() {
    setRunning(false);
    setButtonEnabled(true);
    m_progressBar->setValue(0);
}

} // namespace dstools::widgets
