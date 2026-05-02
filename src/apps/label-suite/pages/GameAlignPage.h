#pragma once

#include <dsfw/IPageActions.h>

#include <QWidget>

#include <dstools/RunProgressRow.h>

class QLineEdit;
class QComboBox;
class QTextEdit;

namespace dstools::labeler {

class GameAlignPage : public QWidget, public IPageActions {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions)

public:
    explicit GameAlignPage(QWidget *parent = nullptr);

    void setWorkingDirectory(const QString &dir) override;
    QString workingDirectory() const override;

private:
    void buildUi();

    QLineEdit *m_modelPath = nullptr;
    QComboBox *m_gpuSelector = nullptr;
    dstools::widgets::RunProgressRow *m_runProgress = nullptr;
    QTextEdit *m_log = nullptr;

    QString m_workingDir;
};

} // namespace dstools::labeler
