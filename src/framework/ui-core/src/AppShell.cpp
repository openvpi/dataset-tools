#include <dsfw/AppShell.h>
#include <dsfw/FramelessHelper.h>
#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>
#include <dsfw/IconNavBar.h>

#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QMessageBox>
#include <QStackedWidget>
#include <QStatusBar>

namespace dsfw {

AppShell::AppShell(QWidget *parent) : QMainWindow(parent) {
    setAcceptDrops(true);

    auto *central = new QWidget;
    auto *hLayout = new QHBoxLayout(central);
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->setSpacing(0);

    m_navBar = new IconNavBar(this);
    m_stack = new QStackedWidget(this);

    hLayout->addWidget(m_navBar);
    hLayout->addWidget(m_stack, 1);

    setCentralWidget(central);

    m_navBar->hide(); // Hidden until >1 page

    connect(m_navBar, &IconNavBar::currentChanged, this, &AppShell::onPageSwitched);

    FramelessHelper::apply(this);
}

AppShell::~AppShell() = default;

int AppShell::addPage(QWidget *page, const QString &id, const QIcon &icon,
                      const QString &label) {
    if (!page)
        return -1;

    // Verify page implements IPageActions
    auto *actions = qobject_cast<dstools::labeler::IPageActions *>(page);
    Q_UNUSED(actions); // Not required, but expected

    const int idx = m_pages.size();
    m_pages.append({page, id});
    m_stack->addWidget(page);
    m_navBar->addItem(icon, label);

    // Show/hide nav bar based on page count
    m_navBar->setVisible(m_pages.size() > 1);

    // Auto-select first page
    if (m_pages.size() == 1) {
        setCurrentPage(0);
    }

    return idx;
}

int AppShell::pageCount() const {
    return m_pages.size();
}

int AppShell::currentPageIndex() const {
    return m_stack->currentIndex();
}

QWidget *AppShell::currentPage() const {
    return m_stack->currentWidget();
}

QWidget *AppShell::pageAt(int index) const {
    if (index < 0 || index >= m_pages.size())
        return nullptr;
    return m_pages[index].widget;
}

void AppShell::setCurrentPage(int index) {
    if (index < 0 || index >= m_pages.size())
        return;
    m_navBar->setCurrentIndex(index);
    // onPageSwitched will be called via signal
}

void AppShell::addGlobalMenuActions(const QList<QAction *> &actions) {
    m_globalActions.append(actions);
    rebuildMenuBar();
}

void AppShell::setWorkingDirectory(const QString &dir) {
    if (m_workingDir == dir)
        return;
    m_workingDir = dir;

    // Propagate to all pages
    for (const auto &entry : m_pages) {
        if (auto *actions =
                qobject_cast<dstools::labeler::IPageActions *>(entry.widget))
            actions->setWorkingDirectory(dir);
    }

    emit workingDirectoryChanged(dir);
}

QString AppShell::workingDirectory() const {
    return m_workingDir;
}

void AppShell::onPageSwitched(int index) {
    if (index < 0 || index >= m_pages.size())
        return;

    // Deactivate old page
    auto *oldPage = m_stack->currentWidget();
    if (oldPage && oldPage != m_pages[index].widget) {
        if (auto *lifecycle =
                qobject_cast<dstools::labeler::IPageLifecycle *>(oldPage)) {
            if (!lifecycle->onDeactivating()) {
                // Veto — revert nav bar selection
                m_navBar->blockSignals(true);
                m_navBar->setCurrentIndex(m_stack->currentIndex());
                m_navBar->blockSignals(false);
                return;
            }
            lifecycle->onDeactivated();
        }
    }

    // Switch stack
    m_stack->setCurrentIndex(index);

    // Rebuild menu and status bar
    rebuildMenuBar();
    rebuildStatusBar();

    // Activate new page
    auto *newPage = m_pages[index].widget;
    if (auto *lifecycle =
            qobject_cast<dstools::labeler::IPageLifecycle *>(newPage)) {
        lifecycle->onActivated();
    }

    // Update window title
    if (auto *actions =
            qobject_cast<dstools::labeler::IPageActions *>(newPage)) {
        const auto title = actions->windowTitle();
        if (!title.isEmpty())
            setWindowTitle(title);
    }

    emit currentPageChanged(index);
}

void AppShell::rebuildMenuBar() {
    // Delete old menu bar and create a fresh one
    auto *oldBar = menuBar();

    auto *page = m_stack->currentWidget();
    auto *actions =
        page ? qobject_cast<dstools::labeler::IPageActions *>(page) : nullptr;

    QMenuBar *newBar = nullptr;

    if (actions)
        newBar = actions->createMenuBar(this);

    if (!newBar)
        newBar = new QMenuBar(this);

    // Prepend global actions before existing menus
    if (!m_globalActions.isEmpty()) {
        // Insert global actions at the beginning
        auto existingActions = newBar->actions();
        QAction *firstAction =
            existingActions.isEmpty() ? nullptr : existingActions.first();
        for (auto *ga : m_globalActions) {
            newBar->insertAction(firstAction, ga);
        }
    }

    setMenuBar(newBar);

    if (oldBar && oldBar != newBar)
        oldBar->deleteLater();
}

void AppShell::rebuildStatusBar() {
    // Clear existing status bar widgets
    auto *sb = statusBar();

    // Remove all widgets from status bar
    while (auto *child = sb->findChild<QWidget *>(QString(), Qt::FindDirectChildrenOnly)) {
        sb->removeWidget(child);
        child->deleteLater();
    }

    auto *page = m_stack->currentWidget();
    if (auto *actions =
            qobject_cast<dstools::labeler::IPageActions *>(page)) {
        actions->createStatusBarContent(sb);
    }
}

bool AppShell::hasAnyUnsavedChanges() const {
    for (const auto &entry : m_pages) {
        if (auto *actions =
                qobject_cast<dstools::labeler::IPageActions *>(entry.widget))
            if (actions->hasUnsavedChanges())
                return true;
    }
    return false;
}

void AppShell::closeEvent(QCloseEvent *event) {
    // Check active page lifecycle veto
    if (auto *page = m_stack->currentWidget()) {
        if (auto *lifecycle =
                qobject_cast<dstools::labeler::IPageLifecycle *>(page)) {
            if (!lifecycle->onDeactivating()) {
                event->ignore();
                return;
            }
        }
    }

    // Check unsaved changes across all pages
    if (hasAnyUnsavedChanges()) {
        const auto ret = QMessageBox::question(
            this, tr("Unsaved Changes"),
            tr("There are unsaved changes. Are you sure you want to exit?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret != QMessageBox::Yes) {
            event->ignore();
            return;
        }
    }

    event->accept();
}

void AppShell::dragEnterEvent(QDragEnterEvent *event) {
    if (auto *page = m_stack->currentWidget()) {
        if (auto *actions =
                qobject_cast<dstools::labeler::IPageActions *>(page)) {
            if (actions->supportsDragDrop()) {
                actions->handleDragEnter(event);
                return;
            }
        }
    }
    event->ignore();
}

void AppShell::dropEvent(QDropEvent *event) {
    if (auto *page = m_stack->currentWidget()) {
        if (auto *actions =
                qobject_cast<dstools::labeler::IPageActions *>(page)) {
            if (actions->supportsDragDrop()) {
                actions->handleDrop(event);
                return;
            }
        }
    }
    event->ignore();
}

} // namespace dsfw
