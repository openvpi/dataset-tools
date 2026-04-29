#include "GameAlignPage.h"

#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
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

    auto *modelRow = new QHBoxLayout;
    m_modelPath = new QLineEdit;
    m_modelPath->setPlaceholderText(tr("Path to GAME model directory"));
    auto *btnBrowseModel = new QPushButton(tr("Browse..."));
    connect(btnBrowseModel, &QPushButton::clicked, this, [this]() {
        auto path = QFileDialog::getExistingDirectory(this, tr("Select GAME Model Directory"),
                                                       m_workingDir);
        if (!path.isEmpty())
            m_modelPath->setText(path);
    });
    modelRow->addWidget(m_modelPath);
    modelRow->addWidget(btnBrowseModel);
    form->addRow(tr("Model:"), modelRow);

    m_gpuSelector = new QComboBox;
    m_gpuSelector->addItem(tr("CPU"));
    m_gpuSelector->addItem(tr("GPU 0"));
    form->addRow(tr("Device:"), m_gpuSelector);

    vLayout->addLayout(form);

    // Run button + progress
    auto *runRow = new QHBoxLayout;
    m_btnRun = new QPushButton(tr("Run"));
    m_progress = new QProgressBar;
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    runRow->addWidget(m_btnRun);
    runRow->addWidget(m_progress, 1);
    vLayout->addLayout(runRow);

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
    connect(m_btnRun, &QPushButton::clicked, this, runSlot);
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
