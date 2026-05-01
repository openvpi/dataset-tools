#pragma once

#include <dsfw/IPageActions.h>

#include <QCheckBox>
#include <QTextEdit>
#include <QWidget>
#include <dstools/PathSelector.h>
#include <dstools/RunProgressRow.h>

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

    dstools::widgets::PathSelector *m_dictPath = nullptr;
    QCheckBox *m_chkPhNum = nullptr;
    dstools::widgets::RunProgressRow *m_runProgress = nullptr;
    QTextEdit *m_log = nullptr;
    QAction *m_runAction = nullptr;

    QString m_workingDir;
};

} // namespace dstools::labeler
