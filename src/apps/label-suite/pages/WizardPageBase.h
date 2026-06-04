#pragma once

#include <QAction>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <dsfw/IPageActions.h>
#include <dsfw/widgets/RunProgressRow.h>

namespace dstools::labeler {

/// Base class for label-suite wizard pages providing common UI structure:
/// working directory management, RunProgressRow, log output, and optional Run QAction.
class WizardPageBase : public QWidget, public IPageActions {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions)

public:
    explicit WizardPageBase(QWidget *parent = nullptr);

    void setWorkingDirectory(const QString &dir) override;
    QString workingDirectory() const override;

protected:
    /// Build the common bottom section (RunProgressRow + log + optional QAction).
    /// Call at the end of subclass buildUi() after adding form fields.
    /// @param vLayout       Parent vertical layout.
    /// @param runLabel      Label for the RunProgressRow button.
    /// @param runActionText If non-empty, creates m_runAction with Ctrl+R shortcut.
    void buildCommonUi(QVBoxLayout *vLayout, const QString &runLabel, const QString &runActionText = {});

    QString m_workingDir;
    dsfw::widgets::RunProgressRow *m_runProgress = nullptr;
    QTextEdit *m_log = nullptr;
    QAction *m_runAction = nullptr;
};

} // namespace dstools::labeler