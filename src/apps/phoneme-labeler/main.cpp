#include "gui/PhonemeLabelerPage.h"
#include "PhonemeLabelerKeys.h"

#include <QApplication>

#include <dstools/AppInit.h>
#include <dsfw/AppShell.h>
#include <dsfw/Theme.h>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    app.setApplicationName("PhonemeLabeler");
    app.setOrganizationName("DiffSinger");
    app.setApplicationVersion("0.1.0");

    if (!dstools::AppInit::init(app, /*initCrashHandler=*/true))
        return 0;
    dsfw::Theme::instance().init(app);

    dsfw::AppShell shell;
    auto *page = new dstools::phonemelabeler::PhonemeLabelerPage(&shell);
    shell.addPage(page, "phoneme-labeler", {}, "PhonemeLabeler");
    shell.setSettings(&page->settings());

    // Add the page's toolbar to the main window
    if (auto *tb = page->toolbar())
        shell.addToolBar(tb);

    shell.show();

    // Support command-line file path
    if (argc > 1) {
        page->openFile(QString::fromLocal8Bit(argv[1]));
    }

    return app.exec();
}
