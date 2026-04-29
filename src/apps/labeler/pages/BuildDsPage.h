#pragma once

#include "IPageActions.h"
#include "IPageProgress.h"

#include <QComboBox>
#include <QSpinBox>
#include <QTextEdit>
#include <QWidget>
#include <dstools/PathSelector.h>
#include <dstools/RunProgressRow.h>

namespace dstools::labeler {

class BuildDsPage : public QWidget, public IPageActions, public IPageProgress {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageProgress)

public:
    explicit BuildDsPage(QWidget *parent = nullptr);

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

    dstools::widgets::PathSelector *m_rmvpePath = nullptr;
    QComboBox *m_gpuSelector = nullptr;
    QSpinBox *m_hopSize = nullptr;
    QSpinBox *m_sampleRate = nullptr;
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
