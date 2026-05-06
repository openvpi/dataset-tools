#include <dsfw/AppShell.h>
#include <dsfw/CommonKeys.h>
#include <dsfw/FramelessHelper.h>
#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>
#include <dsfw/IconNavBar.h>
#include <dsfw/Log.h>
#include <dsfw/LogNotifier.h>
#include <dsfw/LogPanelWidget.h>
#include <dsfw/widgets/ToastNotification.h>

#include <QAction>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QHBoxLayout>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QRegularExpression>
#include <QShowEvent>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTimer>
#include <QToolButton>

#ifdef Q_OS_WIN
#  include <windows.h>
#endif

namespace dsfw {

    // Strip accelerator markers for menu title comparison.
    // Handles both "&File" and "File(&F)" styles.
    static QString menuTitleKey(const QString &title) {
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
    static void installMenuPopupRaiseFix(QMenuBar *menuBar) {
#ifdef Q_OS_WIN
        for (auto *action : menuBar->actions()) {
            auto *menu = action->menu();
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
                        ::SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                                       SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                        ::SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                                       SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                    }
                });
            });
        }
#else
        Q_UNUSED(menuBar);
#endif
    }

    AppShell::AppShell(QWidget *parent) : QMainWindow(parent) {
        setAcceptDrops(true);

        auto *central = new QWidget;
        auto *hLayout = new QHBoxLayout(central);
        hLayout->setContentsMargins(0, 0, 0, 0);
        hLayout->setSpacing(0);

        m_navBar = new IconNavBar(this);

        // Content area: QStackedWidget + optional LogPanel
        m_contentSplitter = new QSplitter(Qt::Horizontal, this);
        m_contentSplitter->setChildrenCollapsible(false);

        m_stack = new QStackedWidget(this);
        m_contentSplitter->addWidget(m_stack);

        m_logPanel = new LogPanelWidget(this);
        m_logPanel->setFixedWidth(320);
        m_logPanel->hide();
        m_contentSplitter->addWidget(m_logPanel);

        // Init sizes: page takes all space, log panel is 0 when hidden
        m_contentSplitter->setSizes({1, 0});
        m_contentSplitter->setStretchFactor(0, 1);
        m_contentSplitter->setStretchFactor(1, 0);

        hLayout->addWidget(m_navBar);
        hLayout->addWidget(m_contentSplitter, 1);

        setCentralWidget(central);

        // Install sinks (background thread → main thread bridge)
        dstools::Logger::instance().addSink(dstools::Logger::ringBufferSink(500));
        dstools::Logger::instance().addSink(dstools::LogNotifier::instance().sink());
        dstools::Logger::instance().addSink(dstools::Logger::qtMessageSink());
        m_logPanel->connectToNotifier();

        m_navBar->hide(); // Hidden until >1 page

        // Create persistent menu bar and register it BEFORE FramelessHelper
        m_menuBar = new QMenuBar(this);
        setMenuBar(m_menuBar);

        // Toggle log button in the menu bar (right-aligned via corner widget)
        m_toggleLogAction = new QAction(tr("Log"), this);
        m_toggleLogAction->setCheckable(true);
        m_toggleLogAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
        connect(m_toggleLogAction, &QAction::toggled, this, [this](bool visible) {
            if (visible) {
                m_logPanel->show();
                m_contentSplitter->setSizes({m_contentSplitter->width() - m_logPanel->width(),
                                             m_logPanel->width()});
            } else {
                m_logPanel->hide();
                m_contentSplitter->setSizes({1, 0});
            }
        });

        // Add log toggle as a corner widget in the menu bar
        auto *logBtn = new QToolButton(m_menuBar);
        logBtn->setDefaultAction(m_toggleLogAction);
        logBtn->setToolTip(tr("Toggle Log Panel (Ctrl+L)"));
        m_menuBar->setCornerWidget(logBtn, Qt::TopRightCorner);

        connect(m_navBar, &IconNavBar::currentChanged, this, &AppShell::onPageSwitched);

        FramelessHelper::apply(this);
    }

    AppShell::~AppShell() = default;

    int AppShell::addPage(QWidget *page, const QString &id, const QIcon &icon, const QString &label) {
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
            if (auto *actions = qobject_cast<dstools::labeler::IPageActions *>(entry.widget))
                actions->setWorkingDirectory(dir);
        }

        // Notify pages of directory change
        for (const auto &entry : m_pages) {
            if (auto *lifecycle = qobject_cast<dstools::labeler::IPageLifecycle *>(entry.widget))
                lifecycle->onWorkingDirectoryChanged(dir);
        }

        emit workingDirectoryChanged(dir);
    }

    QString AppShell::workingDirectory() const {
        return m_workingDir;
    }

    void AppShell::setSettings(dstools::AppSettings *settings) {
        m_settings = settings;
    }

    void AppShell::showEvent(QShowEvent *event) {
        QMainWindow::showEvent(event);
        if (!m_geometryRestored && m_settings) {
            m_geometryRestored = true;
            auto geomB64 = m_settings->get(dsfw::CommonKeys::WindowGeometry);
            if (!geomB64.isEmpty())
                restoreGeometry(QByteArray::fromBase64(geomB64.toUtf8()));
            auto stateB64 = m_settings->get(dsfw::CommonKeys::WindowState);
            if (!stateB64.isEmpty())
                restoreState(QByteArray::fromBase64(stateB64.toUtf8()));
        }
    }

    void AppShell::onPageSwitched(int index) {
        if (index < 0 || index >= m_pages.size())
            return;

        // Deactivate old page
        auto *oldPage = m_stack->currentWidget();
        if (oldPage && oldPage != m_pages[index].widget) {
            if (auto *lifecycle = qobject_cast<dstools::labeler::IPageLifecycle *>(oldPage)) {
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
        if (auto *lifecycle = qobject_cast<dstools::labeler::IPageLifecycle *>(newPage)) {
            lifecycle->onActivated();
        }

        // Update window title
        if (auto *actions = qobject_cast<dstools::labeler::IPageActions *>(newPage)) {
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
        m_menuBar->clear();

        auto *page = m_stack->currentWidget();
        auto *actions = page ? qobject_cast<dstools::labeler::IPageActions *>(page) : nullptr;

    // Let the current page populate menus
    if (actions) {
        if (auto *pageBar = actions->createMenuBar(this)) {
            // Transfer menus from the temporary bar into our persistent bar.
            // We create new QMenu wrappers on the persistent bar and steal
            // the actions from the page's menus.  This avoids calling
            // QMenu::setParent() which resets Qt::Popup window flags and
            // breaks popup z-order on frameless (QWK) windows.
            for (auto *srcAction : pageBar->actions()) {
                if (auto *srcMenu = srcAction->menu()) {
                    auto *dstMenu = m_menuBar->addMenu(srcMenu->title());
                    // Move all actions (including sub-menus) from srcMenu → dstMenu.
                    // Re-parent each action so it survives pageBar deletion.
                    const auto srcActions = srcMenu->actions();
                    for (auto *a : srcActions) {
                        srcMenu->removeAction(a);
                        a->setParent(dstMenu);
                        dstMenu->addAction(a);
                    }
                } else if (srcAction->isSeparator()) {
                    m_menuBar->addSeparator();
                } else {
                    srcAction->setParent(m_menuBar);
                    m_menuBar->addAction(srcAction);
                }
            }
            pageBar->deleteLater();
        }
    }

        // Merge global actions into page menus.
        // If a global action owns a QMenu whose title matches a page menu (e.g. both
        // named "File"), merge the global menu's items into the top of the page menu
        // instead of creating a duplicate top-level menu.
        if (!m_globalActions.isEmpty()) {
            // Build lookup: stripped title → page menu action
            QHash<QString, QAction *> pageMenuByTitle;
            for (auto *a : m_menuBar->actions()) {
                if (a->menu())
                    pageMenuByTitle.insert(menuTitleKey(a->menu()->title()), a);
            }

            QAction *firstAction = m_menuBar->actions().isEmpty()
                                       ? nullptr
                                       : m_menuBar->actions().first();

            for (auto *ga : m_globalActions) {
                auto *globalMenu = ga->menu();
                if (!globalMenu) {
                    // Plain action — prepend as before
                    m_menuBar->insertAction(firstAction, ga);
                    continue;
                }

                auto *pageMenuAction = pageMenuByTitle.value(menuTitleKey(globalMenu->title()));
                if (pageMenuAction && pageMenuAction->menu()) {
                    // Merge: prepend global items + separator into the matching page menu
                    auto *pageMenu = pageMenuAction->menu();
                    auto pageActions = pageMenu->actions();
                    QAction *insertBefore = pageActions.isEmpty() ? nullptr : pageActions.first();

                    for (auto *item : globalMenu->actions()) {
                        pageMenu->insertAction(insertBefore, item);
                    }
                    // Separator between global and page items
                    if (insertBefore)
                        pageMenu->insertSeparator(insertBefore);
                } else {
                    // No matching page menu — prepend as a standalone top-level menu
                    m_menuBar->insertAction(firstAction, ga);
                }
            }
        }

        // Fix QMenu popup z-order on Windows frameless windows (QWK).
        installMenuPopupRaiseFix(m_menuBar);
    }

    void AppShell::rebuildStatusBar() {
        auto *sb = statusBar();

        // Remove widgets that were previously added via addWidget/addPermanentWidget.
        // We track them explicitly to avoid accidentally removing internal Qt/QWK
        // children that findChild might pick up.
        const auto children = sb->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly);
        for (auto *child : children) {
            // Skip the internal QSizeGrip that QStatusBar may create
            if (qstrcmp(child->metaObject()->className(), "QSizeGrip") == 0)
                continue;
            sb->removeWidget(child);
            child->deleteLater();
        }

        auto *page = m_stack->currentWidget();
        if (auto *actions = qobject_cast<dstools::labeler::IPageActions *>(page)) {
            actions->createStatusBarContent(sb);
        }
    }

    bool AppShell::hasAnyUnsavedChanges() const {
        for (const auto &entry : m_pages) {
            if (auto *actions = qobject_cast<dstools::labeler::IPageActions *>(entry.widget))
                if (actions->hasUnsavedChanges())
                    return true;
        }
        return false;
    }

    void AppShell::closeEvent(QCloseEvent *event) {
        // Check active page lifecycle veto
        if (auto *page = m_stack->currentWidget()) {
            if (auto *lifecycle = qobject_cast<dstools::labeler::IPageLifecycle *>(page)) {
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

        // Dispatch shutdown to all pages
        for (const auto &entry : m_pages) {
            if (auto *lifecycle = qobject_cast<dstools::labeler::IPageLifecycle *>(entry.widget))
                lifecycle->onShutdown();
        }

        // Save window geometry and state
        if (m_settings) {
            m_settings->set(dsfw::CommonKeys::WindowGeometry,
                            QString::fromLatin1(saveGeometry().toBase64()));
            m_settings->set(dsfw::CommonKeys::WindowState,
                            QString::fromLatin1(saveState().toBase64()));
            m_settings->flush();
        }

        event->accept();
    }

    void AppShell::dragEnterEvent(QDragEnterEvent *event) {
        if (auto *page = m_stack->currentWidget()) {
            if (auto *actions = qobject_cast<dstools::labeler::IPageActions *>(page)) {
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
            if (auto *actions = qobject_cast<dstools::labeler::IPageActions *>(page)) {
                if (actions->supportsDragDrop()) {
                    actions->handleDrop(event);
                    return;
                }
            }
        }
        event->ignore();
    }

    void AppShell::showToast(dsfw::widgets::ToastType type, const QString &message,
                             int timeoutMs) {
        widgets::ToastNotification::show(this, type, message, timeoutMs);
    }

    void AppShell::toggleLogPanel() {
        m_toggleLogAction->toggle();
    }

    bool AppShell::isLogPanelVisible() const {
        return m_logPanel && m_logPanel->isVisible();
    }

} // namespace dsfw
