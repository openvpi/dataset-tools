#pragma once

#include "IPageActions.h"

#include <QCheckBox>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QTextEdit>
#include <QWidget>

namespace dstools::labeler {

class BuildCsvPage : public QWidget, public IPageActions {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions)

public:
    explicit BuildCsvPage(QWidget *parent = nullptr);

    QList<QAction *> editActions() const override;
    void setWorkingDirectory(const QString &dir) override;
    QString workingDirectory() const override;

private:
    void buildUi();

    QLineEdit *m_dictPath = nullptr;
    QCheckBox *m_chkPhNum = nullptr;
    QPushButton *m_btnRun = nullptr;
    QProgressBar *m_progress = nullptr;
    QTextEdit *m_log = nullptr;
    QAction *m_runAction = nullptr;

    QString m_workingDir;
};

} // namespace dstools::labeler
