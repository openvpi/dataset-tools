# dsfw Getting Started Guide

A practical guide to building desktop applications with the **dsfw** framework — a C++20/Qt6 application framework providing unified window shells, theme management, settings persistence, and service-oriented architecture.

## 1. Prerequisites

| Component | Minimum Version | Notes |
|-----------|----------------|-------|
| **Qt** | 6.8+ | Modules: Core, Gui, Widgets, Svg, Network |
| **C++ compiler** | C++20 | MSVC 2022, GCC, Clang |
| **CMake** | 3.21+ | |
| **vcpkg deps** | — | `nlohmann_json`, `QWindowKit` |

## 2. Installation

dsfw is built as part of the dataset-tools project and installed via CMake.

### Build and install

```bash
# Clone the repository
git clone https://github.com/openvpi/dataset-tools.git
cd dataset-tools

# Configure (adjust paths for your environment)
cmake -B build -G Ninja \
    -DCMAKE_INSTALL_PREFIX=<install-prefix> \
    -DCMAKE_PREFIX_PATH=<qt-dir> \
    -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build

# Install (installs dsfw headers, libraries, and CMake package config)
cmake --install build
```

After installation, the following CMake package is available:

- `dsfw` — with targets `dsfw::core` and `dsfw::ui-core`

## 3. Using dsfw in Your Project

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.21)
project(MyApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(dsfw REQUIRED)

add_executable(MyApp main.cpp)
target_link_libraries(MyApp PRIVATE
    Qt6::Widgets
    dsfw::core       # AppSettings, ServiceLocator, JsonHelper, etc.
    dsfw::ui-core    # AppShell, Theme, IPageActions, IPageLifecycle, etc.
)
```

`dsfw::core` provides settings, JSON helpers, service locator, and domain-agnostic interfaces.
`dsfw::ui-core` provides AppShell, Theme, FramelessHelper, IconNavBar, and page interfaces.

## 4. Hello World: Single-Page App

A minimal application with one page. The sidebar is automatically hidden in single-page mode.

### main.cpp

```cpp
#include <QApplication>
#include <QLabel>
#include <QMenuBar>
#include <QVBoxLayout>

#include <dsfw/AppShell.h>
#include <dsfw/IPageActions.h>
#include <dsfw/Theme.h>

// A minimal page that implements IPageActions for menu/title integration.
class HelloPage : public QWidget, public dstools::labeler::IPageActions {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions)

public:
    explicit HelloPage(QWidget *parent = nullptr) : QWidget(parent) {
        auto *layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel("Hello, dsfw!"));
    }

    // -- IPageActions --
    QString windowTitle() const override { return "Hello World"; }

    QMenuBar *createMenuBar(QWidget *parent) override {
        auto *menuBar = new QMenuBar(parent);
        auto *fileMenu = menuBar->addMenu("&File");
        fileMenu->addAction("&Quit", qApp, &QApplication::quit, QKeySequence::Quit);
        return menuBar;
    }
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("HelloDsfw");

    // Initialize theme (dark mode by default)
    dsfw::Theme::instance().init(app);

    // Create the shell and add a single page
    dsfw::AppShell shell;
    shell.addPage(new HelloPage(&shell), "hello", {}, "Hello");
    shell.resize(800, 600);
    shell.show();

    return app.exec();
}

#include "main.moc"
```

**What happens:**
- `Theme::instance().init(app)` applies the dark theme and Qt palette.
- `AppShell` creates a frameless window with a custom title bar.
- With only one page, the icon sidebar (`IconNavBar`) is automatically hidden.
- The page's `createMenuBar()` provides the window menu bar.
- The page's `windowTitle()` is shown in the title bar.

## 5. Hello World: Multi-Page App

When you add 2+ pages, the `IconNavBar` sidebar appears automatically.

```cpp
#include <QApplication>
#include <QLabel>
#include <QMenuBar>
#include <QVBoxLayout>

#include <dsfw/AppShell.h>
#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>
#include <dsfw/Theme.h>

// Base page with title and optional lifecycle hooks.
class SimplePage : public QWidget,
                   public dstools::labeler::IPageActions,
                   public dstools::labeler::IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)

public:
    SimplePage(const QString &title, QWidget *parent = nullptr)
        : QWidget(parent), m_title(title) {
        auto *layout = new QVBoxLayout(this);
        m_label = new QLabel(this);
        layout->addWidget(m_label);
        updateLabel();
    }

    // -- IPageActions --
    QString windowTitle() const override { return m_title; }

    // -- IPageLifecycle --
    void onActivated() override { updateLabel(); }
    void onDeactivated() override {}

private:
    void updateLabel() { m_label->setText(m_title + " is active"); }

    QString m_title;
    QLabel *m_label;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("MultiPageDemo");

    dsfw::Theme::instance().init(app);

    dsfw::AppShell shell;
    shell.setWindowTitle("MultiPageDemo");

    // Add pages — each gets a sidebar icon/label
    shell.addPage(new SimplePage("Editor", &shell),
                  "editor", QIcon(":/icons/editor.svg"), "Editor");
    shell.addPage(new SimplePage("Settings", &shell),
                  "settings", QIcon(":/icons/settings.svg"), "Settings");

    shell.resize(1024, 768);
    shell.show();

    return app.exec();
}

#include "main.moc"
```

**What happens:**
- `IconNavBar` appears on the left (~40–48px wide) with icons for each page.
- Clicking an icon switches the visible page.
- On page switch, `IPageLifecycle::onDeactivating()` → `onDeactivated()` is called on the old page, and `onActivated()` on the new page.
- Each page's menu bar and status bar content are swapped automatically.

## 6. Key APIs

### `dsfw::AppShell` (`<dsfw/AppShell.h>`)

The unified window shell. Wraps `FramelessHelper` + custom title bar + `IconNavBar` + `QStackedWidget`. Use `addPage()` to register pages; single-page mode hides the sidebar automatically.

### `dsfw::Theme` (`<dsfw/Theme.h>`)

Singleton theme manager. Supports `Dark`, `Light`, and `FollowSystem` modes. Call `Theme::instance().init(app)` at startup. Access colors via `Theme::instance().palette()`. Connect to `themeChanged()` for dynamic updates.

### `dstools::AppSettings` (`<dsfw/AppSettings.h>`)

Type-safe, reactive, JSON-backed settings. Define keys with `SettingsKey<T>` and use `get()`/`set()`. Supports `observe()` for change callbacks with automatic cleanup via QObject context.

```cpp
inline const dstools::SettingsKey<int> Volume("Audio/volume", 80);

dstools::AppSettings settings("MyApp");
settings.set(Volume, 90);
int vol = settings.get(Volume);   // 90
settings.observe(Volume, [](int v) { qDebug() << "Volume:" << v; });
```

### `dstools::ServiceLocator` (`<dsfw/ServiceLocator.h>`)

Global type-safe service registry. Register interface implementations at startup, retrieve them anywhere.

```cpp
dstools::ServiceLocator::set<IFileIOProvider>(new LocalFileIOProvider);
auto *io = dstools::ServiceLocator::get<IFileIOProvider>();
```

### `dstools::labeler::IPageActions` (`<dsfw/IPageActions.h>`)

Page interface for menus, status bar, window title, save/unsaved tracking, and drag-drop. Key methods: `windowTitle()`, `createMenuBar()`, `createStatusBarContent()`, `hasUnsavedChanges()`, `save()`, `handleDragEnter()`, `handleDrop()`.

### `dstools::labeler::IPageLifecycle` (`<dsfw/IPageLifecycle.h>`)

Page lifecycle callbacks: `onActivated()`, `onDeactivating()` (can veto switch), `onDeactivated()`, `onWorkingDirectoryChanged()`, `onShutdown()`.

### `dstools::JsonHelper` (`<dsfw/JsonHelper.h>`)

Safe `nlohmann::json` wrapper. Slash-delimited path queries (`"Audio/sampleRate"`), file I/O with error reporting, typed accessors with fallback defaults.

## 7. Next Steps

- **[Framework Architecture](framework-architecture.md)** — Deep dive into the six-layer architecture, interface-driven design, plugin system, and migration strategy.
- Browse the header files under `include/dsfw/` for full API documentation (Doxygen comments on all public methods).
