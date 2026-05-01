#include "GameAlignPage.h"

#include <QDir>
#include <QFormLayout>
#include <QMessageBox>
#include <QVBoxLayout>

#include <dsfw/ServiceLocator.h>

namespace dstools::labeler {

GameAlignPage::GameAlignPage(QWidget *parent) : QWidget(parent) {
    auto *vLayout = new QVBoxLayout(this);

    auto *form = new QFormLayout;

    m_modelPath = new QLineEdit;
    m_modelPath->setPlaceholder(tr("Path to GAME model directory"));
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

    auto *service = dstools::ServiceLocator::get<dstools::IAlignmentService>();
    if (!service) {
        QMessageBox::warning(this, tr("Error"), tr("No alignment service registered."));
        return;
    }

    const int gpuIndex = m_gpuSelector->currentData().toInt();
    auto loadResult = service->loadModel(m_modelPath->text(), gpuIndex);
    if (!loadResult) {
        QMessageBox::warning(this, tr("Error"),
                             tr("Failed to load model: %1").arg(QString::fromStdString(loadResult.error())));
        return;
    }

    dstools::AlignCsvOptions opts;
    auto alignResult = service->alignCSV(
        m_modelPath->text(), "",
        opts,
        [](int progress) { (void)progress; });

    if (!alignResult) {
        QMessageBox::warning(this, tr("MIDI Alignment Failed"),
                             QString::fromStdString(alignResult.error()));
    }
}

} // namespace dstools::labeler
