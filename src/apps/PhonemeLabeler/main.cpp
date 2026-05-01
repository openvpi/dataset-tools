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

    // Add the page's toolbar to the main window
    if (auto *tb = page->toolbar())
        shell.addToolBar(tb);

    // Restore window geometry and state
    auto geomB64 = page->settings().get(PhonemeLabelerKeys::WindowGeometry);
    if (!geomB64.isEmpty())
        shell.restoreGeometry(QByteArray::fromBase64(geomB64.toUtf8()));
    auto stateB64 = page->settings().get(PhonemeLabelerKeys::WindowState);
    if (!stateB64.isEmpty())
        shell.restoreState(QByteArray::fromBase64(stateB64.toUtf8()));

    shell.show();

    // Support command-line file path
    if (argc > 1) {
        page->openFile(QString::fromLocal8Bit(argv[1]));
    }

    // Save window geometry and state on quit
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&]() {
        page->settings().set(PhonemeLabelerKeys::WindowGeometry,
                             QString::fromLatin1(shell.saveGeometry().toBase64()));
        page->settings().set(PhonemeLabelerKeys::WindowState,
                             QString::fromLatin1(shell.saveState().toBase64()));
    });

    return app.exec();
}
