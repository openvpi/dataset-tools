#include <QApplication>

#include <dsfw/AppSettings.h>
#include <dsfw/AppShell.h>
#include <dsfw/Log.h>

#include "CounterPage.h"
#include "HelloPage.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("minimal-appshell");

    // Logger setup (console sink)
    auto &logger = dstools::Logger::instance();
    logger.addSink([](const dstools::LogEntry &entry) {
        std::fprintf(stderr, "[%s] %s\n", entry.category.c_str(), entry.message.c_str());
    });

    // AppSettings for geometry persistence
    dstools::AppSettings settings("minimal-appshell");

    dsfw::AppShell shell;
    shell.setSettings(&settings);
    shell.setWindowTitle("dsfw AppShell Example");
    shell.resize(800, 500);

    shell.addPage(new HelloPage, "hello", QIcon(), "Hello");
    shell.addPage(new CounterPage, "counter", QIcon(), "Counter");

    shell.show();
    return app.exec();
}
