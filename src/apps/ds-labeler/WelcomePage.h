/// @file WelcomePage.h
/// @brief DsLabeler welcome page with new project, open project, and recent files.

#pragma once

#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>
#include <dstools/DsProject.h>

#include <QWidget>

class QLabel;
class QListWidget;
class QPushButton;

namespace dstools {

/// @brief Welcome/home page for DsLabeler.
///
/// Provides buttons to create a new project or open an existing one,
/// plus a list of recently opened projects.
class WelcomePage : public QWidget,
                    public labeler::IPageActions,
                    public labeler::IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)

public:
    explicit WelcomePage(QWidget *parent = nullptr);
    ~WelcomePage() override = default;

    // IPageActions
    QMenuBar *createMenuBar(QWidget *parent) override;
    QString windowTitle() const override;

    // IPageLifecycle
    void onActivated() override;

signals:
    /// Emitted when a project is successfully loaded or created.
    void projectLoaded(dstools::DsProject *project, const QString &projectPath);

private:
    QLabel *m_titleLabel = nullptr;
    QLabel *m_subtitleLabel = nullptr;
    QPushButton *m_btnNewProject = nullptr;
    QPushButton *m_btnOpenProject = nullptr;
    QListWidget *m_recentList = nullptr;

    void onNewProject();
    void onOpenProject();
    void onRecentItemDoubleClicked(int row);
    void loadProject(const QString &path);
    void addToRecent(const QString &path);
    void refreshRecentList();
};

} // namespace dstools
