/// @file BuildCsvPage.h
/// @brief CSV transcription build page for the labeler wizard.

#pragma once

#include <dsfw/IPageActions.h>

#include <QCheckBox>
#include <QTextEdit>
#include <QWidget>
#include <dstools/PathSelector.h>
#include <dstools/RunProgressRow.h>

namespace dstools::labeler {

/// @brief IPageActions page that builds transcription CSV from aligned TextGrid files with optional phoneme count computation.
class BuildCsvPage : public QWidget, public IPageActions {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions)

public:
    /// @brief Constructs the CSV build page.
    /// @param parent Optional parent widget.
    explicit BuildCsvPage(QWidget *parent = nullptr);

    void setWorkingDirectory(const QString &dir) override;
    QString workingDirectory() const override;

private:
    void buildUi();  ///< Builds the page UI.

    dstools::widgets::PathSelector *m_dictPath = nullptr;      ///< Dictionary file path selector.
    QCheckBox *m_chkPhNum = nullptr;                           ///< Checkbox to enable phoneme count computation.
    dstools::widgets::RunProgressRow *m_runProgress = nullptr;  ///< Progress row for the build task.
    QTextEdit *m_log = nullptr;                                ///< Log output area.
    QAction *m_runAction = nullptr;                            ///< Action to start the CSV build.

    QString m_workingDir;  ///< Current working directory.
};

} // namespace dstools::labeler
