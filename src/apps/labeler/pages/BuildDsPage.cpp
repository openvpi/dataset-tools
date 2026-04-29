#include "BuildDsPage.h"

#include <QDir>
#include <QFormLayout>
#include <QMessageBox>
#include <QVBoxLayout>

#include <dstools/CsvToDsConverter.h>
#include <rmvpe-infer/RmvpeModel.h>

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
        if (m_workingDir.isEmpty()) {
            QMessageBox::warning(this, tr("Error"), tr("No working directory set."));
            return;
        }
        if (m_rmvpePath->path().isEmpty()) {
            QMessageBox::warning(this, tr("Error"), tr("No RMVPE model file selected."));
            return;
        }

        const QString csvPath = m_workingDir + QStringLiteral("/dstemp/midi/transcriptions.csv");
        const QString outputDir = m_workingDir + QStringLiteral("/dstemp/build_ds");
        QDir().mkpath(outputDir);

        m_log->clear();
        m_log->append(tr("Loading RMVPE model..."));

        // Determine execution provider
        Rmvpe::ExecutionProvider ep = Rmvpe::ExecutionProvider::CPU;
        if (m_gpuSelector->currentIndex() == 1)
            ep = Rmvpe::ExecutionProvider::DML;

        auto rmvpe = std::make_shared<Rmvpe::RmvpeModel>(
            m_rmvpePath->path().toStdString(), ep, 0);

        if (!rmvpe->is_open()) {
            m_log->append(tr("<b>Error:</b> Failed to load RMVPE model."));
            return;
        }

        m_log->append(tr("Converting CSV to DS files with F0 extraction..."));
        m_runProgress->setProgress(0);

        dstools::CsvToDsConverter::Options opts;
        opts.csvPath = csvPath;
        opts.wavsDir = m_workingDir;
        opts.outputDir = outputDir;
        opts.sampleRate = m_sampleRate->value();
        opts.hopSize = m_hopSize->value();

        auto f0Callback = [&rmvpe](const QString &wavPath, std::vector<float> &f0, QString &error) -> bool {
            // TODO: Load WAV, resample to 16kHz, run RMVPE forward
            // This is a placeholder — full implementation requires audio loading
            Q_UNUSED(wavPath)
            Q_UNUSED(f0)
            error = "F0 extraction not yet fully implemented (requires audio decode integration)";
            return false;
        };

        auto progressCb = [this](int current, int total, const QString &name) {
            if (total > 0)
                m_runProgress->setProgress(current * 100 / total);
            m_log->append(tr("[%1/%2] %3").arg(current).arg(total).arg(name));
        };

        QString error;
        if (!dstools::CsvToDsConverter::convert(opts, f0Callback, progressCb, error)) {
            m_log->append(tr("<b>Error:</b> %1").arg(error));
            QMessageBox::warning(this, tr("Build DS Failed"), error);
        } else {
            m_runProgress->setProgress(100);
            m_log->append(tr("<b>Done.</b> DS files in %1").arg(outputDir));
        }
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
