#include "BuildDsPage.h"

#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>

namespace dstools::labeler {

BuildDsPage::BuildDsPage(QWidget *parent) : QWidget(parent) {
    buildUi();
}

void BuildDsPage::buildUi() {
    auto *vLayout = new QVBoxLayout(this);

    // Parameter panel
    auto *form = new QFormLayout;

    auto *modelRow = new QHBoxLayout;
    m_rmvpePath = new QLineEdit;
    m_rmvpePath->setPlaceholderText(tr("Path to RMVPE model file"));
    auto *btnBrowseModel = new QPushButton(tr("Browse..."));
    connect(btnBrowseModel, &QPushButton::clicked, this, [this]() {
        auto path = QFileDialog::getOpenFileName(this, tr("Select RMVPE Model"),
                                                  m_workingDir, tr("ONNX Model (*.onnx)"));
        if (!path.isEmpty())
            m_rmvpePath->setText(path);
    });
    modelRow->addWidget(m_rmvpePath);
    modelRow->addWidget(btnBrowseModel);
    form->addRow(tr("RMVPE Model:"), modelRow);

    m_gpuSelector = new QComboBox;
    m_gpuSelector->addItem(tr("CPU"));
    m_gpuSelector->addItem(tr("GPU 0"));
    form->addRow(tr("Device:"), m_gpuSelector);

    m_hopSize = new QSpinBox;
    m_hopSize->setRange(64, 1024);
    m_hopSize->setValue(512);
    m_hopSize->setSingleStep(64);
    form->addRow(tr("Hop Size:"), m_hopSize);

    m_sampleRate = new QSpinBox;
    m_sampleRate->setRange(8000, 48000);
    m_sampleRate->setValue(44100);
    m_sampleRate->setSingleStep(100);
    form->addRow(tr("Sample Rate:"), m_sampleRate);

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
    m_runAction = new QAction(tr("Run Build DS"), this);
    m_runAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    auto runSlot = [this]() {
        QMessageBox::information(this, tr("Not Implemented"),
                                 tr("Build DS processing is not yet implemented.\n\n"
                                    "This step will convert transcriptions.csv to .ds files "
                                    "and extract F0 curves using RMVPE."));
    };
    connect(m_btnRun, &QPushButton::clicked, this, runSlot);
    connect(m_runAction, &QAction::triggered, this, runSlot);
}

QList<QAction *> BuildDsPage::editActions() const {
    return {m_runAction};
}

void BuildDsPage::setWorkingDirectory(const QString &dir) {
    m_workingDir = dir;
}

QString BuildDsPage::workingDirectory() const {
    return m_workingDir;
}

} // namespace dstools::labeler
