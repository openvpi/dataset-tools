#include "BuildCsvPage.h"

#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QMessageBox>
#include <QVBoxLayout>

#include <dsfw/PipelineContext.h>
#include <dsfw/PipelineRunner.h>
#include <dsfw/PipelineValidators.h>

#include <dstools/DomainInit.h>
#include <dstools/ProjectPaths.h>

namespace dstools::labeler {


BuildCsvPage::BuildCsvPage(QWidget *parent) : dsfw::WizardPageBase(parent) {
    buildUi();
}

void BuildCsvPage::buildUi() {
    auto *vLayout = new QVBoxLayout(this);

    auto *form = new QFormLayout;

    m_dictPath = new dsfw::widgets::PathSelector(dsfw::widgets::PathSelector::OpenFile, "",
                                                    tr("All Files (*)"));
    m_dictPath->setPlaceholder(tr("Path to dictionary file"));
    form->addRow(tr("Dictionary:"), m_dictPath);

    m_chkPhNum = new QCheckBox(tr("Calculate ph_num"));
    m_chkPhNum->setChecked(true);
    form->addRow(QString(), m_chkPhNum);

    vLayout->addLayout(form);

    buildCommonUi(vLayout, tr("Run"), tr("Run Build CSV"));
    auto runSlot = [this]() {
        if (m_workingDir.isEmpty()) {
            QMessageBox::warning(this, tr("Error"), tr("No working directory set."));
            return;
        }

        const QString tgDir = ProjectPaths::alignmentReviewDir(m_workingDir);
        const QString csvPath = ProjectPaths::transcriptionsCsvPath(m_workingDir);

        QDir().mkpath(ProjectPaths::buildCsvDir(m_workingDir));

        m_log->clear();
        m_log->append(tr("Building CSV from dsfw::TextGrid files..."));
        m_log->append(tr("dsfw::TextGrid dir: %1").arg(tgDir));
        m_log->append(tr("Output: %1").arg(csvPath));

        QDir dir(tgDir);
        const QStringList tgFiles = dir.entryList({QStringLiteral("*.dsfw::TextGrid")}, QDir::Files);

        if (tgFiles.isEmpty()) {
            m_log->append(tr("<b>Error:</b> No .dsfw::TextGrid files found in %1").arg(tgDir));
            QMessageBox::warning(this, tr("Build CSV Failed"),
                                 tr("No .dsfw::TextGrid files found in %1").arg(tgDir));
            return;
        }

        m_log->append(tr("Found %1 dsfw::TextGrid file(s).").arg(tgFiles.size()));

        std::vector<dsfw::PipelineContext> contexts;
        for (const QString &fileName : tgFiles) {
            const QString filePath = QDir(tgDir).filePath(fileName);
            const QString stem = QFileInfo(fileName).completeBaseName();

            dsfw::PipelineContext ctx;
            ctx.itemId = stem;
            ctx.audioPath = filePath;
            contexts.push_back(std::move(ctx));
        }

        dsfw::PipelineOptions opts;
        dsfw::StepConfig step;
        step.taskName = QStringLiteral("build_csv");
        step.processorId = QStringLiteral("passthrough");
        step.importFormat = QStringLiteral("textgrid");
        if (m_chkPhNum->isChecked())
            step.config["computePhNum"] = true;
        step.validator = makeSliceLengthValidator(0.1, 30.0);
        opts.steps.push_back(step);
        opts.workingDir = m_workingDir;
        opts.snapshotDir = ProjectPaths::buildCsvDir(m_workingDir) + QStringLiteral("/snapshots");

        if (dsfw::PipelineRunner::hasLatestSnapshot(opts.snapshotDir)) {
            auto answer = QMessageBox::question(
                this, tr("Recover Interrupted Build"),
                tr("An interrupted build was detected. Do you want to recover the previous progress?"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
            if (answer == QMessageBox::Yes) {
                auto recoveryResult = dsfw::PipelineRunner::loadLatestSnapshot(opts.snapshotDir);
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
                dsfw::PipelineRunner::cleanupOldSnapshots(opts.snapshotDir);
            }
        }

        dsfw::PipelineRunner runner;
        QObject::connect(&runner, &dsfw::PipelineRunner::progress,
                         this, [this](int, int item, int total, const QString &) {
                             if (total > 0)
                                 m_runProgress->setProgress(item * 100 / total);
                         });

        auto result = runner.run(opts, contexts);
        if (!result.ok()) {
            m_log->append(tr("<b>Error:</b> %1").arg(QString::fromStdString(result.error())));
            QMessageBox::warning(this, tr("Build CSV Failed"),
                                 QString::fromStdString(result.error()));
            return;
        }

        int processed = 0;
        int skipped = 0;
        for (const auto &ctx : contexts) {
            if (ctx.status == dsfw::PipelineContext::Status::Active) {
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

        if (processed == 0) {
            m_log->append(tr("<b>Error:</b> No valid transcriptions extracted."));
            QMessageBox::warning(this, tr("Build CSV Failed"),
                                 tr("No valid transcriptions could be extracted."));
            return;
        }

        auto exportResult = dstools::exportContextsToCsv(contexts, csvPath);
        if (!exportResult.ok()) {
            m_log->append(tr("<b>Error:</b> %1").arg(QString::fromStdString(exportResult.error())));
            QMessageBox::warning(this, tr("Build CSV Failed"),
                                 QString::fromStdString(exportResult.error()));
        } else {
            m_runProgress->setProgress(100);
            m_log->append(tr("<b>Done.</b> %1 file(s) written to %2").arg(processed).arg(csvPath));
            if (skipped > 0)
                m_log->append(tr("  %1 file(s) skipped.").arg(skipped));
        }
    };
    connect(m_runProgress, &dsfw::widgets::RunProgressRow::runClicked, this, runSlot);
    connect(m_runAction, &QAction::triggered, this, runSlot);
}

} // namespace dstools::labeler
