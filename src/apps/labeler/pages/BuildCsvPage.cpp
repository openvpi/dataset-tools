#include "BuildCsvPage.h"

#include <QDir>
#include <QFormLayout>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QtConcurrent>

#include <dstools/TranscriptionPipeline.h>

namespace dstools::labeler {

BuildCsvPage::BuildCsvPage(QWidget *parent) : QWidget(parent) {
    buildUi();
}

void BuildCsvPage::buildUi() {
    auto *vLayout = new QVBoxLayout(this);

    // Parameter panel
    auto *form = new QFormLayout;

    m_dictPath = new dstools::widgets::PathSelector(dstools::widgets::PathSelector::OpenFile, "",
                                                    tr("All Files (*)"));
    m_dictPath->setPlaceholder(tr("Path to dictionary file"));
    form->addRow(tr("Dictionary:"), m_dictPath);

    m_chkPhNum = new QCheckBox(tr("Calculate ph_num"));
    m_chkPhNum->setChecked(true);
    form->addRow(QString(), m_chkPhNum);

    vLayout->addLayout(form);

    // Run progress row
    m_runProgress = new dstools::widgets::RunProgressRow(tr("Run"));
    vLayout->addWidget(m_runProgress);

    // Log area
    m_log = new QTextEdit;
    m_log->setReadOnly(true);
    vLayout->addWidget(m_log, 1);

    // Run action
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

        dstools::TranscriptionPipeline::Options opts;
        opts.textGridDir = tgDir;
        opts.wavsDir = m_workingDir;
        opts.dictPath = m_dictPath->path();
        opts.writeCsv = true;
        opts.csvPath = csvPath;

        QString error;
        bool ok = dstools::TranscriptionPipeline::extractTextGridToCsv(opts, error);
        if (!ok) {
            m_log->append(tr("<b>Error:</b> %1").arg(error));
            QMessageBox::warning(this, tr("Build CSV Failed"), error);
        } else {
            m_log->append(tr("<b>Done.</b> CSV written to %1").arg(csvPath));
        }
    };
    connect(m_runProgress, &dstools::widgets::RunProgressRow::runClicked, this, runSlot);
    connect(m_runAction, &QAction::triggered, this, runSlot);
}

QList<QAction *> BuildCsvPage::editActions() const {
    return {m_runAction};
}

void BuildCsvPage::setWorkingDirectory(const QString &dir) {
    m_workingDir = dir;
}

QString BuildCsvPage::workingDirectory() const {
    return m_workingDir;
}

} // namespace dstools::labeler
