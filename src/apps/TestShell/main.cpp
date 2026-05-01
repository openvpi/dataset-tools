#include <QAction>
#include <QApplication>
#include <QLabel>
#include <QMenu>
#include <QVBoxLayout>

#include <dsfw/AppShell.h>
#include <dsfw/Theme.h>
#include <dstools/AppInit.h>

// Test: AppShell + QMenu dropdown

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("TestShell");

    if (!dstools::AppInit::init(app))
        return 0;
    dsfw::Theme::instance().init(app);

    // TEMP: clear QSS to test if it causes menu overlap
    app.setStyleSheet({});

    dsfw::AppShell shell;
    shell.setWindowTitle("TestShell");
    shell.resize(600, 400);

    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->addWidget(new QLabel("Click File menu above. Does dropdown appear?"));
    shell.addPage(page, "test", {}, "Test");

    // File menu with submenu items
    auto *fileMenu = new QMenu("&File", &shell);
    fileMenu->addAction("Open");
    fileMenu->addAction("Save");
    fileMenu->addSeparator();
    fileMenu->addAction("Exit", &shell, &QWidget::close);
    shell.addGlobalMenuActions({fileMenu->menuAction()});

    shell.show();

    return app.exec();
}
