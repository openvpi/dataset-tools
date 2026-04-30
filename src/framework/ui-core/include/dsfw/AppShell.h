#pragma once

#include <QList>
#include <QMainWindow>
#include <QVector>

class QStackedWidget;
class QStatusBar;
class QAction;

namespace dsfw {

class IconNavBar;

class AppShell : public QMainWindow {
    Q_OBJECT
public:
    explicit AppShell(QWidget *parent = nullptr);
    ~AppShell() override;

    // Page management
    int addPage(QWidget *page, const QString &id, const QIcon &icon = {},
                const QString &label = {});
    int pageCount() const;
    int currentPageIndex() const;
    QWidget *currentPage() const;
    QWidget *pageAt(int index) const;
    void setCurrentPage(int index);

    // Global actions (persist across page switches, prepended to menu bar)
    void addGlobalMenuActions(const QList<QAction *> &actions);

    // Working directory
    void setWorkingDirectory(const QString &dir);
    QString workingDirectory() const;

signals:
    void currentPageChanged(int index);
    void workingDirectoryChanged(const QString &dir);

protected:
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void onPageSwitched(int index);
    void rebuildMenuBar();
    void rebuildStatusBar();
    bool hasAnyUnsavedChanges() const;

    struct PageEntry {
        QWidget *widget = nullptr;
        QString id;
    };

    IconNavBar *m_navBar = nullptr;
    QStackedWidget *m_stack = nullptr;
    QVector<PageEntry> m_pages;
    QList<QAction *> m_globalActions;
    QString m_workingDir;
};

} // namespace dsfw
