#pragma once

#include "IPageActions.h"
#include "IPageProgress.h"

#include <QTextEdit>
#include <QWidget>
#include <dstools/GpuSelector.h>
#include <dstools/PathSelector.h>
#include <dstools/RunProgressRow.h>

namespace dstools::labeler {

class GameAlignPage : public QWidget, public IPageActions, public IPageProgress {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageProgress)

public:
    explicit GameAlignPage(QWidget *parent = nullptr);

    QList<QAction *> editActions() const override;
    void setWorkingDirectory(const QString &dir) override;
    QString workingDirectory() const override;

    // IPageProgress
    int progressTotal() const override;
    int progressCurrent() const override;
    bool isRunning() const override;
    QString progressMessage() const override;
    void cancelOperation() override;

private:
    void buildUi();

    dstools::widgets::PathSelector *m_modelPath = nullptr;
    dstools::widgets::GpuSelector *m_gpuSelector = nullptr;
    dstools::widgets::RunProgressRow *m_runProgress = nullptr;
    QTextEdit *m_log = nullptr;
    QAction *m_runAction = nullptr;

    QString m_workingDir;

    // Progress state
    int m_progressTotal = 0;
    int m_progressCurrent = 0;
    bool m_running = false;
    QString m_progressMessage;
};

} // namespace dstools::labeler
