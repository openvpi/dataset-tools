#include "CleanupDialog.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QDirIterator>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace dstools::labeler {

static const char *stepNames[] = {
    "Slice", "ASR", "Label", "Align", "Phone", "CSV", "MIDI", "DS", "Pitch",
};

static const char *stepDirs[] = {
    "slicer",           "asr",      "asr_review", "alignment", "alignment_review",
    "build_csv",        "midi",     "build_ds",   "pitch_review",
};

CleanupDialog::CleanupDialog(const QString &workingDir, QWidget *parent)
    : QDialog(parent), m_workingDir(workingDir) {
    setWindowTitle(tr("Clean Working Directory"));
    setMinimumWidth(420);
    buildLayout();
    scanDirectory();
}

void CleanupDialog::buildLayout() {
    auto *layout = new QVBoxLayout(this);

    auto *desc = new QLabel(
        tr("Select intermediate files to delete. Original audio files are never deleted."));
    desc->setWordWrap(true);
    layout->addWidget(desc);

    auto *group = new QGroupBox(tr("Steps"));
    auto *groupLayout = new QVBoxLayout(group);

    m_checkBoxes.resize(9);
    for (int i = 0; i < 9; ++i) {
        m_checkBoxes[i] = new QCheckBox(tr("Step %1 (%2)").arg(i + 1).arg(stepNames[i]));
        groupLayout->addWidget(m_checkBoxes[i]);
    }

    layout->addWidget(group);

    // Select All / Deselect All
    auto *btnLayout = new QHBoxLayout;
    auto *selAll = new QPushButton(tr("Select All"));
    auto *deselAll = new QPushButton(tr("Deselect All"));
    btnLayout->addWidget(selAll);
    btnLayout->addWidget(deselAll);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    connect(selAll, &QPushButton::clicked, this, &CleanupDialog::selectAll);
    connect(deselAll, &QPushButton::clicked, this, &CleanupDialog::deselectAll);

    // OK / Cancel
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void CleanupDialog::scanDirectory() {
    QDir base(m_workingDir + QStringLiteral("/dstemp"));
    for (int i = 0; i < 9; ++i) {
        QDir stepDir(base.filePath(stepDirs[i]));
        if (!stepDir.exists()) {
            m_checkBoxes[i]->setText(
                tr("Step %1 (%2): no files").arg(i + 1).arg(stepNames[i]));
            m_checkBoxes[i]->setEnabled(false);
            continue;
        }

        int fileCount = 0;
        qint64 totalSize = 0;
        QDirIterator it(stepDir.absolutePath(), QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            it.next();
            ++fileCount;
            totalSize += it.fileInfo().size();
        }

        QString sizeStr;
        if (totalSize >= 1024 * 1024)
            sizeStr = QStringLiteral("~%1 MB").arg(totalSize / (1024 * 1024));
        else if (totalSize >= 1024)
            sizeStr = QStringLiteral("~%1 KB").arg(totalSize / 1024);
        else
            sizeStr = QStringLiteral("%1 bytes").arg(totalSize);

        m_checkBoxes[i]->setText(
            tr("Step %1 (%2): %3 files, %4")
                .arg(i + 1)
                .arg(stepNames[i])
                .arg(fileCount)
                .arg(sizeStr));

        if (fileCount == 0)
            m_checkBoxes[i]->setEnabled(false);
    }
}

QVector<int> CleanupDialog::selectedSteps() const {
    QVector<int> result;
    for (int i = 0; i < m_checkBoxes.size(); ++i) {
        if (m_checkBoxes[i]->isChecked())
            result.append(i);
    }
    return result;
}

void CleanupDialog::selectAll() {
    for (auto *cb : m_checkBoxes)
        if (cb->isEnabled())
            cb->setChecked(true);
}

void CleanupDialog::deselectAll() {
    for (auto *cb : m_checkBoxes)
        cb->setChecked(false);
}

} // namespace dstools::labeler
