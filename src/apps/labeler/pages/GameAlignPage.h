#pragma once

#include "IPageActions.h"

#include <QTextEdit>
#include <QWidget>
#include <dstools/GpuSelector.h>
#include <dstools/PathSelector.h>
#include <dstools/RunProgressRow.h>

namespace dstools::labeler {

class GameAlignPage : public QWidget, public IPageActions {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions)

public:
    explicit GameAlignPage(QWidget *parent = nullptr);

    QList<QAction *> editActions() const override;
    void setWorkingDirectory(const QString &dir) override;
    QString workingDirectory() const override;

private:
    void buildUi();

    dstools::widgets::PathSelector *m_modelPath = nullptr;
    dstools::widgets::GpuSelector *m_gpuSelector = nullptr;
    dstools::widgets::RunProgressRow *m_runProgress = nullptr;
    QTextEdit *m_log = nullptr;
    QAction *m_runAction = nullptr;

    QString m_workingDir;
};

} // namespace dstools::labeler
