#include "GameAlignPage.h"

#include <QComboBox>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QTextEdit>
#include <QVBoxLayout>

#include <dsfw/PipelineContext.h>
#include <dsfw/PipelineRunner.h>
#include <dstools/PathSelector.h>
#include <dstools/RunProgressRow.h>

namespace dstools::labeler {

GameAlignPage::GameAlignPage(QWidget *parent) : QWidget(parent) {
    buildUi();
}

void GameAlignPage::buildUi() {
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

    m_runProgress = new dstools::widgets::RunProgressRow(tr("Run"));
    vLayout->addWidget(m_runProgress);

    m_log = new QTextEdit;
    m_log->setReadOnly(true);
    vLayout->addWidget(m_log, 1);

    connect(m_runProgress, &dstools::widgets::RunProgressRow::runClicked, this, [this]() {
        if (m_workingDir.isEmpty()) {
            QMessageBox::warning(this, tr("Error"), tr("No working directory set."));
            return;
        }
        if (m_modelPath->text().isEmpty()) {
            QMessageBox::warning(this, tr("Error"), tr("No GAME model directory selected."));
            return;
        }

        const QString audioDir = m_workingDir + QStringLiteral("/dstemp/slicer_output");
        const QString csvPath = m_workingDir + QStringLiteral("/dstemp/build_csv/transcriptions.csv");

        m_log->clear();
        m_log->append(tr("Running GAME alignment..."));
        m_log->append(tr("Audio dir: %1").arg(audioDir));
        m_log->append(tr("CSV: %2").arg(csvPath));

        QDir dir(audioDir);
        const QStringList wavFiles = dir.entryList({QStringLiteral("*.wav")}, QDir::Files);
        if (wavFiles.isEmpty()) {
            m_log->append(tr("<b>Error:</b> No .wav files found in %1").arg(audioDir));
            QMessageBox::warning(this, tr("GAME Alignment Failed"),
                                 tr("No .wav files found in %1").arg(audioDir));
            return;
        }

        m_log->append(tr("Found %1 audio file(s).").arg(wavFiles.size()));

        std::vector<PipelineContext> contexts;
        for (const QString &fileName : wavFiles) {
            PipelineContext ctx;
            ctx.itemId = QFileInfo(fileName).completeBaseName();
            ctx.audioPath = audioDir + QStringLiteral("/") + fileName;
            contexts.push_back(std::move(ctx));
        }

        const int gpuIndex = m_gpuSelector->currentData().toInt();

        PipelineOptions opts;
        StepConfig step;
        step.taskName = QStringLiteral("midi_transcription");
        step.processorId = QStringLiteral("game");
        step.config = ProcessorConfig::object();
        step.config["path"] = m_modelPath->text().toStdString();
        step.config["deviceId"] = gpuIndex;
        step.config["csvPath"] = csvPath.toStdString();
        opts.steps.push_back(step);
        opts.workingDir = m_workingDir;

        PipelineRunner runner;
        QObject::connect(&runner, &PipelineRunner::progress,
                         this, [this](int, int item, int total, const QString &) {
                             if (total > 0)
                                 m_runProgress->setProgress(item * 100 / total);
                         });

        auto result = runner.run(opts, contexts);
        if (!result.ok()) {
            m_log->append(tr("<b>Error:</b> %1").arg(QString::fromStdString(result.error())));
            QMessageBox::warning(this, tr("GAME Alignment Failed"),
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
        m_log->append(tr("<b>Done.</b> %1 file(s) processed.").arg(processed));
        if (skipped > 0)
            m_log->append(tr("  %1 file(s) skipped.").arg(skipped));
    });
}

void GameAlignPage::setWorkingDirectory(const QString &dir) {
    m_workingDir = dir;
}

QString GameAlignPage::workingDirectory() const {
    return m_workingDir;
}

} // namespace dstools::labeler
