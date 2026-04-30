# Migration Guide: MainWindow to dsfw AppShell

This guide walks through migrating an existing Qt desktop application from a custom `QMainWindow` subclass to the dsfw `AppShell` unified window shell.

## 1. Why Migrate

| Before (custom MainWindow) | After (AppShell) |
|---|---|
| Each app implements its own frameless window, title bar, close handling | `AppShell` provides all of this out of the box |
| `buildMenuBar()`, `buildStatusBar()`, `closeEvent()` duplicated per app | Unified implementation in `AppShell`; pages only provide content |
| Theme integration is manual per window | `Theme` + `AppShell` handle it automatically |
| Adding multi-page navigation requires building your own tab/sidebar system | `addPage()` and `IconNavBar` sidebar work automatically |
| `main.cpp` is 50–100+ lines of window setup boilerplate | `main.cpp` shrinks to ~10 lines |

The same page widget works identically as a standalone single-page app or embedded in a multi-page app — zero code changes required.

## 2. Migration Steps

### Step 1: Replace MainWindow with AppShell

**Before** — your app has a `MainWindow` class:

```cpp
// MainWindow.h
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
private:
    void buildMenuBar();
    void buildStatusBar();
    void setupUi();
    // ... all your app logic mixed with window logic
};
```

**After** — delete `MainWindow`. Create a page widget instead:

```cpp
// EditorPage.h
#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>

class EditorPage : public QWidget,
                   public dstools::labeler::IPageActions,
                   public dstools::labeler::IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)

public:
    explicit EditorPage(QWidget *parent = nullptr);
    // ... your app logic lives here (no window chrome logic)
};
```

### Step 2: Move page content into the page widget

Move the central widget setup from `MainWindow::setupUi()` into `EditorPage`'s constructor. The page widget *is* the central content — no need for `setCentralWidget()`.

**Before:**
```cpp
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    auto *central = new QWidget;
    auto *layout = new QVBoxLayout(central);
    layout->addWidget(new QTextEdit);
    layout->addWidget(new QPushButton("Run"));
    setCentralWidget(central);
    // ... window chrome setup ...
}
```

**After:**
```cpp
EditorPage::EditorPage(QWidget *parent) : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QTextEdit);
    layout->addWidget(new QPushButton("Run"));
}
```

### Step 3: Move menu bar to `createMenuBar()`

**Before:**
```cpp
void MainWindow::buildMenuBar() {
    auto *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("&Open", this, &MainWindow::onOpen, QKeySequence::Open);
    fileMenu->addAction("&Save", this, &MainWindow::onSave, QKeySequence::Save);
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", this, &QWidget::close, QKeySequence::Quit);

    auto *editMenu = menuBar()->addMenu("&Edit");
    editMenu->addAction("&Undo", this, &MainWindow::onUndo, QKeySequence::Undo);
}
```

**After:**
```cpp
QMenuBar *EditorPage::createMenuBar(QWidget *parent) {
    auto *menuBar = new QMenuBar(parent);

    auto *fileMenu = menuBar->addMenu("&File");
    fileMenu->addAction("&Open", this, &EditorPage::onOpen, QKeySequence::Open);
    fileMenu->addAction("&Save", this, &EditorPage::onSave, QKeySequence::Save);
    // Note: no Exit action needed — AppShell handles window close

    auto *editMenu = menuBar->addMenu("&Edit");
    editMenu->addAction("&Undo", this, &EditorPage::onUndo, QKeySequence::Undo);

    return menuBar;
}
```

### Step 4: Move status bar to `createStatusBarContent()`

**Before:**
```cpp
void MainWindow::buildStatusBar() {
    m_statusLabel = new QLabel("Ready");
    statusBar()->addWidget(m_statusLabel);
    m_progressBar = new QProgressBar;
    statusBar()->addPermanentWidget(m_progressBar);
}
```

**After:**
```cpp
QWidget *EditorPage::createStatusBarContent(QWidget *parent) {
    auto *container = new QWidget(parent);
    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);

    m_statusLabel = new QLabel("Ready");
    layout->addWidget(m_statusLabel);
    layout->addStretch();
    m_progressBar = new QProgressBar;
    layout->addWidget(m_progressBar);

    return container;
}
```

### Step 5: Move save/unsaved tracking to `IPageActions`

**Before:**
```cpp
void MainWindow::closeEvent(QCloseEvent *event) {
    if (m_modified) {
        auto result = QMessageBox::question(this, "Unsaved Changes",
            "Save before closing?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (result == QMessageBox::Save) save();
        else if (result == QMessageBox::Cancel) { event->ignore(); return; }
    }
    event->accept();
}
```

**After** — implement two methods; `AppShell::closeEvent()` calls them automatically:

```cpp
bool EditorPage::hasUnsavedChanges() const {
    return m_modified;
}

bool EditorPage::save() {
    // ... your save logic ...
    m_modified = false;
    return true;
}
```

`AppShell` checks `hasUnsavedChanges()` on all pages before closing and prompts the user if needed.

### Step 6: Move drag-drop to `IPageActions`

**Before:**
```cpp
void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event) {
    for (const QUrl &url : event->mimeData()->urls())
        openFile(url.toLocalFile());
}
```

**After:**
```cpp
bool EditorPage::supportsDragDrop() const { return true; }

void EditorPage::handleDragEnter(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void EditorPage::handleDrop(QDropEvent *event) {
    for (const QUrl &url : event->mimeData()->urls())
        openFile(url.toLocalFile());
}
```

`AppShell` forwards drag-drop events to the active page if `supportsDragDrop()` returns true.

### Step 7: Simplify main.cpp

**Before (typical MainWindow app):**
```cpp
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("MyEditor");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("MyOrg");

    // DPI, crash handler, style setup ...
    // Theme initialization ...
    // Frameless window setup ...
    // Service registration ...

    MainWindow w;
    w.resize(1280, 720);
    w.show();

    return app.exec();
}
```

**After (~10 lines):**
```cpp
#include <QApplication>
#include <dsfw/AppShell.h>
#include <dsfw/Theme.h>
#include "EditorPage.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("MyEditor");

    dsfw::Theme::instance().init(app);

    dsfw::AppShell shell;
    shell.addPage(new EditorPage(&shell), "editor", {}, "Editor");
    shell.resize(1280, 720);
    shell.show();

    return app.exec();
}
```

## 3. Before/After Summary

### Before: MainWindow pattern

```
MainWindow (QMainWindow subclass)
├── buildMenuBar()          — manual menu bar construction
├── buildStatusBar()        — manual status bar construction
├── closeEvent()            — unsaved changes check
├── dragEnterEvent()        — drag-drop handling
├── dropEvent()             — drag-drop handling
├── setupUi()               — central widget setup
├── FramelessHelper setup   — manual frameless window
└── Theme wiring            — manual theme connection
```

### After: AppShell + Page pattern

```
main.cpp (~10 lines)
├── Theme::instance().init(app)
├── AppShell shell
├── shell.addPage(new EditorPage, ...)
└── shell.show()

EditorPage (QWidget + IPageActions + IPageLifecycle)
├── windowTitle()               — returns page title
├── createMenuBar(parent)       — returns QMenuBar*
├── createStatusBarContent(parent) — returns QWidget*
├── hasUnsavedChanges() / save()   — save integration
├── handleDragEnter/handleDrop()   — drag-drop
├── onActivated() / onDeactivated() — lifecycle
└── constructor                    — UI content (no window chrome)
```

## 4. Common Pitfalls

### Don't call `QMainWindow` APIs on the page

Pages are plain `QWidget` subclasses, not `QMainWindow`. Don't call `menuBar()`, `statusBar()`, `setCentralWidget()`, or `setWindowTitle()` on the page. Instead, implement the corresponding `IPageActions` methods.

### Remember `Q_INTERFACES`

The page must declare both interfaces for `qobject_cast` to work:

```cpp
Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)
```

Without this, `AppShell` cannot detect that your page implements these interfaces and will treat it as a plain widget (no menu bar, no lifecycle callbacks).

### Don't add an Exit/Quit action

`AppShell` manages window close. If you need a File → Quit action for discoverability, connect it to `qApp->quit()` or the shell's `close()`, but the close-with-unsaved-changes prompt is handled by the shell via `hasUnsavedChanges()`.

### Page ownership

Pass the shell as the page's parent: `new EditorPage(&shell)`. This ensures proper Qt object tree cleanup. The shell does not take ownership via `addPage()` alone.

### Working directory

Use `IPageActions::setWorkingDirectory()` / `IPageLifecycle::onWorkingDirectoryChanged()` instead of managing paths globally. `AppShell::setWorkingDirectory()` broadcasts to all pages.

### Theme changes

Connect to `Theme::instance().themeChanged()` if your page has custom-painted widgets that need to update colors. Standard Qt widgets are re-themed automatically.
