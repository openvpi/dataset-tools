#pragma once

#include <QLabel>
#include <QMainWindow>
#include <QStackedWidget>

#include "StepNavigator.h"

class QMenu;

namespace dstools {
class DsProject;
}

namespace dstools::labeler {

class LabelerWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit LabelerWindow(QWidget *parent = nullptr);
    ~LabelerWindow() override;

    void setWorkingDirectory(const QString &dir);
    QString workingDirectory() const { return m_workingDir; }

    void loadProject(const QString &path);
    void saveProject();
    void saveProjectAs();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupMenuBar();
    void setupStatusBar();
    void onStepChanged(int step);
    QWidget *ensurePage(int step);
    void updateDynamicMenus();
    bool hasAnyUnsavedChanges() const;

    StepNavigator *m_navigator = nullptr;
    QStackedWidget *m_stack = nullptr;

    QMenu *m_editMenu = nullptr;
    QMenu *m_viewMenu = nullptr;
    QMenu *m_toolsMenu = nullptr;

    QLabel *m_statusDir = nullptr;
    QLabel *m_statusStep = nullptr;
    QLabel *m_statusFiles = nullptr;
    QLabel *m_statusModified = nullptr;

    QString m_workingDir;
    QWidget *m_pages[9] = {};

    dstools::DsProject *m_project = nullptr;
};

} // namespace dstools::labeler
