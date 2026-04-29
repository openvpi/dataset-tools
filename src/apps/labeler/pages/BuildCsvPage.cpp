#include "BuildCsvPage.h"

#include <QFormLayout>
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
        QMessageBox::information(this, tr("Not Implemented"),
                                 tr("Build CSV processing is not yet implemented.\n\n"
                                    "This step will convert TextGrid files to transcriptions.csv "
                                    "and optionally calculate ph_num values."));
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
