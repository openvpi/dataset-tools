#pragma once

#include <QCheckBox>
#include <QTextEdit>
#include <QWidget>
#include <dsfw/IPageActions.h>
#include <dsfw/widgets/PathSelector.h>
#include <dsfw/widgets/RunProgressRow.h>

namespace dstools::labeler {

    class BuildCsvPage : public QWidget, public IPageActions {
        Q_OBJECT
        Q_INTERFACES(dstools::labeler::IPageActions)

    public:
        explicit BuildCsvPage(QWidget *parent = nullptr);

        void setWorkingDirectory(const QString &dir) override;
        QString workingDirectory() const override;

    private:
        void buildUi();

        dsfw::widgets::PathSelector *m_dictPath = nullptr;
        QCheckBox *m_chkPhNum = nullptr;
        dsfw::widgets::RunProgressRow *m_runProgress = nullptr;
        QTextEdit *m_log = nullptr;
        QAction *m_runAction = nullptr;

        QString m_workingDir;
    };

} // namespace dstools::labeler
