#include "BuildCsvPage.h"

#include <QDir>
#include <QFormLayout>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QtConcurrent>

#include <dstools/TranscriptionCsv.h>
#include <textgrid.hpp>

#include <fstream>

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

        // Scan for .TextGrid files
        QDir dir(tgDir);
        const QStringList tgFiles = dir.entryList({QStringLiteral("*.TextGrid")}, QDir::Files);

        if (tgFiles.isEmpty()) {
            m_log->append(tr("<b>Error:</b> No .TextGrid files found in %1").arg(tgDir));
            QMessageBox::warning(this, tr("Build CSV Failed"),
                                 tr("No .TextGrid files found in %1").arg(tgDir));
            return;
        }

        m_log->append(tr("Found %1 TextGrid file(s).").arg(tgFiles.size()));

        std::vector<dstools::TranscriptionRow> rows;
        int processed = 0;

        for (const QString &fileName : tgFiles) {
            const QString filePath = tgDir + QStringLiteral("/") + fileName;
            const QString stem = QFileInfo(fileName).completeBaseName();

            std::ifstream ifs(filePath.toStdString());
            if (!ifs.is_open()) {
                m_log->append(tr("  [SKIP] Cannot open: %1").arg(fileName));
                continue;
            }

            try {
                textgrid::Parser parser(ifs);
                textgrid::TextGrid tg = parser.Parse();

                // Find the "phones" interval tier
                auto phonesTier = tg.GetTierAs<textgrid::IntervalTier>("phones");
                if (!phonesTier) {
                    // Fallback: try first interval tier
                    for (size_t i = 0; i < tg.GetNumberOfTiers(); ++i) {
                        phonesTier = tg.GetTierAs<textgrid::IntervalTier>(i);
                        if (phonesTier)
                            break;
                    }
                }

                if (!phonesTier) {
                    m_log->append(tr("  [SKIP] No interval tier in: %1").arg(fileName));
                    continue;
                }

                // Extract ph_seq and ph_dur
                QStringList phSeqList;
                QStringList phDurList;
                const auto &intervals = phonesTier->GetAllIntervals();

                for (const auto &interval : intervals) {
                    QString text = QString::fromStdString(interval.text).trimmed();
                    if (text.isEmpty())
                        text = QStringLiteral("SP");
                    double dur = interval.max_time - interval.min_time;
                    phSeqList.append(text);
                    phDurList.append(QString::number(dur, 'f', 6));
                }

                dstools::TranscriptionRow row;
                row.name = stem;
                row.phSeq = phSeqList.join(QStringLiteral(" "));
                row.phDur = phDurList.join(QStringLiteral(" "));
                rows.push_back(std::move(row));

                ++processed;
                m_log->append(tr("  [OK] %1 (%2 phones)").arg(stem).arg(phSeqList.size()));

            } catch (const textgrid::Exception &e) {
                m_log->append(
                    tr("  [ERR] %1: %2").arg(fileName, QString::fromStdString(e.what())));
            }
        }

        if (rows.empty()) {
            m_log->append(tr("<b>Error:</b> No valid transcriptions extracted."));
            QMessageBox::warning(this, tr("Build CSV Failed"),
                                 tr("No valid transcriptions could be extracted."));
            return;
        }

        // Write CSV
        QString error;
        if (!dstools::TranscriptionCsv::write(csvPath, rows, error)) {
            m_log->append(tr("<b>Error:</b> %1").arg(error));
            QMessageBox::warning(this, tr("Build CSV Failed"), error);
        } else {
            m_log->append(tr("<b>Done.</b> %1 file(s) written to %2").arg(processed).arg(csvPath));
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
