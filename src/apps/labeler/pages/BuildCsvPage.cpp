#include "BuildCsvPage.h"

#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>

namespace dstools::labeler {

BuildCsvPage::BuildCsvPage(QWidget *parent) : QWidget(parent) {
    buildUi();
}

void BuildCsvPage::buildUi() {
    auto *vLayout = new QVBoxLayout(this);

    // Parameter panel
    auto *form = new QFormLayout;

    auto *dictRow = new QHBoxLayout;
    m_dictPath = new QLineEdit;
    m_dictPath->setPlaceholderText(tr("Path to dictionary file"));
    auto *btnBrowseDict = new QPushButton(tr("Browse..."));
    connect(btnBrowseDict, &QPushButton::clicked, this, [this]() {
        auto path = QFileDialog::getOpenFileName(this, tr("Select Dictionary File"),
                                                  m_workingDir, tr("All Files (*)"));
        if (!path.isEmpty())
            m_dictPath->setText(path);
    });
    dictRow->addWidget(m_dictPath);
    dictRow->addWidget(btnBrowseDict);
    form->addRow(tr("Dictionary:"), dictRow);

    m_chkPhNum = new QCheckBox(tr("Calculate ph_num"));
    m_chkPhNum->setChecked(true);
    form->addRow(QString(), m_chkPhNum);

    vLayout->addLayout(form);

    // Run button + progress
    auto *runRow = new QHBoxLayout;
    m_btnRun = new QPushButton(tr("Run"));
    m_progress = new QProgressBar;
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    runRow->addWidget(m_btnRun);
    runRow->addWidget(m_progress, 1);
    vLayout->addLayout(runRow);

    // Log area
    m_log = new QTextEdit;
    m_log->setReadOnly(true);
    vLayout->addWidget(m_log, 1);

    // Run action
    m_runAction = new QAction(tr("Run Build CSV"), this);
    m_runAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    auto runSlot = [this]() {
        QMessageBox::information(this, tr("Not Implemented"),
                                 tr("Build CSV processing is not yet implemented.\n\n"
                                    "This step will convert TextGrid files to transcriptions.csv "
                                    "and optionally calculate ph_num values."));
    };
    connect(m_btnRun, &QPushButton::clicked, this, runSlot);
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
