#include <dsfw/AppShell.h>
#include <dsfw/AppSettings.h>
#include <dsfw/AudioPlaybackManager.h>
#include <dsfw/IPlaybackEvents.h>
#include <dsfw/CommonKeys.h>
#include <dsfw/FramelessHelper.h>
#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>
#include <dsfw/IconNavBar.h>
#include <dsfw/Log.h>
#include <dsfw/LogNotifier.h>
#include <dsfw/WorkspaceConfig.h>
#include <dsfw/widgets/ToastNotification.h>

#include <QAction>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QHBoxLayout>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMetaMethod>
#include <QRegularExpression>
#include <QShowEvent>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTimer>
#include <QVector>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace dsfw {

// PIMPL: private implementation struct
struct AppShell::Impl {
    struct PageEntry {
        QWidget* widget = nullptr;
        QString id;
    };

    IconNavBar* navBar = nullptr;
    QStackedWidget* stack = nullptr;
    QVector<PageEntry> pages;
    QList<QAction*> globalActions;
    QString workingDir;
    QMenuBar* menuBar = nullptr;
    AppSettings* settings = nullptr;
    AudioPlaybackManager* audioManager = nullptr;
    bool geometryRestored = false;
};

// Strip accelerator markers for menu title comparison.
// Handles both "&File" and "File(&F)" styles.
static QString menuTitleKey(const QString& title) {
    QString s = title;
    s.remove(QLatin1Char('&'));
    // Remove trailing "(X)" accelerator hint, e.g. "文件(&F)" → "文件"
    static const QRegularExpression trailingAccel(QStringLiteral("\\(\\w\\)$"));
    s.remove(trailingAccel);
    return s.trimmed();
}

// Work around a Windows-specific issue where QMenu popups from a QMenuBar
// inside a frameless (QWK) title bar can render behind the main window.
// The DWM compositor may re-raise the main HWND between menu switches,
// burying subsequent popup windows.  We hook each QMenu::aboutToShow and
// force the popup to the foreground.
static void installMenuPopupRaiseFix(QMenuBar* menuBar) {
#ifdef Q_OS_WIN
    for (auto* action : menuBar->actions()) {
        auto* menu = action->menu();
        if (!menu)
            continue;
        // Disconnect any prior connection from a previous rebuildMenuBar() call
        // (relevant for global menus that persist across rebuilds).
        QObject::disconnect(menu, &QMenu::aboutToShow, menu, nullptr);
        QObject::connect(menu, &QMenu::aboutToShow, menu, [menu]() {
            QTimer::singleShot(0, menu, [menu]() {
                menu->raise();
                menu->activateWindow();
                if (auto hwnd = reinterpret_cast<HWND>(menu->winId())) {
                    ::SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                    ::SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                }
            });
        });
    }
#else
    Q_UNUSED(menuBar);
#endif
}

AppShell::AppShell(QWidget* parent) : QMainWindow(parent), m_impl(std::make_unique<Impl>()) {
    setAcceptDrops(true);

    auto* central = new QWidget;
    auto* hLayout = new QHBoxLayout(central);
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->setSpacing(0);

    m_impl->navBar = new IconNavBar(this);

    m_impl->stack = new QStackedWidget(this);

    hLayout->addWidget(m_impl->navBar);
    hLayout->addWidget(m_impl->stack, 1);

    setCentralWidget(central);

    dsfw::Logger::instance().addSink(dsfw::Logger::ringBufferSink(2000));
    dsfw::Logger::instance().addSink(dsfw::LogNotifier::instance().sink());
    dsfw::Logger::instance().addSink(dsfw::Logger::qtMessageSink());

    m_impl->audioManager = new AudioPlaybackManager(this);

    m_impl->navBar->hide();

    m_impl->menuBar = new QMenuBar(this);
    setMenuBar(m_impl->menuBar);

    connect(m_impl->navBar, &IconNavBar::currentChanged, this, &AppShell::onPageSwitched);

    FramelessHelper::apply(this);
}

AppShell::~AppShell() {
    setMenuWidget(nullptr);
    m_impl->menuBar = nullptr;
}

AudioPlaybackManager* AppShell::audioPlaybackManager() const {
    return m_impl->audioManager;
}

int AppShell::addPage(QWidget* page, const QString& id, const QIcon& icon, const QString& label) {
    if (!page)
        return -1;

    // Verify page implements IPageActions
    auto* actions = qobject_cast<dsfw::IPageActions*>(page);
    Q_UNUSED(actions); // Not required, but expected

    const int idx = m_impl->pages.size();
    m_impl->pages.append({page, id});
    m_impl->stack->addWidget(page);
    m_impl->navBar->addItem(icon, label);

    // Show/hide nav bar based on page count
    m_impl->navBar->setVisible(m_impl->pages.size() > 1);

    // Auto-connect PlayWidget signals to AudioPlaybackManager via IPlaybackEvents interface
    // Uses compile-time type-safe connections through dsfw-core interface
    // to avoid circular dependency between ui-core and widgets modules.
    for (auto* child : page->findChildren<QObject*>()) {
        auto* playEvents = qobject_cast<IPlaybackEvents*>(child);
        if (playEvents) {
            playEvents->registerPlayCallbacks(
                this, [this]() { onChildPlayRequested(); }, [this]() { onChildPlayStopped(); });
        }
    }

    // Auto-select first page
    if (m_impl->pages.size() == 1) {
        setCurrentPage(0);
    }

    return idx;
}

int AppShell::pageCount() const {
    return m_impl->pages.size();
}

int AppShell::currentPageIndex() const {
    return m_impl->stack->currentIndex();
}

QWidget* AppShell::currentPage() const {
    return m_impl->stack->currentWidget();
}

QWidget* AppShell::pageAt(int index) const {
    if (index < 0 || index >= m_impl->pages.size())
        return nullptr;
    return m_impl->pages[index].widget;
}

void AppShell::setCurrentPage(int index) {
    if (index < 0 || index >= m_impl->pages.size())
        return;
    m_impl->navBar->setCurrentIndex(index);
    // onPageSwitched will be called via signal
}

void AppShell::addGlobalMenuActions(const QList<QAction*>& actions) {
    m_impl->globalActions.append(actions);
    rebuildMenuBar();
}

void AppShell::setWorkingDirectory(const QString& dir) {
    if (m_impl->workingDir == dir)
        return;
    m_impl->workingDir = dir;

    // Propagate to all pages
    for (const auto& entry : m_impl->pages) {
        if (auto* actions = qobject_cast<dsfw::IPageActions*>(entry.widget))
            actions->setWorkingDirectory(dir);
    }

    // Notify pages of directory change
    for (const auto& entry : m_impl->pages) {
        if (auto* lifecycle = qobject_cast<dsfw::IPageLifecycle*>(entry.widget))
            lifecycle->onWorkingDirectoryChanged(dir);
    }

    emit workingDirectoryChanged(dir);
}

QString AppShell::workingDirectory() const {
    return m_impl->workingDir;
}

void AppShell::setSettings(AppSettings* settings) {
    m_impl->settings = settings;
}

void AppShell::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    if (!m_impl->geometryRestored && m_impl->settings) {
        m_impl->geometryRestored = true;
        dsfw::WorkspaceConfig ws(m_impl->settings);
        int iconSize = ws.iconSize();
        if (iconSize > 0)
            m_impl->navBar->setIconSize(iconSize);
        auto geomB64 = m_impl->settings->get(dsfw::CommonKeys::WindowGeometry);
        if (!geomB64.isEmpty())
            restoreGeometry(QByteArray::fromBase64(geomB64.toUtf8()));
        auto stateB64 = m_impl->settings->get(dsfw::CommonKeys::WindowState);
        if (!stateB64.isEmpty())
            restoreState(QByteArray::fromBase64(stateB64.toUtf8()));
    }
}

void AppShell::onPageSwitched(int index) {
    if (index < 0 || index >= m_impl->pages.size())
        return;

    // Deactivate old page
    auto* oldPage = m_impl->stack->currentWidget();
    if (oldPage && oldPage != m_impl->pages[index].widget) {
        if (auto* lifecycle = qobject_cast<dsfw::IPageLifecycle*>(oldPage)) {
            if (!lifecycle->onDeactivating()) {
                // Veto — revert nav bar selection
                m_impl->navBar->blockSignals(true);
                m_impl->navBar->setCurrentIndex(m_impl->stack->currentIndex());
                m_impl->navBar->blockSignals(false);
                return;
            }
            lifecycle->onDeactivated();
        }
    }

    // Switch stack
    m_impl->stack->setCurrentIndex(index);

    // Rebuild menu and status bar
    rebuildMenuBar();
    rebuildStatusBar();

    // Activate new page
    auto* newPage = m_impl->pages[index].widget;
    if (auto* lifecycle = qobject_cast<dsfw::IPageLifecycle*>(newPage)) {
        lifecycle->onActivated();
    }

    // Update window title
    if (auto* actions = qobject_cast<dsfw::IPageActions*>(newPage)) {
        const auto title = actions->windowTitle();
        if (!title.isEmpty())
            setWindowTitle(title);
    }

    emit currentPageChanged(index);
}

void AppShell::rebuildMenuBar() {
    // Clear all actions but keep the persistent QMenuBar widget alive.
    // Calling setMenuBar() would replace the menu widget that FramelessHelper
    // installed as the custom title bar, destroying the frameless window.
    m_impl->menuBar->clear();

    auto* page = m_impl->stack->currentWidget();
    auto* actions = page ? qobject_cast<dsfw::IPageActions*>(page) : nullptr;

    // Let the current page populate menus
    if (actions) {
        if (auto* pageBar = actions->createMenuBar(this)) {
            // Transfer menus from the temporary bar into our persistent bar.
            // We create new QMenu wrappers on the persistent bar and steal
            // the actions from the page's menus.  This avoids calling
            // QMenu::setParent() which resets Qt::Popup window flags and
            // breaks popup z-order on frameless (QWK) windows.
            for (auto* srcAction : pageBar->actions()) {
                if (auto* srcMenu = srcAction->menu()) {
                    auto* dstMenu = m_impl->menuBar->addMenu(srcMenu->title());
                    // Move all actions (including sub-menus) from srcMenu → dstMenu.
                    // Re-parent each action so it survives pageBar deletion.
                    const auto srcActions = srcMenu->actions();
                    for (auto* a : srcActions) {
                        srcMenu->removeAction(a);
                        a->setParent(dstMenu);
                        dstMenu->addAction(a);
                    }
                } else if (srcAction->isSeparator()) {
                    m_impl->menuBar->addSeparator();
                } else {
                    srcAction->setParent(m_impl->menuBar);
                    m_impl->menuBar->addAction(srcAction);
                }
            }
            // pageBar is a child of |this| (AppShell). After all actions have
            // been moved out, delete it immediately to prevent a deferred
            // deleteLater() from racing with AppShell's own destruction cascade.
            delete pageBar;
        }
    }

    // Merge global actions into page menus.
    // If a global action owns a QMenu whose title matches a page menu (e.g. both
    // named "File"), merge the global menu's items into the top of the page menu
    // instead of creating a duplicate top-level menu.
    if (!m_impl->globalActions.isEmpty()) {
        // Build lookup: stripped title → page menu action
        QHash<QString, QAction*> pageMenuByTitle;
        for (auto* a : m_impl->menuBar->actions()) {
            if (a->menu())
                pageMenuByTitle.insert(menuTitleKey(a->menu()->title()), a);
        }

        QAction* firstAction = m_impl->menuBar->actions().isEmpty() ? nullptr : m_impl->menuBar->actions().first();

        for (auto* ga : m_impl->globalActions) {
            auto* globalMenu = ga->menu();
            if (!globalMenu) {
                // Plain action — prepend as before
                m_impl->menuBar->insertAction(firstAction, ga);
                continue;
            }

            auto* pageMenuAction = pageMenuByTitle.value(menuTitleKey(globalMenu->title()));
            if (pageMenuAction && pageMenuAction->menu()) {
                // Merge: prepend global items + separator into the matching page menu
                auto* pageMenu = pageMenuAction->menu();
                auto pageActions = pageMenu->actions();
                QAction* insertBefore = pageActions.isEmpty() ? nullptr : pageActions.first();

                for (auto* item : globalMenu->actions()) {
                    pageMenu->insertAction(insertBefore, item);
                }
                // Separator between global and page items
                if (insertBefore)
                    pageMenu->insertSeparator(insertBefore);
            } else {
                // No matching page menu — prepend as a standalone top-level menu
                m_impl->menuBar->insertAction(firstAction, ga);
            }
        }
    }

    // Fix QMenu popup z-order on Windows frameless windows (QWK).
    installMenuPopupRaiseFix(m_impl->menuBar);
}

void AppShell::rebuildStatusBar() {
    auto* sb = statusBar();

    // Remove widgets that were previously added via addWidget/addPermanentWidget.
    // We track them explicitly to avoid accidentally removing internal Qt/QWK
    // children that findChild might pick up.
    const auto children = sb->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (auto* child : children) {
        // Skip the internal QSizeGrip that QStatusBar may create
        if (qstrcmp(child->metaObject()->className(), "QSizeGrip") == 0)
            continue;
        sb->removeWidget(child);
        child->deleteLater();
    }

    auto* page = m_impl->stack->currentWidget();
    if (auto* actions = qobject_cast<dsfw::IPageActions*>(page)) {
        actions->createStatusBarContent(sb);
    }
}

bool AppShell::hasAnyUnsavedChanges() const {
    for (const auto& entry : m_impl->pages) {
        if (auto* actions = qobject_cast<dsfw::IPageActions*>(entry.widget))
            if (actions->hasUnsavedChanges())
                return true;
    }
    return false;
}

void AppShell::onChildPlayRequested() {
    if (auto* w = qobject_cast<QWidget*>(sender()))
        m_impl->audioManager->requestPlay(w);
}

void AppShell::onChildPlayStopped() {
    if (auto* w = qobject_cast<QWidget*>(sender()))
        m_impl->audioManager->releasePlay(w);
}

void AppShell::closeEvent(QCloseEvent* event) {
    // Check active page lifecycle veto
    if (auto* page = m_impl->stack->currentWidget()) {
        if (auto* lifecycle = qobject_cast<dsfw::IPageLifecycle*>(page)) {
            if (!lifecycle->onDeactivating()) {
                event->ignore();
                return;
            }
        }
    }

    // Check unsaved changes across all pages
    if (hasAnyUnsavedChanges()) {
        const auto ret = QMessageBox::question(this, tr("Unsaved Changes"),
                                               tr("There are unsaved changes. Are you sure you want to exit?"),
                                               QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret != QMessageBox::Yes) {
            event->ignore();
            return;
        }
    }

    // Stop all audio playback before shutdown
    if (m_impl->audioManager)
        m_impl->audioManager->stopAll();

    // Dispatch shutdown to all pages
    for (const auto& entry : m_impl->pages) {
        if (auto* lifecycle = qobject_cast<dsfw::IPageLifecycle*>(entry.widget))
            lifecycle->onShutdown();
    }

    // Save window geometry and state
    if (m_impl->settings) {
        m_impl->settings->set(dsfw::CommonKeys::WindowGeometry, QString::fromLatin1(saveGeometry().toBase64()));
        m_impl->settings->set(dsfw::CommonKeys::WindowState, QString::fromLatin1(saveState().toBase64()));
        dsfw::WorkspaceConfig ws(m_impl->settings);
        ws.setIconSize(m_impl->navBar->iconSize());
        ws.saveAll();
        m_impl->settings->flush();
    }

    event->accept();
}

void AppShell::dragEnterEvent(QDragEnterEvent* event) {
    if (auto* page = m_impl->stack->currentWidget()) {
        if (auto* actions = qobject_cast<dsfw::IPageActions*>(page)) {
            if (actions->supportsDragDrop()) {
                actions->handleDragEnter(event);
                return;
            }
        }
    }
    event->ignore();
}

void AppShell::dropEvent(QDropEvent* event) {
    if (auto* page = m_impl->stack->currentWidget()) {
        if (auto* actions = qobject_cast<dsfw::IPageActions*>(page)) {
            if (actions->supportsDragDrop()) {
                actions->handleDrop(event);
                return;
            }
        }
    }
    event->ignore();
}

void AppShell::showToast(widgets::ToastType type, const QString& message, int timeoutMs) {
    widgets::ToastNotification::show(this, type, message, timeoutMs);
}

} // namespace dsfw
