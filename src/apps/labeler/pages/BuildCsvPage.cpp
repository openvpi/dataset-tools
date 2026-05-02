#include "BuildCsvPage.h"

#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QMessageBox>
#include <QVBoxLayout>

#include <dsfw/PipelineContext.h>
#include <dsfw/PipelineRunner.h>

#include <dstools/DomainInit.h>

namespace dstools::labeler {

BuildCsvPage::BuildCsvPage(QWidget *parent) : QWidget(parent) {
    buildUi();
}

void BuildCsvPage::buildUi() {
    auto *vLayout = new QVBoxLayout(this);

    auto *form = new QFormLayout;

    m_dictPath = new dstools::widgets::PathSelector(dstools::widgets::PathSelector::OpenFile, "",
                                                    tr("All Files (*)"));
    m_dictPath->setPlaceholder(tr("Path to dictionary file"));
    form->addRow(tr("Dictionary:"), m_dictPath);

    m_chkPhNum = new QCheckBox(tr("Calculate ph_num"));
    m_chkPhNum->setChecked(true);
    form->addRow(QString(), m_chkPhNum);

    vLayout->addLayout(form);

    m_runProgress = new dstools::widgets::RunProgressRow(tr("Run"));
    vLayout->addWidget(m_runProgress);

    m_log = new QTextEdit;
    m_log->setReadOnly(true);
    vLayout->addWidget(m_log, 1);

    m_runAction = new QAction(tr("Run Build CSV"), this);
    m_runAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    auto runSlot = [this]() {
        if (m_workingDir.isEmpty()) {
            QMessageBox::warning(this, tr("Error"), tr("No working directory set."));
            return;
        }

        const QString tgDir = m_workingDir + QStringLiteral("/dstemp/alignment_review");
        const QString csvPath = m_workingDir + QStringLiteral("/dstemp/build_csv/transcriptions.csv");

        QDir().mkpath(m_workingDir + QStringLiteral("/dstemp/build_csv"));

        m_log->clear();
        m_log->append(tr("Building CSV from TextGrid files..."));
        m_log->append(tr("TextGrid dir: %1").arg(tgDir));
        m_log->append(tr("Output: %1").arg(csvPath));

        QDir dir(tgDir);
        const QStringList tgFiles = dir.entryList({QStringLiteral("*.TextGrid")}, QDir::Files);

        if (tgFiles.isEmpty()) {
            m_log->append(tr("<b>Error:</b> No .TextGrid files found in %1").arg(tgDir));
            QMessageBox::warning(this, tr("Build CSV Failed"),
                                 tr("No .TextGrid files found in %1").arg(tgDir));
            return;
        }

        m_log->append(tr("Found %1 TextGrid file(s).").arg(tgFiles.size()));

        std::vector<dstools::PipelineContext> contexts;
        for (const QString &fileName : tgFiles) {
            const QString filePath = tgDir + QStringLiteral("/") + fileName;
            const QString stem = QFileInfo(fileName).completeBaseName();

            dstools::PipelineContext ctx;
            ctx.itemId = stem;
            ctx.audioPath = filePath;
            contexts.push_back(std::move(ctx));
        }

        dstools::PipelineOptions opts;
        dstools::StepConfig step;
        step.taskName = QStringLiteral("build_csv");
        step.processorId = QStringLiteral("passthrough");
        step.importFormat = QStringLiteral("textgrid");
        step.config = dstools::ProcessorConfig::object();
        if (m_chkPhNum->isChecked())
            step.config["computePhNum"] = true;
        opts.steps.push_back(step);
        opts.workingDir = m_workingDir;

        dstools::PipelineRunner runner;
        QObject::connect(&runner, &dstools::PipelineRunner::progress,
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
            if (ctx.status == dstools::PipelineContext::Status::Active) {
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
    connect(m_runProgress, &dstools::widgets::RunProgressRow::runClicked, this, runSlot);
    connect(m_runAction, &QAction::triggered, this, runSlot);
}

void BuildCsvPage::setWorkingDirectory(const QString &dir) {
    m_workingDir = dir;
}

QString BuildCsvPage::workingDirectory() const {
    return m_workingDir;
}

} // namespace dstools::labeler
