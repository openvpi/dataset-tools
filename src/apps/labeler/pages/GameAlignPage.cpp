#include "GameAlignPage.h"

#include <QFormLayout>
#include <QMessageBox>
#include <QVBoxLayout>

namespace dstools::labeler {

GameAlignPage::GameAlignPage(QWidget *parent) : QWidget(parent) {
    buildUi();
}

void GameAlignPage::buildUi() {
    auto *vLayout = new QVBoxLayout(this);

    // Parameter panel
    auto *form = new QFormLayout;

    m_modelPath = new dstools::widgets::PathSelector(dstools::widgets::PathSelector::Directory, "",
                                                     "");
    m_modelPath->setPlaceholder(tr("Path to GAME model directory"));
    form->addRow(tr("Model:"), m_modelPath);

    m_gpuSelector = new dstools::widgets::GpuSelector;
    form->addRow(tr("Device:"), m_gpuSelector);

    vLayout->addLayout(form);

    // Run progress row
    m_runProgress = new dstools::widgets::RunProgressRow(tr("Run"));
    vLayout->addWidget(m_runProgress);

    // Log area
    m_log = new QTextEdit;
    m_log->setReadOnly(true);
    vLayout->addWidget(m_log, 1);

    // Run action
    m_runAction = new QAction(tr("Run MIDI Alignment"), this);
    m_runAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    auto runSlot = [this]() {
        QMessageBox::information(this, tr("Not Implemented"),
                                 tr("GAME audio-to-MIDI alignment is not yet implemented.\n\n"
                                    "This step will use the GAME model to generate MIDI "
                                    "transcriptions from audio files."));
    };
    connect(m_runProgress, &dstools::widgets::RunProgressRow::runClicked, this, runSlot);
    connect(m_runAction, &QAction::triggered, this, runSlot);
}

QList<QAction *> GameAlignPage::editActions() const {
    return {m_runAction};
}

void GameAlignPage::setWorkingDirectory(const QString &dir) {
    m_workingDir = dir;
}

QString GameAlignPage::workingDirectory() const {
    return m_workingDir;
}

} // namespace dstools::labeler
