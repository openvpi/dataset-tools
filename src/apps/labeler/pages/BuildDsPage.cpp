#include "BuildDsPage.h"

#include <QDir>
#include <QFormLayout>
#include <QMessageBox>
#include <QVBoxLayout>

#include <dstools/CsvToDsConverter.h>

namespace dstools::labeler {

static constexpr int kDefaultHopSize = 512;
static constexpr int kDefaultSampleRate = 44100;

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
    m_hopSize->setValue(kDefaultHopSize);
    m_hopSize->setSingleStep(64);
    form->addRow(tr("Hop Size:"), m_hopSize);

    m_sampleRate = new QSpinBox;
    m_sampleRate->setRange(8000, 48000);
    m_sampleRate->setValue(kDefaultSampleRate);
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

        const QString csvPath = m_workingDir + QStringLiteral("/dstemp/midi/transcriptions.csv");
        const QString outputDir = m_workingDir + QStringLiteral("/dstemp/build_ds");
        QDir().mkpath(outputDir);

        m_log->clear();
        m_log->append(tr("Converting CSV to DS files with F0 extraction..."));
        m_runProgress->setProgress(0);

        dstools::CsvToDsConverter::Options opts;
        opts.csvPath = csvPath;
        opts.wavsDir = m_workingDir;
        opts.outputDir = outputDir;
        opts.sampleRate = m_sampleRate->value();
        opts.hopSize = m_hopSize->value();

        auto progressCb = [this](int current, int total, const QString &name) {
            if (total > 0)
                m_runProgress->setProgress(current * 100 / total);
            m_log->append(tr("[%1/%2] %3").arg(current).arg(total).arg(name));
        };

        // Pass nullptr as f0Callback to use the built-in autocorrelation fallback.
        // TODO: Implement proper RMVPE-based F0 extraction when audio decode integration is ready.
        QString error;
        if (!dstools::CsvToDsConverter::convert(opts, nullptr, progressCb, error)) {
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

int BuildDsPage::progressTotal() const {
    return m_progressTotal;
}

int BuildDsPage::progressCurrent() const {
    return m_progressCurrent;
}

bool BuildDsPage::isRunning() const {
    return m_running;
}

QString BuildDsPage::progressMessage() const {
    return m_progressMessage;
}

void BuildDsPage::cancelOperation() {
    m_running = false;
}

} // namespace dstools::labeler
