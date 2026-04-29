#include "BuildDsPage.h"

#include <QFormLayout>
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

    m_rmvpePath = new dstools::widgets::PathSelector(dstools::widgets::PathSelector::OpenFile, "",
                                                     tr("ONNX Model (*.onnx)"));
    m_rmvpePath->setPlaceholder(tr("Path to RMVPE model file"));
    form->addRow(tr("RMVPE Model:"), m_rmvpePath);

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

    // Run progress row
    m_runProgress = new dstools::widgets::RunProgressRow(tr("Run"));
    vLayout->addWidget(m_runProgress);

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
    connect(m_runProgress, &dstools::widgets::RunProgressRow::runClicked, this, runSlot);
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
