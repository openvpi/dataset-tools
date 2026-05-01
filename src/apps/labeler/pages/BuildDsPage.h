/// @file BuildDsPage.h
/// @brief DiffSinger .ds file build page for the labeler wizard.

#pragma once

#include <dsfw/IPageActions.h>

#include <QComboBox>
#include <QSpinBox>
#include <QTextEdit>
#include <QWidget>
#include <dstools/PathSelector.h>
#include <dstools/RunProgressRow.h>

namespace dstools::labeler {

/// @brief IPageActions page that builds .ds files with RMVPE pitch extraction.
class BuildDsPage : public QWidget, public IPageActions {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions)

public:
    /// @brief Constructs the .ds build page.
    /// @param parent Optional parent widget.
    explicit BuildDsPage(QWidget *parent = nullptr);

    void setWorkingDirectory(const QString &dir) override;
    QString workingDirectory() const override;

private:
    void buildUi();  ///< Builds the page UI.

    dstools::widgets::PathSelector *m_rmvpePath = nullptr;      ///< RMVPE model path selector.
    QComboBox *m_gpuSelector = nullptr;                         ///< GPU device selector.
    QSpinBox *m_hopSize = nullptr;                              ///< Hop size for pitch extraction.
    QSpinBox *m_sampleRate = nullptr;                           ///< Sample rate setting.
    dstools::widgets::RunProgressRow *m_runProgress = nullptr;   ///< Progress row for the build task.
    QTextEdit *m_log = nullptr;                                 ///< Log output area.
    QAction *m_runAction = nullptr;                             ///< Action to start the .ds build.

    QString m_workingDir;  ///< Current working directory.
};

} // namespace dstools::labeler
