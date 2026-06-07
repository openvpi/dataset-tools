#include "BuildDsPage.h"

#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QMessageBox>
#include <QVBoxLayout>

#include <dsfw/PipelineContext.h>
#include <dsfw/PipelineRunner.h>
#include <dsfw/PipelineValidators.h>
#include <dstools/Constants.h>
#include <dstools/CsvAdapter.h>
#include <dstools/ProjectPaths.h>
#include <dstools/TranscriptionCsv.h>

namespace dstools::labeler {

BuildDsPage::BuildDsPage(QWidget *parent) : WizardPageBase(parent) {
    buildUi();
}

void BuildDsPage::buildUi() {
    auto *vLayout = new QVBoxLayout(this);

    auto *form = new QFormLayout;

    m_rmvpePath = new dsfw::widgets::PathSelector(dsfw::widgets::PathSelector::OpenFile, "",
                                                     tr("ONNX Model (*.onnx)"));
    m_rmvpePath->setPlaceholder(tr("Path to RMVPE model file"));
    form->addRow(tr("RMVPE Model:"), m_rmvpePath);

    m_gpuSelector = new QComboBox;
    m_gpuSelector->addItem(tr("CPU"), -1);
    m_gpuSelector->addItem(tr("GPU 0"), 0);
    form->addRow(tr("Device:"), m_gpuSelector);

    m_hopSize = new QSpinBox;
    m_hopSize->setRange(64, 1024);
    m_hopSize->setValue(constants::kDefaultHopSize);
    m_hopSize->setSingleStep(64);
    form->addRow(tr("Hop Size:"), m_hopSize);

    m_sampleRate = new QSpinBox;
    m_sampleRate->setRange(8000, 48000);
    m_sampleRate->setValue(constants::kDefaultSampleRate);
    m_sampleRate->setSingleStep(100);
    form->addRow(tr("Sample Rate:"), m_sampleRate);

    vLayout->addLayout(form);

    buildCommonUi(vLayout, tr("Run"), tr("Run Build DS"));
    auto runSlot = [this]() {
        if (m_workingDir.isEmpty()) {
            QMessageBox::warning(this, tr("Error"), tr("No working directory set."));
            return;
        }

        const QString csvPath = ProjectPaths::transcriptionsCsvPath(m_workingDir);
        const QString outputDir = ProjectPaths::buildDsDir(m_workingDir);
        QDir().mkpath(outputDir);

        m_log->clear();
        m_log->append(tr("Building DS files with pitch extraction..."));
        m_runProgress->setProgress(0);

        std::vector<TranscriptionRow> rows;
        auto readResult = CsvAdapter::readRows(csvPath, rows);
        if (!readResult.ok()) {
            const QString csvError = QString::fromStdString(readResult.error());
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
            const QString wavPath = ProjectPaths::slicerOutputAudioPath(m_workingDir, row.name);
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
        opts.steps.push_back(csvStep);

        StepConfig pitchStep;
        pitchStep.taskName = QStringLiteral("pitch_extraction");
        pitchStep.processorId = QStringLiteral("rmvpe");
        if (!m_rmvpePath->path().isEmpty())
            pitchStep.config["path"] = m_rmvpePath->path();
        pitchStep.config["deviceId"] = static_cast<int64_t>(gpuIndex);
        pitchStep.config["hopSize"] = static_cast<int64_t>(m_hopSize->value());
        pitchStep.config["sampleRate"] = static_cast<int64_t>(m_sampleRate->value());
        pitchStep.validator = makePitchCoverageValidator(QStringLiteral("pitch"), 0.5);
        opts.steps.push_back(pitchStep);

        StepConfig dsStep;
        dsStep.taskName = QStringLiteral("export_ds");
        dsStep.processorId = QStringLiteral("passthrough");
        dsStep.exportFormat = QStringLiteral("ds");
        opts.steps.push_back(dsStep);

        opts.workingDir = m_workingDir;
        opts.snapshotDir = ProjectPaths::buildDsDir(m_workingDir) + QStringLiteral("/snapshots");

        if (PipelineRunner::hasLatestSnapshot(opts.snapshotDir)) {
            auto answer = QMessageBox::question(
                this, tr("Recover Interrupted Build"),
                tr("An interrupted build was detected. Do you want to recover the previous progress?"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
            if (answer == QMessageBox::Yes) {
                auto recoveryResult = PipelineRunner::loadLatestSnapshot(opts.snapshotDir);
                if (recoveryResult.ok()) {
                    auto &[recoveredCtxs, recoveredStep] = recoveryResult.value();
                    m_log->append(tr("Recovered %1 item(s) from step %2.")
                                      .arg(recoveredCtxs.size())
                                      .arg(recoveredStep));
                    contexts = std::move(recoveredCtxs);
                } else {
                    m_log->append(tr("Warning: Failed to load snapshot: %1")
                                      .arg(QString::fromStdString(recoveryResult.error())));
                }
            } else {
                PipelineRunner::cleanupOldSnapshots(opts.snapshotDir);
            }
        }

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
    connect(m_runProgress, &dsfw::widgets::RunProgressRow::runClicked, this, runSlot);
    connect(m_runAction, &QAction::triggered, this, runSlot);
}

} // namespace dstools::labeler
