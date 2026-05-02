#include "BuildDsPage.h"

#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QMessageBox>
#include <QVBoxLayout>

#include <dsfw/PipelineContext.h>
#include <dsfw/PipelineRunner.h>
#include <dstools/TranscriptionCsv.h>
#include <dstools/RunProgressRow.h>

namespace dstools::labeler {

static constexpr int kDefaultHopSize = 512;
static constexpr int kDefaultSampleRate = 44100;

BuildDsPage::BuildDsPage(QWidget *parent) : QWidget(parent) {
    buildUi();
}

void BuildDsPage::buildUi() {
    auto *vLayout = new QVBoxLayout(this);

    auto *form = new QFormLayout;

    m_rmvpePath = new dstools::widgets::PathSelector(dstools::widgets::PathSelector::OpenFile, "",
                                                     tr("ONNX Model (*.onnx)"));
    m_rmvpePath->setPlaceholder(tr("Path to RMVPE model file"));
    form->addRow(tr("RMVPE Model:"), m_rmvpePath);

    m_gpuSelector = new QComboBox;
    m_gpuSelector->addItem(tr("CPU"), -1);
    m_gpuSelector->addItem(tr("GPU 0"), 0);
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

    m_runProgress = new dstools::widgets::RunProgressRow(tr("Run"));
    vLayout->addWidget(m_runProgress);

    m_log = new QTextEdit;
    m_log->setReadOnly(true);
    vLayout->addWidget(m_log, 1);

    m_runAction = new QAction(tr("Run Build DS"), this);
    m_runAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    auto runSlot = [this]() {
        if (m_workingDir.isEmpty()) {
            QMessageBox::warning(this, tr("Error"), tr("No working directory set."));
            return;
        }

        const QString csvPath = m_workingDir + QStringLiteral("/dstemp/build_csv/transcriptions.csv");
        const QString outputDir = m_workingDir + QStringLiteral("/dstemp/build_ds");
        QDir().mkpath(outputDir);

        m_log->clear();
        m_log->append(tr("Building DS files with pitch extraction..."));
        m_runProgress->setProgress(0);

        std::vector<TranscriptionRow> rows;
        QString csvError;
        if (!TranscriptionCsv::read(csvPath, rows, csvError)) {
            m_log->append(tr("<b>Error:</b> Cannot read CSV: %1").arg(csvError));
            QMessageBox::warning(this, tr("Build DS Failed"), csvError);
            return;
        }

        if (rows.empty()) {
            m_log->append(tr("<b>Error:</b> CSV file is empty."));
            QMessageBox::warning(this, tr("Build DS Failed"), tr("CSV file contains no rows."));
            return;
        }

        m_log->append(tr("Found %1 item(s) in CSV.").arg(rows.size()));

        std::vector<PipelineContext> contexts;
        for (const auto &row : rows) {
            const QString wavPath = m_workingDir + QStringLiteral("/dstemp/slicer_output/") + row.name + QStringLiteral(".wav");
            if (!QFile::exists(wavPath)) {
                m_log->append(tr("  [SKIP] %1: WAV not found").arg(row.name));
                continue;
            }

            PipelineContext ctx;
            ctx.itemId = row.name;
            ctx.audioPath = wavPath;
            contexts.push_back(std::move(ctx));
        }

        if (contexts.empty()) {
            m_log->append(tr("<b>Error:</b> No valid audio files found."));
            QMessageBox::warning(this, tr("Build DS Failed"), tr("No valid audio files found."));
            return;
        }

        const int gpuIndex = m_gpuSelector->currentData().toInt();

        PipelineOptions opts;
        StepConfig csvStep;
        csvStep.taskName = QStringLiteral("import_csv");
        csvStep.processorId = QStringLiteral("passthrough");
        csvStep.importFormat = QStringLiteral("csv");
        csvStep.importPath = csvPath;
        csvStep.config = ProcessorConfig::object();
        opts.steps.push_back(csvStep);

        StepConfig pitchStep;
        pitchStep.taskName = QStringLiteral("pitch_extraction");
        pitchStep.processorId = QStringLiteral("rmvpe");
        pitchStep.config = ProcessorConfig::object();
        if (!m_rmvpePath->path().isEmpty())
            pitchStep.config["path"] = m_rmvpePath->path().toStdString();
        pitchStep.config["deviceId"] = gpuIndex;
        pitchStep.config["hopSize"] = m_hopSize->value();
        pitchStep.config["sampleRate"] = m_sampleRate->value();
        opts.steps.push_back(pitchStep);

        StepConfig dsStep;
        dsStep.taskName = QStringLiteral("export_ds");
        dsStep.processorId = QStringLiteral("passthrough");
        dsStep.exportFormat = QStringLiteral("ds");
        dsStep.config = ProcessorConfig::object();
        opts.steps.push_back(dsStep);

        opts.workingDir = m_workingDir;

        PipelineRunner runner;
        QObject::connect(&runner, &PipelineRunner::progress,
                         this, [this](int, int item, int total, const QString &) {
                             if (total > 0)
                                 m_runProgress->setProgress(item * 100 / total);
                         });
        QObject::connect(&runner, &PipelineRunner::stepCompleted,
                         this, [this](int step, const QString &name) {
                             m_log->append(tr("Step %1: %2 completed.").arg(step + 1).arg(name));
                         });

        auto result = runner.run(opts, contexts);
        if (!result.ok()) {
            m_log->append(tr("<b>Error:</b> %1").arg(QString::fromStdString(result.error())));
            QMessageBox::warning(this, tr("Build DS Failed"),
                                 QString::fromStdString(result.error()));
            return;
        }

        int processed = 0;
        int skipped = 0;
        for (const auto &ctx : contexts) {
            if (ctx.status == PipelineContext::Status::Active) {
                m_log->append(tr("  [OK] %1").arg(ctx.itemId));
                ++processed;
            } else {
                m_log->append(tr("  [SKIP] %1: %2")
                                  .arg(ctx.itemId,
                                       ctx.discardReason.isEmpty()
                                           ? tr("unknown error")
                                           : ctx.discardReason));
                ++skipped;
            }
        }

        m_runProgress->setProgress(100);
        m_log->append(tr("<b>Done.</b> %1 DS file(s) in %2").arg(processed).arg(outputDir));
        if (skipped > 0)
            m_log->append(tr("  %1 file(s) skipped.").arg(skipped));
    };
    connect(m_runProgress, &dstools::widgets::RunProgressRow::runClicked, this, runSlot);
    connect(m_runAction, &QAction::triggered, this, runSlot);
}

void BuildDsPage::setWorkingDirectory(const QString &dir) {
    m_workingDir = dir;
}

QString BuildDsPage::workingDirectory() const {
    return m_workingDir;
}

} // namespace dstools::labeler
