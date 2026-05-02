#include "GameAlignPage.h"

#include <QDir>
#include <QFormLayout>
#include <QMessageBox>
#include <QVBoxLayout>

#include <dsfw/TaskProcessorRegistry.h>
#include <dstools/ModelManager.h>
#include <QComboBox>
#include <QLineEdit>

GameAlignPage::GameAlignPage(QWidget *parent) : QWidget(parent) {
    auto *vLayout = new QVBoxLayout(this);

    auto *form = new QFormLayout;

    m_modelPath = new QLineEdit;
    m_modelPath->setPlaceholderText(tr("Path to GAME model directory"));
    form->addRow(tr("Model:"), m_modelPath);

    m_gpuSelector = new QComboBox;
    m_gpuSelector->addItem(tr("CPU"), -1);
    m_gpuSelector->addItem(tr("GPU 0"), 0);
    form->addRow(tr("Device:"), m_gpuSelector);

    vLayout->addLayout(form);

    vLayout->addStretch(1);
}

void GameAlignPage::onRunAlignment() {
    if (m_modelPath->text().isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("No GAME model directory selected."));
        return;
    }

    auto processor = dstools::TaskProcessorRegistry::instance().create(
        QStringLiteral("midi_transcription"), QStringLiteral("game"));
    if (!processor) {
        QMessageBox::warning(this, tr("Error"), tr("No GAME processor registered."));
        return;
    }

    const int gpuIndex = m_gpuSelector->currentData().toInt();
    dstools::ModelManager mm;
    dstools::ProcessorConfig config;
    config["path"] = m_modelPath->text().toStdString();
    config["deviceId"] = gpuIndex;
    auto initResult = processor->initialize(mm, config);
    if (!initResult) {
        QMessageBox::warning(this, tr("Error"),
                             tr("Failed to load model: %1").arg(QString::fromStdString(initResult.error())));
        return;
    }

    dstools::TaskInput taskInput;
    taskInput.audioPath = m_modelPath->text();
    taskInput.config = config;
    auto result = processor->process(taskInput);
    if (!result) {
        QMessageBox::warning(this, tr("MIDI Alignment Failed"),
                             QString::fromStdString(result.error()));
    }
}
