#include "WizardPageBase.h"

#include <QKeySequence>

namespace dstools::labeler {

using namespace dsfw;

WizardPageBase::WizardPageBase(QWidget *parent) : QWidget(parent) {}

void WizardPageBase::setWorkingDirectory(const QString &dir) {
    m_workingDir = dir;
}

QString WizardPageBase::workingDirectory() const {
    return m_workingDir;
}

void WizardPageBase::buildCommonUi(QVBoxLayout *vLayout, const QString &runLabel, const QString &runActionText) {
    m_runProgress = new dsfw::widgets::RunProgressRow(runLabel);
    vLayout->addWidget(m_runProgress);

    m_log = new QTextEdit;
    m_log->setReadOnly(true);
    vLayout->addWidget(m_log, 1);

    if (!runActionText.isEmpty()) {
        m_runAction = new QAction(runActionText, this);
        m_runAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    }
}

} // namespace dstools::labeler