#pragma once

#include "IPageActions.h"

#include <QComboBox>
#include <QSpinBox>
#include <QTextEdit>
#include <QWidget>
#include <dstools/PathSelector.h>
#include <dstools/RunProgressRow.h>

namespace dstools::labeler {

class BuildDsPage : public QWidget, public IPageActions {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions)

public:
    explicit BuildDsPage(QWidget *parent = nullptr);

    QList<QAction *> editActions() const override;
    void setWorkingDirectory(const QString &dir) override;
    QString workingDirectory() const override;

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
};

} // namespace dstools::labeler
